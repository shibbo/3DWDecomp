#include "MapObj/TrampleSwitchTimer.hpp"

#include "Layout/IslandMap.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraPoseInfo.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Play/Camera/CameraPoserFix.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/TimerManager.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/IslandDataList.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(TrampleSwitchTimer, OffWait)
NERVE_DECL(TrampleSwitchTimer, On)
NERVE_DECL(TrampleSwitchTimer, OnWait)
NERVE_DECL(TrampleSwitchTimer, Off)
NERVE_DECL(TrampleSwitchTimer, Kill)
NERVE_DECL(TrampleSwitchTimer, WaitCameraIn)
NERVE_DECL(TrampleSwitchTimer, WaitCameraArea)
NERVE_DECL(TrampleSwitchTimer, WaitCameraOut)
NERVE_DECL(TrampleSwitchTimer, WaitGameplayCamera)

NERVES_MAKE_NOSTRUCT(TrampleSwitchTimer, OffWait, On, OnWait, Off, Kill, WaitCameraIn,
                     WaitCameraArea, WaitCameraOut, WaitGameplayCamera)

typedef al::FunctorV0M<TrampleSwitchTimer*, void (TrampleSwitchTimer::*)()>
    TrampleSwitchTimerFunctor;

/// The BGM name used when the switch has no challenge BGM.
const char* const cNoBgmName = "NoBgm";

/// The sample the challenge BGM starts playing from.
const s32 cChallengeBgmStartSample = 838400;
}  // namespace

/**
 * @brief Construct the timer switch.
 * @param pName The actor name.
 */
TrampleSwitchTimer::TrampleSwitchTimer(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Check whether the scenario of the switch's challenge has been completed.
 * @return Whether the scenario is complete.
 */
inline bool TrampleSwitchTimer::isScenarioComplete() const {
    return SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mIslandId - 1,
                                                      mScenarioId - 1);
}

/**
 * @brief Check whether the switch plays its own challenge BGM.
 * @return Whether a challenge BGM is set.
 */
inline bool TrampleSwitchTimer::isPlayingBgm() const {
    return !al::isEqualString(mBgmName, cNoBgmName);
}

/**
 * @brief Pause the field BGM while the challenge camera is shown.
 */
inline void TrampleSwitchTimer::pauseFieldBgm() {
    if (isPlayingBgm()) {
        al::pauseOceanBgm(this, -1);
        al::pauseIslandBgm(this, -1);
    }
}

/**
 * @brief Hide the HUD timer, if one is shown.
 */
inline void TrampleSwitchTimer::hideTimer() {
    if (mSceneLayout != nullptr) {
        mSceneLayout->hideTimer(true);
    }
}

/**
 * @brief Release the timer and allow island warps again outside of race bounds.
 */
inline void TrampleSwitchTimer::releaseTimer() {
    auto* timerManager = TimerManager::tryGetTimerManager(this);
    if (timerManager != nullptr && timerManager->deactivateTimer(this) && mAreaGroup == nullptr) {
        IslandMap::setIslandWarpEnable(this, true);
    }
}

/**
 * @brief Mark the challenge as completed and show the inactive switch.
 */
inline void TrampleSwitchTimer::setScenarioCompleteAnim() {
    mIsScenarioComplete = true;
    if (al::isMtpAnimExist(this, "OffWaitInactive")) {
        al::startMtpAnimAndSetFrameAndStop(this, "OffWaitInactive", 1.0f);
    }
}

/**
 * @brief Check whether the fixed camera finished returning to the player.
 * @return Whether the camera return is done.
 */
inline bool TrampleSwitchTimer::isCameraReturnDone() const {
    return (mIsFixedCamera && mCameraTicket->getPoser<al::CameraPoserFix>()->isReturnDone()) ||
           (mIsFixedActorCamera &&
            mCameraTicket->getPoser<al::CameraPoserFixActor>()->mIsReturnEnd);
}

/**
 * @brief Freeze or unfreeze the camera input of the player.
 * @param isFreeze Whether to freeze the input.
 */
inline void TrampleSwitchTimer::freezeCameraInput(bool isFreeze) {
    mActorSceneInfo->cameraDirector->freezeCameraInput(isFreeze);
}

/**
 * @brief Initialize the switch, its cameras, goal item, collision and stage switches.
 * @param rInfo The actor init info.
 */
void TrampleSwitchTimer::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    mIsMysteryBox = al::isObjectName(rInfo, "TrampleSwitchAnyplaceMysteryBox");
    al::initNerve(this, &NrvTrampleSwitchTimerOffWait, 0);
    mConnector = al::createMtxConnector(this);
    al::listenStageSwitchOff(this, "SwitchTrampleOn",
                             TrampleSwitchTimerFunctor(this, &TrampleSwitchTimer::offSwitch));
    al::listenStageSwitchOn(this, "SwitchReset",
                            TrampleSwitchTimerFunctor(this, &TrampleSwitchTimer::resetSwitch));
    al::listenStageSwitchOn(this, "SwitchKill",
                            TrampleSwitchTimerFunctor(this, &TrampleSwitchTimer::killBySwitch));
    al::listenStageSwitchOn(this, "SwitchOff",
                            TrampleSwitchTimerFunctor(this, &TrampleSwitchTimer::stopBySwitch));
    al::listenStageSwitchOn(this, "SwitchAllEnemiesDead",
                            TrampleSwitchTimerFunctor(this, &TrampleSwitchTimer::allEnemiesDead));
    al::listenStageSwitchOn(this, "SwitchGoalComplete",
                            TrampleSwitchTimerFunctor(this, &TrampleSwitchTimer::goalComplete));
    al::tryGetArg(&mTimeFrameCount, rInfo, "TimeFrameCount");
    al::tryGetArg(&mIsShowTimer, rInfo, "ShowTimer");
    al::tryGetArg(&mIsNoResetOnSwitchOff, rInfo, "NoResetOnSwitchOff");
    al::tryGetArg(&mIsUseCamera, rInfo, "UseCamera");
    al::tryGetArg(&mIsNoIslandScenario, rInfo, "NoIslandScenario");

    if (mIsUseCamera) {
        mCameraTicket = al::initObjectCamera_RS(this, rInfo, nullptr);
        al::tryGetArg(&mFocusCameraInStep, rInfo, "FocusCameraInStep");
        al::tryGetArg(&mFocusCameraOutStep, rInfo, "FocusCameraOutStep");
        al::tryGetArg(&mFocusCameraHoldStep, rInfo, "FocusCameraHoldStep");
        al::tryGetArg(&mAreaCameraHoldStep, rInfo, "AreaCameraHoldStep");

        al::CameraPoser_RS* poser = mCameraTicket->getPoser();
        poser->setInterpoleStep(mFocusCameraInStep);
        poser->setEndInterpoleStep(mFocusCameraOutStep);
        if (al::isEqualString(poser->getName(), "Fixed")) {
            mIsFixedCamera = true;
            mFixedLookAt = static_cast<al::CameraPoserFix*>(poser)->getFixedLookAt();
        } else if (al::isEqualString(poser->getName(), "FixedActor")) {
            mIsFixedActorCamera = true;
        }
    }

    al::tryGetArg(&mIsPreserveCamera, rInfo, "PreserveCamera");
    if (!al::tryGetStringArg(&mBgmName, rInfo, "PlayBgm")) {
        mBgmName = cNoBgmName;
    }

    rc::IUseTimer::init(rInfo, "RaceBounds");

    if (al::calcLinkChildNum(rInfo, "CameraArea") != 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "CameraArea", 0);
        al::AreaInitInfo areaInitInfo(placementInfo, rInfo.getStageSwitchDirector());
        mCameraArea = new al::AreaObj("CameraArea");
        mCameraArea->init(areaInitInfo);
        mCameraArea->invalidate();

        al::AreaObjGroup* group = al::tryFindAreaObjGroup(this, "CameraArea");
        if (group != nullptr) {
            group->resisterAreaObj(mCameraArea);
        }
    }

    if (al::calcLinkChildNum(rInfo, "GoalItem") != 0) {
        mGoalItem = new GoalItem("GoalItem");
        al::initLinksActor(mGoalItem, rInfo, "GoalItem", 0);
        mGoalItem->makeActorDead();
        mGoalItem->setAnimEndListener(this);
    }

    al::initSubActorKeeperNoFile(this, rInfo, 1);
    mCollisionObj = al::createCollisionObj(this, rInfo, "SwitchBase",
                                           al::getHitSensor(this, "CollisionParts2"), nullptr,
                                           nullptr);
    al::registerSubActorSyncClipping(this, mCollisionObj, false);
    al::validateCollisionParts(mCollisionObj);
    mCollisionObj->makeActorAppeared();

    al::tryGetArg(&mScenarioId, rInfo, "ScenarioID");
    s32 quadrant = 0;
    al::tryGetArg(&quadrant, rInfo, "Quadrant");
    mIslandId = quadrant != 0 ? IslandDataFunction::getIslandIDFromParam(quadrant) :
                                mPlacementHolder->getZoneNo();
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) {
        al::tryGetArg(&mFreezeBeforeStart, rInfo, "FreezeBeforeStart");
    }

    al::tryGetArg(&mHasReturnAngles, rInfo, "HasReturnAngles");
    al::tryGetArg(&mCamAngleH, rInfo, "CamAngleH");
    al::tryGetArg(&mCamAngleV, rInfo, "CamAngleV");
    al::tryGetArg(&mCamReturnDist, rInfo, "CamReturnDist");

    sead::Vector3f rotate = sead::Vector3f::zero;
    if (al::tryGetRotate_ParentY(&rotate, al::getPlacementInfo(rInfo))) {
        mCamAngleH -= rotate.y;
    }

    makeActorAppeared();
}

/**
 * @brief Turn the switch off when the trample stage switch is turned off.
 */
void TrampleSwitchTimer::offSwitch() {
    if (al::isNerve(this, &NrvTrampleSwitchTimerOn) ||
        al::isNerve(this, &NrvTrampleSwitchTimerOnWait)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvTrampleSwitchTimerOff);
    }
}

/**
 * @brief Kill the switch when the kill stage switch is turned on.
 */
void TrampleSwitchTimer::killBySwitch() {
    if (al::isNerve(this, &NrvTrampleSwitchTimerKill)) {
        return;
    }

    if (isPlayingBgm()) {
        al::stopBgm(this, mBgmName, 10, -1);
    }

    al::tryOffStageSwitch(this, "SwitchTrampleOn");
    al::setNerve(this, &NrvTrampleSwitchTimerKill);
    hideTimer();
    kill();
}

/**
 * @brief Finish the challenge when all enemies were defeated.
 */
void TrampleSwitchTimer::allEnemiesDead() {
    mIsAllEnemiesDead = true;
    al::stopAllSeFromUser(this, 0);
    if (isPlayingBgm()) {
        al::stopBgm(this, mBgmName, 30, 60);
    }

    hideTimer();
    if (mIsSingleMode) {
        auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->endMysteryBox();
        }
    }
}

/**
 * @brief Called when the challenge goal is reached; marks the shine as collected on the map.
 */
void TrampleSwitchTimer::goalComplete() {
    if (!mIsSingleMode || mIsNoIslandScenario) {
        return;
    }

    if (mScenarioId > 0 && mIslandId < 1) {
        auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        if (islandMap != nullptr) {
            islandMap->setSpecialShineIconComplete(this, true);
        }
    }

    if (isScenarioComplete()) {
        mIsScenarioComplete = true;
        if (al::isMtpAnimExist(this, "OffWaitInactive")) {
            al::startMtpAnimAndSetFrameAndStop(this, "OffWaitInactive", 1.0f);
            if (getEffectKeeper() != nullptr) {
                al::tryDeleteEffect(this, "ItemAvailable");
            }
        }
    }
}

/**
 * @brief Find the HUD timer, attach to the collision and register the shine on the island map.
 */
void TrampleSwitchTimer::initAfterPlacement() {
    if (mIsShowTimer) {
        auto* itemDirector =
            static_cast<ProjectItemDirector*>(getSceneInfo()->itemDirectorBase);
        if (itemDirector != nullptr) {
            mSceneLayout = static_cast<SingleModeSceneLayout*>(itemDirector->getSceneLayout());
        }
    }

    al::attachMtxConnectorToCollision(mConnector, this, false);
    if (!mIsSingleMode || mIsNoIslandScenario) {
        return;
    }

    bool isComplete = isScenarioComplete();
    if (mIslandId < 1) {
        auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        if (islandMap != nullptr) {
            islandMap->addSpecialShineLocation(this, al::getTrans(this),
                                               {mIslandId - 1, mScenarioId - 1});
            islandMap->setSpecialShineIconComplete(this, isComplete);
        }
    }

    if (isComplete) {
        setScenarioCompleteAnim();
    }
}

/**
 * @brief Follow the collision the switch is placed on and update the press state.
 */
void TrampleSwitchTimer::control() {
    al::connectPoseQT(this, mConnector);
    mIsPressed = mIsPressedNext;
    mIsPressedNext = false;
}

/**
 * @brief Make the switch appear and show whether the challenge was already completed.
 */
void TrampleSwitchTimer::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    if (!mIsSingleMode || mIsNoIslandScenario) {
        return;
    }

    if (isScenarioComplete()) {
        setScenarioCompleteAnim();
    } else if (getEffectKeeper() != nullptr) {
        al::tryEmitEffect(this, "ItemAvailable", nullptr);
    }
}

/**
 * @brief Reset the switch once the goal item finished its animation.
 */
void TrampleSwitchTimer::goalItemAnimIsDone() {
    resetSwitchCancel(false);
}

/**
 * @brief Reset the switch switches, optionally also cancelling the challenge.
 * @param isReset Whether to cancel the challenge and return to waiting.
 */
void TrampleSwitchTimer::resetSwitchCancel(bool isReset) {
    if (!isReset) {
        al::tryOffStageSwitchInstant(this, "SwitchTrampleOn");
        al::tryOnStageSwitch(this, "SwitchResetTimerOn");
        return;
    }

    al::tryOffStageSwitch(this, "SwitchTrampleOn");
    al::tryOnStageSwitch(this, "SwitchResetTimerOn");
    releaseTimer();
    al::setNerve(this, &NrvTrampleSwitchTimerOffWait);
    if (mGoalItem != nullptr) {
        mGoalItem->makeActorDeadAll();
    }

    if (mIsSingleMode) {
        auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->endMysteryBox();
        }
    }
}

/**
 * @brief Reset the switch when the reset stage switch is turned on.
 */
void TrampleSwitchTimer::resetSwitch() {
    resetSwitchCancel(true);
}

/**
 * @brief Cancel the challenge when another timer takes over.
 */
void TrampleSwitchTimer::forceCancel() {
    if (mSceneLayout != nullptr) {
        mSceneLayout->hideTimer(false);
    }

    resetSwitchCancel(true);
}

/**
 * @brief Reset the switch when the timer manager resets the current timer.
 */
void TrampleSwitchTimer::reset() {
    resetSwitchCancel(true);
}

/**
 * @brief Check whether the running challenge may be cancelled.
 * @return Whether the switch has race bounds.
 */
bool TrampleSwitchTimer::canCancel() const {
    return mAreaGroup != nullptr;
}

/**
 * @brief Wait to be stepped on; bounces when touched.
 */
void TrampleSwitchTimer::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::startAction(this, "OffWait");
        if (!mIsScenarioComplete && getEffectKeeper() != nullptr) {
            al::tryEmitEffect(this, "ItemAvailable", nullptr);
        }
    }

    if (mIsPressedNext && !mIsPressed) {
        al::startAction(this, "Bounce");
    }

    if (al::isActionEnd(this)) {
        al::startAction(this, "OffWait");
    }
}

/**
 * @brief Press the switch, start the demo and the challenge camera or timer.
 */
void TrampleSwitchTimer::exeOn() {
    rc::invalidatePlayerInput(this, 2);
    if (al::isFirstStep(this)) {
        if (!rc::requestStartDemoInGameCutscene(this)) {
            al::setNerve(this, &NrvTrampleSwitchTimerOn);
            return;
        }

        rc::setDemoAudioType(this, alSeFunction::DemoType(3));
        rc::addDemoActor(this);
        rc::addDemoActor(mCollisionObj);
        if (mTrampleSensor != nullptr &&
            rc::isReallyPlayerActor(al::getSensorHost(mTrampleSensor))) {
            rc::addDemoPlayer(static_cast<PlayerActor*>(al::getSensorHost(mTrampleSensor)));
        }

        auto* timerManager = TimerManager::tryGetTimerManager(this);
        if (timerManager != nullptr) {
            timerManager->activateTimer(this);
        }

        al::startAction(this, "On");
        al::startSe(this, "PgAppear");
        if (getEffectKeeper() != nullptr) {
            al::tryDeleteEffect(this, "ItemAvailable");
        }

        if (mIsSingleMode && mIsMysteryBox) {
            auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
            if (raidon != nullptr) {
                rc::addDemoActor(raidon);
                raidon->forceSpawn(false);
            }
        }

        if (isPlayingBgm()) {
            al::pauseOceanBgm(this, 10);
            al::pauseIslandBgm(this, 10);
        } else {
            al::changeBgmVolume(this, 0.5f, 30);
        }

        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            rc::addDemoActor(controller);
            controller->pause(true);
        }
    }

    if (al::isStep(this, 60)) {
        al::tryOnStageSwitchInstant(this, "SwitchTrampleOn");
        al::tryOffStageSwitchInstant(this, "SwitchResetTimerOn");
    }

    if (al::isStep(this, 60)) {
        al::tryOnStageSwitchInstant(this, "SwitchCameraAnimOn");
        if (mIsUseCamera) {
            mStoredCameraAt = al::getCameraAt_RS(this, 0);
            mStoredCameraPos = al::getCameraPos_RS(this, 0);
        }

        if (mIsFixedCamera) {
            mCameraTicket->getPoser<al::CameraPoserFix>()->resetReturn();
            if (mIsPreserveCamera) {
                mCameraTicket->getPoser<al::CameraPoserFix>()->setFixedLookAt(mFixedLookAt);
            }
        }

        if (mCameraArea != nullptr) {
            mCameraArea->validate();
        } else {
            al::startCamera_RS(this, mCameraTicket, -1);
        }
        return;
    }

    if (!al::isActionPlaying(this, "On") || !al::isActionEnd(this) ||
        !al::isGreaterStep(this, 61)) {
        return;
    }

    if (!mIsUseCamera) {
        al::setNerve(this, &NrvTrampleSwitchTimerOnWait);
        if (mSceneLayout != nullptr) {
            mSceneLayout->showTimer();
            mSceneLayout->setTimer(mTimeFrameCount);
        }
        return;
    }

    if (mGoalItem != nullptr) {
        freezeCameraInput(true);
    }

    if (mIsFixedCamera) {
        mCameraTicket->getPoser<al::CameraPoserFix>()->storeCamera(
            al::getCameraPos_RS(this, 0), al::getCameraAt_RS(this, 0));
    } else if (mIsFixedActorCamera) {
        mCameraTicket->getPoser<al::CameraPoserFixActor>()->storeCamera(
            al::getCameraPos_RS(this, 0), al::getCameraAt_RS(this, 0));
    }

    if (mCameraArea == nullptr) {
        al::setNerve(this, &NrvTrampleSwitchTimerWaitCameraIn);
    } else {
        al::setNerve(this, &NrvTrampleSwitchTimerWaitCameraArea);
    }
}

/**
 * @brief Show the focus camera and make the goal item appear.
 */
void TrampleSwitchTimer::exeWaitCameraIn() {
    if (al::isFirstStep(this)) {
        al::tryStartAction(this, "Before");
    }

    pauseFieldBgm();
    rc::invalidatePlayerInput(this, 2);
    if (al::isStep(this, mFocusCameraInStep) && mGoalItem != nullptr) {
        mGoalItem->appear();
        s32 appearFrames = al::getActionFrameMax(mGoalItem, "AppearRise");
        if (mFocusCameraHoldStep <= appearFrames) {
            mFocusCameraHoldStep = appearFrames;
        }

        mGoalItem->addGoalItemToDemo();
    }

    if (al::isGreaterEqualStep(this, mFocusCameraInStep + mFocusCameraHoldStep)) {
        al::setNerve(this, &NrvTrampleSwitchTimerWaitCameraOut);
    }
}

/**
 * @brief Show the camera area, then move to the focus camera.
 */
void TrampleSwitchTimer::exeWaitCameraArea() {
    if (al::isGreaterEqualStep(this, mAreaCameraHoldStep)) {
        if (mCameraArea != nullptr) {
            mCameraArea->invalidate();
            al::startCamera_RS(this, mCameraTicket, -1);
        }

        al::setNerve(this, &NrvTrampleSwitchTimerWaitCameraIn);
    }

    rc::invalidatePlayerInput(this, 2);
    pauseFieldBgm();
}

/**
 * @brief Return the focus camera to the player.
 */
void TrampleSwitchTimer::exeWaitCameraOut() {
    pauseFieldBgm();
    rc::invalidatePlayerInput(this, 2);
    if (mGoalItem != nullptr) {
        if (al::isStep(this, mFocusCameraHoldStep - 1)) {
            if (mHasReturnAngles) {
                endCameraAtReturnAngles(0.0f);
            } else {
                al::endCamera_RS(this, mCameraTicket, -1, false);
            }

            freezeCameraInput(true);
        }

        if (al::isGreaterEqualStep(this, mFocusCameraHoldStep) && isCameraReturnDone()) {
            al::endCamera_RS(this, mCameraTicket, -1, false);
        }

        if (al::isGreaterEqualStep(this, mFocusCameraOutStep + mFocusCameraHoldStep)) {
            al::setNerve(this, &NrvTrampleSwitchTimerWaitGameplayCamera);
        }
        return;
    }

    if (al::isFirstStep(this)) {
        if (mHasReturnAngles) {
            endCameraAtReturnAngles(mFocusCameraOutStep - 1);
        } else if (mIsFixedCamera) {
            mCameraTicket->getPoser<al::CameraPoserFix>()->setReturn(mFocusCameraOutStep - 1);
        } else if (mIsFixedActorCamera) {
            mCameraTicket->getPoser<al::CameraPoserFixActor>()->setReturn(
                this, mFocusCameraOutStep - 1, false);
        } else {
            al::endCamera_RS(this, mCameraTicket, -1, false);
        }

        freezeCameraInput(true);
    } else if (isCameraReturnDone()) {
        al::endCamera_RS(this, mCameraTicket, -1, false);
    }

    if (al::isGreaterEqualStep(this, mFocusCameraOutStep)) {
        al::setNerve(this, &NrvTrampleSwitchTimerWaitGameplayCamera);
    }
}

/**
 * @brief End the focus camera, returning it at the placed return angles.
 * @param step The number of steps the return takes.
 */
void TrampleSwitchTimer::endCameraAtReturnAngles(f32 step) {
    if (mIsFixedCamera) {
        mCameraTicket->getPoser<al::CameraPoserFix>()->setReturnWithAngles(
            step, mCamAngleH, mCamAngleV, mCamReturnDist);
        return;
    }

    sead::Vector3f cameraPos = al::getCameraPos_RS(this, 0);
    sead::Vector3f cameraAt = al::getCameraAt_RS(this, 0);
    f32 distance = mCamReturnDist;
    if (distance == 0.0f) {
        distance = (cameraPos - cameraAt).length();
    }

    sead::Vector3f returnPos;
    returnPos.x = cameraAt.x + distance * sinf(sead::Mathf::deg2rad(mCamAngleH));
    returnPos.z = cameraAt.z - distance * cosf(sead::Mathf::deg2rad(mCamAngleH));
    returnPos.y = cameraAt.y + distance * sinf(sead::Mathf::deg2rad(mCamAngleV));
    if (mIsFixedActorCamera) {
        mCameraTicket->getPoser<al::CameraPoserFixActor>()->setReturn(this, step, returnPos,
                                                                       cameraAt);
        return;
    }

    al::CameraPoseInfo poseInfo;
    poseInfo.pos = returnPos;
    poseInfo.at = cameraAt;
    poseInfo.up = sead::Vector3f::ey;
    al::endCameraWithNextCameraPose(this, mCameraTicket, &poseInfo, -1);
}

/**
 * @brief Wait for the gameplay camera, then start the timer.
 */
void TrampleSwitchTimer::exeWaitGameplayCamera() {
    rc::invalidatePlayerInput(this, 2);
    if (al::isGreaterEqualStep(this, mFreezeBeforeStart)) {
        al::tryOffStageSwitchInstant(this, "SwitchCameraAnimOn");
        freezeCameraInput(false);
        al::setNerve(this, &NrvTrampleSwitchTimerOnWait);
        rc::requestEndDemoInGameCutscene(this);

        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            controller->resume(true);
        }

        if (mSceneLayout != nullptr) {
            mSceneLayout->showTimer();
            mSceneLayout->setTimer(mTimeFrameCount);
            mSceneLayout->displayTimer();
        }
    }

    pauseFieldBgm();
}

/**
 * @brief Count the challenge time and cancel when it runs out or the player leaves.
 */
void TrampleSwitchTimer::exeOnWait() {
    if (al::isFirstStep(this)) {
        if (mIsReusable) {
            mIsHeldOn = true;
        }

        al::startAction(this, "OnWait");
        if (mSceneLayout != nullptr) {
            mSceneLayout->startTimer();
        }

        mTimer = 0;
        mIsAllEnemiesDead = false;
        if (isPlayingBgm()) {
            al::BgmPlayingRequest request(mBgmName, 3);
            request._18 = cChallengeBgmStartSample;
            al::startBgm(this, request);
            al::resumeOceanBgm(this, -1);
            al::resumeIslandBgm(this, -1);
        } else {
            al::changeBgmVolume(this, 1.0f, 12);
        }

        if (mIsSingleMode) {
            auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
            if (koopaJr != nullptr) {
                koopaJr->startMysteryBox();
            }
        }

        if (mAreaGroup != nullptr) {
            IslandMap::setIslandWarpEnable(this, true);
        }
    }

    if (isPlayerNotInBounds(this, true)) {
        al::stopAllSeFromUser(this, 0);
        al::startSe(this, "PgTimeUp");
        stop(true);
        if (mGoalItem != nullptr) {
            mGoalItem->makeActorDeadAll();
        }
        return;
    }

    bool isDemo = rc::isAnyActiveDemo(this);
    if (mSceneLayout != nullptr) {
        mSceneLayout->pauseTimer(isDemo);
    }

    if (!isDemo) {
        mTimer++;
    }

    if (!mIsHeldOn && mIsReusable) {
        al::setNerve(this, &NrvTrampleSwitchTimerOff);
        return;
    }

    if (mTimeFrameCount < 1 || mIsAllEnemiesDead) {
        return;
    }

    if (mTimer >= mTimeFrameCount) {
        al::stopAllSeFromUser(this, 0);
        al::startSe(this, "PgTimeUp");
        stop(true);
        if (mGoalItem != nullptr) {
            mGoalItem->makeActorDeadAll();
        }
        return;
    }

    if (al::isGreaterEqualStep(this, mTimeFrameCount - 180)) {
        if (mSceneLayout != nullptr) {
            mSceneLayout->setTimerRed();
        }

        al::holdSe(this, "PgTimerFast");
    } else {
        al::holdSe(this, "PgTimerNormal");
    }
}

/**
 * @brief Stop the challenge when the off stage switch is turned on.
 */
void TrampleSwitchTimer::stopBySwitch() {
    stop(false);
}

/**
 * @brief Stop the challenge: reset the switch, stop the BGM and hide the timer.
 * @param isForceReset Whether to reset the switch even if it should not reset on switch off.
 */
void TrampleSwitchTimer::stop(bool isForceReset) {
    if (!mIsNoResetOnSwitchOff || isForceReset) {
        resetSwitchCancel(true);
    }

    if (isPlayingBgm()) {
        al::stopBgm(this, mBgmName, 10, -1);
    }

    hideTimer();
}

/**
 * @brief Release the switch and return to waiting.
 */
void TrampleSwitchTimer::exeOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Off");
        releaseTimer();
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTrampleSwitchTimerOffWait);
    }
}

/**
 * @brief The switch was killed.
 */
void TrampleSwitchTimer::exeKill() {}

/**
 * @brief Check whether the switch was just pressed down.
 * @return Whether the switch turns on this step.
 */
bool TrampleSwitchTimer::isTrigSwitchOn() {
    return al::isNerve(this, &NrvTrampleSwitchTimerOn) && al::isStep(this, 60);
}

/**
 * @brief Check whether the switch is being pressed down.
 * @return Whether the switch is turning on.
 */
bool TrampleSwitchTimer::isEarlyTrigOn() {
    return al::isNerve(this, &NrvTrampleSwitchTimerOn);
}

/**
 * @brief Press the switch on a ground pound and bounce it when touched.
 * @param pMsg The received message.
 * @param pOther The sensor of the sender.
 * @param pSelf The receiving sensor of the switch.
 * @return Whether the message was handled.
 */
bool TrampleSwitchTimer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                    al::HitSensor* pSelf) {
    if (!al::isSensorName(pSelf, "CollisionParts")) {
        if (mIsSingleMode && al::isMsgExplosion(pMsg) &&
            al::isSensorName(pOther, "AmiiboExplosion")) {
            mIsPressedNext = true;
        }
        return false;
    }

    if (al::isMsgPlayerHipDropAll(pMsg) && al::isNerve(this, &NrvTrampleSwitchTimerOffWait)) {
        if (mIsSingleMode && al::isSensorKoopaJr(pOther)) {
            mIsPressedNext = true;
            return true;
        }

        if (rc::isPlayerRollingOnGround(pOther)) {
            return false;
        }

        IslandMap::setIslandWarpEnable(this, false);
        rc::invalidatePlayerInput(al::getSensorHost(pOther), 2);
        al::invalidateClipping(this);
        rc::addScore(this, pOther, 0.0f, 0);
        mIsPressed = true;
        mIsPressedNext = true;
        mTrampleSensor = pOther;
        al::setNerve(this, &NrvTrampleSwitchTimerOn);
        return true;
    }

    if (al::isMsgPlayerFloorTouch(pMsg) || (mIsSingleMode && rc::isMsgRaidonAttack(pMsg))) {
        mIsPressedNext = true;
    }

    if (mIsSingleMode && (al::isMsgEnemyFloorTouch(pMsg) || rc::isMsgSpinnerAttack(pMsg))) {
        mIsPressedNext = true;
    }

    return false;
}

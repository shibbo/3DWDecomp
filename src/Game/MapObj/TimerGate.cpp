#include "MapObj/TimerGate.hpp"

#include "Camera/DummyCameraTarget.hpp"
#include "Enemy/ActorRailBrakeMover.hpp"
#include "Layout/IslandMap.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraPoserFix.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/TimerManager.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
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

namespace {
NERVE_DECL(TimerGate, Wait)
NERVE_DECL(TimerGate, OnWait)

/**
 * @brief Spin of the gate after it was hit; restores the action speed when left.
 */
class TimerGateNrvSpin : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the gate.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<TimerGate>()->exeSpin();
    }

    /**
     * @brief Leave the nerve.
     * @param pKeeper The nerve keeper of the gate.
     */
    void executeOnEnd(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<TimerGate>()->endSpin();
    }
};

NERVE_DECL(TimerGate, On)
NERVE_DECL(TimerGate, RailMove)
NERVE_DECL(TimerGate, WaitCameraIn)
NERVE_DECL(TimerGate, WaitCameraArea)
NERVE_DECL(TimerGate, WaitCameraOut)
NERVE_DECL(TimerGate, WaitCameraReturn)
NERVE_DECL(TimerGate, WaitCameraAreaToIn)
NERVE_DECL(TimerGate, WaitCameraFadeOut)
NERVE_DECL(TimerGate, WaitCameraInHold)

/**
 * @brief Fade back in after a skipped camera; shares its execute with WaitCameraFadeOut.
 */
class TimerGateNrvWaitCameraFadeIn : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the gate.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<TimerGate>()->exeWaitCameraFadeOut();
    }
};

NERVE_DECL(TimerGate, RailMoveDone)
NERVE_DECL(TimerGate, RailFadeOut)

NERVES_MAKE_NOSTRUCT(TimerGate, Wait, OnWait, Spin, On, RailMove, WaitCameraIn, WaitCameraArea,
                     WaitCameraOut, WaitCameraReturn, WaitCameraAreaToIn, WaitCameraFadeOut,
                     WaitCameraInHold, WaitCameraFadeIn, RailMoveDone, RailFadeOut)

typedef al::FunctorV0M<TimerGate*, void (TimerGate::*)()> TimerGateFunctor;

/**
 * @brief Get the controller ports allowed to skip a demo.
 * @return The main controller port as a port mask.
 */
inline sead::BitFlag16 getSkipPorts() {
    return sead::BitFlag16(1 << al::getMainControllerPort());
}
}  // namespace

/**
 * @brief Construct the timer gate.
 * @param pName The actor name.
 */
TimerGate::TimerGate(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Check whether the scenario of the gate's race has been completed.
 * @return Whether the scenario is complete.
 */
inline bool TimerGate::isScenarioComplete() const {
    return SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mIslandId - 1,
                                                      mScenarioId - 1);
}

/**
 * @brief Initialize the gate, its cameras, the race bounds and the stage switches.
 * @param rInfo The actor init info.
 */
void TimerGate::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TimerGate", nullptr);
    al::tryGetArg(&mTimeFrameCount, rInfo, "TimeFrameCount");
    al::tryGetArg(&mIsShowTimer, rInfo, "ShowTimer");
    al::tryGetArg(&mIsUseCamera, rInfo, "UseCamera");
    al::listenStageSwitchOn(this, "SwitchGoalComplete",
                            TimerGateFunctor(this, &TimerGate::goalComplete));

    if (mIsUseCamera) {
        mCameraTicket = al::initObjectCamera_RS(this, rInfo, nullptr);
        al::tryGetArg(&mFocusCameraInStep, rInfo, "FocusCameraInStep");
        al::tryGetArg(&mFocusCameraOutStep, rInfo, "FocusCameraOutStep");
        al::tryGetArg(&mFocusCameraHoldStep, rInfo, "FocusCameraHoldStep");
        al::tryGetArg(&mAreaCameraHoldStep, rInfo, "AreaCameraHoldStep");
        al::tryGetArg(&mAreaCameraOutStep, rInfo, "AreaCameraOutStep");

        al::CameraPoser_RS* poser = mCameraTicket->getPoser();
        poser->setInterpoleStep(mFocusCameraInStep);
        poser->setEndInterpoleStep(mFocusCameraOutStep);
    }

    al::tryGetArg(&mIsSetReturnAngles, rInfo, "SetReturnAngles");
    if (mIsSetReturnAngles) {
        al::tryGetArg(&mReturnAngleH, rInfo, "ReturnAngleH");
        al::tryGetArg(&mReturnAngleV, rInfo, "ReturnAngleV");
    }

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

    mSkipLayout = new DemoSkipLayout(*rInfo.getLayoutInitInfo(), true);

    if (al::isExistRail(rInfo)) {
        mDummyTarget = new DummyCameraTarget("Dummy Target");
        mDummyTarget->init(rInfo);
        al::setTrans(mDummyTarget, sead::Vector3f::zero);
        mDummyTarget->makeActorDead();

        mRailMover = new ActorRailBrakeMover(this, 60.0f, nullptr);
        al::setRailPosToCoord(this, al::getRailCoord(this));
        mRailCameraTicket = al::initProgramableCamera_RS(this, rInfo, "RailCamera", &mRailCameraPos,
                                                         &mRailCameraAt, &mRailCameraUp);
        al::tryGetArg(&mRailCameraDuration, rInfo, "railCameraDuration");
        al::tryGetArg(&mRailCameraDistance, rInfo, "railCameraDistance");
        al::tryGetArg(&mRailCameraInStep, rInfo, "railCameraInStep");
        al::tryGetArg(&mRailCameraOutStep, rInfo, "railCameraOutStep");
        al::tryGetArg(&mGoalItemDelay, rInfo, "GoalItemDelay");
        mRailCameraTicket->getPoser()->setInterpoleStep(mRailCameraInStep);
        mRailCameraTicket->getPoser()->setEndInterpoleStep(mRailCameraOutStep);
    }

    rc::IUseTimer::init(rInfo, "RaceBounds");
    al::initNerve(this, &NrvTimerGateWait, 0);
    al::tryGetArg(&mScenarioId, rInfo, "ScenarioID");

    s32 quadrant = 0;
    al::tryGetArg(&quadrant, rInfo, "Quadrant");
    mIslandId = quadrant != 0 ? IslandDataFunction::getIslandIDFromParam(quadrant) :
                                mPlacementHolder->getZoneNo();
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;

    al::listenStageSwitchOn(this, "SwitchReset", TimerGateFunctor(this, &TimerGate::resetSwitch));
    al::listenStageSwitchOn(this, "SwitchOff", TimerGateFunctor(this, &TimerGate::stop));
    if (al::listenStageSwitchOnAppear(this,
                                      TimerGateFunctor(this, &TimerGate::makeActorAppeared))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

/**
 * @brief Called when the race goal is reached; marks the shine as collected on the map.
 */
void TimerGate::goalComplete() {
    if (!mIsSingleMode || !isScenarioComplete()) {
        return;
    }

    auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
    if (islandMap != nullptr) {
        islandMap->setSpecialShineIconComplete(this, true);
    }

    setGateActions();
    stop();
}

/**
 * @brief Put the gate back to waiting and reset the race switches.
 */
void TimerGate::resetSwitch() {
    al::setNerve(this, &NrvTimerGateWait);
    al::tryOffStageSwitch(this, "SwitchTrampleOn");
    al::tryOnStageSwitch(this, "SwitchResetTimerOn");
    al::tryOffStageSwitchInstant(this, "SwitchGoalItemAppearOn");
}

/**
 * @brief Stop the race: reset the switches, release the timer and hide the HUD timer.
 */
void TimerGate::stop() {
    resetSwitch();
    TimerManager::tryGetTimerManager(this)->deactivateTimer(this);
    al::stopAllSeFromUser(this, 0);
    if (mSceneLayout != nullptr) {
        mSceneLayout->hideTimer(true);
    }
}

/**
 * @brief Find the HUD timer and register the gate's shine on the island map.
 */
void TimerGate::initAfterPlacement() {
    if (mIsShowTimer) {
        auto* itemDirector =
            static_cast<ProjectItemDirector*>(getSceneInfo()->itemDirectorBase);
        if (itemDirector != nullptr) {
            mSceneLayout = static_cast<SingleModeSceneLayout*>(itemDirector->getSceneLayout());
        }
    }

    if (!mIsSingleMode) {
        return;
    }

    setGateActions();
    auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
    if (islandMap == nullptr) {
        return;
    }

    islandMap->addSpecialShineLocation(this, al::getTrans(this),
                                       {mIslandId - 1, mScenarioId - 1});
    islandMap->setSpecialShineIconComplete(this, isScenarioComplete());
}

/**
 * @brief Start the gate actions and effects matching whether the race was already won.
 */
void TimerGate::setGateActions() {
    al::startAction(this, "TimerGateOn");
    if (isScenarioComplete()) {
        al::startMtpAnim(this, "TimerGateOff");
        if (getEffectKeeper() != nullptr) {
            al::tryDeleteEffect(this, "ItemAvailable");
        }
    } else if (getEffectKeeper() != nullptr) {
        al::tryEmitEffect(this, "ItemAvailable", nullptr);
    }
}

/**
 * @brief End the race camera demo and start counting down.
 */
void TimerGate::finishCameraMove() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        controller->resume(true);
    }

    mIsSkipped = false;
    al::changeBgmVolume(this, 1.0f, 12);
    al::setNerve(this, &NrvTimerGateOnWait);
    rc::requestEndDemoMovingCamera(this);
    al::tryOffStageSwitchInstant(this, "SwitchDemoOn");
}

/**
 * @brief Reset the gate when the timer manager resets the current timer.
 */
void TimerGate::reset() {
    resetSwitch();
}

/**
 * @brief Check whether the running race may be cancelled.
 * @return Whether the timer is counting down.
 */
bool TimerGate::canCancel() const {
    return al::isNerve(this, &NrvTimerGateOnWait);
}

/**
 * @brief Start the race when Plessie swims through the gate from the front.
 * @param pSelf The gate sensor.
 * @param pOther The sensor touching the gate.
 */
void TimerGate::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isNerve(this, &NrvTimerGateWait) && !al::isNerve(this, &NrvTimerGateSpin)) {
        return;
    }

    if (!al::isSensorName(pSelf, "GateSensor") || !al::isSensorRide(pOther) ||
        !al::isSensorPlessie(pOther) || !rc::isPlayerOnRaidon(al::getPlayerActor(this, 0))) {
        return;
    }

    al::LiveActor* host = al::getSensorHost(pOther);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    sead::Vector3f velocityDir = al::getActorVelocity(pOther);
    front.normalize();
    velocityDir.normalize();

    sead::Vector3f trans = al::getTrans(this);
    sead::Vector3f toHost = al::getTrans(host) - trans;
    f32 distance = toHost.dot(front);
    if (distance > 0.0f) {
        f32 velocityDot = velocityDir.dot(front);
        const sead::Vector3f& velocity = al::getActorVelocity(pOther);
        f32 speedH = sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        if (velocityDot < 0.0f && distance < speedH &&
            TimerManager::tryGetTimerManager(this)->activateTimer(this)) {
            al::setNerve(this, &NrvTimerGateOn);
        }
    }
}

/**
 * @brief Make the gate appear and restart waiting.
 */
void TimerGate::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    if (mIsSingleMode) {
        setGateActions();
        al::setNerve(this, &NrvTimerGateWait);
    }
}

/**
 * @brief Remove the gate's effects and kill it.
 */
void TimerGate::makeActorDead() {
    al::tryDeleteEffect(this, "ItemAvailable");
    al::LiveActor::makeActorDead();
}

/**
 * @brief Cancel the race when another timer takes over.
 */
void TimerGate::forceCancel() {
    stop();
}

/**
 * @brief Spin the gate when the player attacks it.
 * @param pMsg The received message.
 * @param pOther The sensor of the attacker.
 * @param pSelf The receiving sensor of the gate.
 * @return Whether the message was handled.
 */
bool TimerGate::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (!al::isSensorName(pSelf, "GateReact")) {
        return false;
    }

    if (!al::isNerve(this, &NrvTimerGateWait) && !al::isNerve(this, &NrvTimerGateSpin)) {
        return false;
    }

    if (al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerClimbRollingAttack(pMsg) ||
        al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
        al::isMsgPlayerKouraAttack(pMsg) || al::isMsgExplosion(pMsg) ||
        al::isMsgPlayerRollingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerRollingAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg)) {
        if (!al::isNerve(this, &NrvTimerGateSpin) || al::isGreaterEqualStep(this, 21) ||
            al::isMsgPlayerFireBallAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }

        al::setNerve(this, &NrvTimerGateSpin);
        return true;
    }

    return false;
}

/**
 * @brief Wait for the player; refreshes the gate actions after a spin.
 */
void TimerGate::exeWait() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode && !mIsSpinEnd) {
            setGateActions();
        }

        mIsSpinEnd = false;
        al::validateClipping(this);
    }
}

/**
 * @brief Spin fast and slow down again before returning to waiting.
 */
void TimerGate::exeSpin() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::setActionFrameRate(this, 2.0f);
    }

    if (al::isStep(this, 120)) {
        al::setActionFrameRate(this, 1.0f);
        mIsSpinEnd = true;
        al::setNerve(this, &NrvTimerGateWait);
        return;
    }

    if (al::isGreaterEqualStep(this, 60) && al::isLessEqualStep(this, 120)) {
        f32 rate = (al::getNerveStep(this) - 60) / 60.0f;
        al::setActionFrameRate(this, al::lerpValue(rate, 2.0f, 1.0f));
    }
}

/**
 * @brief Restore the action speed when leaving the spin.
 */
void TimerGate::endSpin() {
    al::setActionFrameRate(this, 1.0f);
    al::validateClipping(this);
}

/**
 * @brief Activate the gate and start the race camera or the timer.
 */
void TimerGate::exeOn() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "TimerGateActivate");
        al::startSe(this, "PgAppear");
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidon != nullptr) {
            raidon->toggleGameWindow(false);
        }
    }

    if (al::isActionPlaying(this, "TimerGateActivate") && al::isActionEnd(this)) {
        al::startAction(this, "TimerGatePlaying");
    }

    if (al::isStep(this, 2)) {
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        al::tryOffStageSwitch(this, "SwitchResetTimerOn");
        return;
    }

    if (!al::isStep(this, 3)) {
        return;
    }

    if (mIsUseCamera) {
        if (!rc::requestStartDemoMovingCamera(this, nullptr, false)) {
            return;
        }

        auto* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            controller->pause(true);
        }

        al::tryOnStageSwitchInstant(this, "SwitchDemoOn");
        mSkipLayout->appear();
        mIsSkipped = false;
        al::changeBgmVolume(this, 0.5f, 90);
        rc::setDemoAudioType(this, alSeFunction::DemoType(3));

        auto* poser = mCameraTicket->getPoser<al::CameraPoserFix>();
        poser->storeCamera(al::getCameraPos_RS(this, 0), al::getCameraAt_RS(this, 0));
        poser->resetReturn();

        if (mRailMover != nullptr) {
            if (mIsSetReturnAngles) {
                sead::Vector3f dir(sinf(sead::Mathf::deg2rad(mReturnAngleH)),
                                   sinf(sead::Mathf::deg2rad(mReturnAngleV)),
                                   cosf(sead::Mathf::deg2rad(mReturnAngleH)));
                dir.normalize();
                mReturnAt = al::getCameraAt_RS(this, 0);
                f32 distance = (mReturnAt - al::getCameraPos_RS(this, 0)).length();
                mReturnPos = mReturnAt - dir * distance;
            } else {
                mReturnPos = al::getCameraPos_RS(this, 0);
                mReturnAt = al::getCameraAt_RS(this, 0);
            }

            al::setNerve(this, &NrvTimerGateRailMove);
        } else if (mCameraArea != nullptr) {
            mSkipLayout->appear();
            mIsSkipped = false;
            mCameraArea->validate();
            al::setNerve(this, &NrvTimerGateWaitCameraArea);
        } else {
            al::startCamera_RS(this, mCameraTicket, -1);
            al::setNerve(this, &NrvTimerGateWaitCameraIn);
        }
    } else {
        if (mSceneLayout != nullptr) {
            mSceneLayout->setTimer(mTimeFrameCount);
            mSceneLayout->showTimer();
        }

        al::setNerve(this, &NrvTimerGateOnWait);
    }
}

/**
 * @brief Cancel the race: reset the gate and release the timer.
 */
void TimerGate::cancel() {
    resetSwitch();
    al::stopAllSeFromUser(this, 0);
    if (mSceneLayout != nullptr) {
        mSceneLayout->hideTimer(true);
    }

    al::setNerve(this, &NrvTimerGateWait);
    TimerManager::tryGetTimerManager(this)->deactivateTimer(this);
}

/**
 * @brief Count the race time down and cancel when it runs out or the player leaves.
 */
void TimerGate::exeOnWait() {
    if (al::isFirstStep(this)) {
        if (mSceneLayout != nullptr) {
            mSceneLayout->startTimer();
        }

        al::tryDeleteEffect(this, "ItemAvailable");
        mTimer = mTimeFrameCount;
    }

    bool isDemo = rc::isAnyActiveDemo(this);
    if (mSceneLayout != nullptr) {
        mSceneLayout->pauseTimer(isDemo);
    }

    if (!isDemo) {
        mTimer--;
    }

    if (isPlayerNotInBounds(this, true)) {
        cancel();
        return;
    }

    if (mTimeFrameCount < 1) {
        return;
    }

    if (mTimer <= 0) {
        cancel();
        al::startSe(this, "PgTimeUp");
    } else if (mTimer <= 180) {
        if (mSceneLayout != nullptr) {
            mSceneLayout->setTimerRed();
        }

        al::holdSe(this, "PgTimerFast");
    } else {
        al::holdSe(this, "PgTimerNormal");
    }
}

/**
 * @brief The gate is off.
 */
void TimerGate::exeOff() {}

/**
 * @brief Show the focus camera, then return it.
 */
void TimerGate::exeWaitCameraIn() {
    if (al::isGreaterEqualStep(this, mFocusCameraInStep + mFocusCameraHoldStep)) {
        if (mIsSetReturnAngles) {
            mCameraTicket->getPoser<al::CameraPoserFix>()->setReturnWithAngles(
                mFocusCameraOutStep, mReturnAngleH, mReturnAngleV, 0.0f);
            al::setNerve(this, &NrvTimerGateWaitCameraReturn);
        } else {
            al::setNerve(this, &NrvTimerGateWaitCameraOut);
        }
    }
}

/**
 * @brief Show the camera area, then move to the focus camera.
 */
void TimerGate::exeWaitCameraArea() {
    if (al::isGreaterEqualStep(this, mAreaCameraHoldStep)) {
        if (mCameraArea != nullptr) {
            mCameraArea->invalidate();
            auto* poser = mCameraTicket->getPoser<al::CameraPoserFix>();
            poser->setInterpoleStep(mAreaCameraOutStep);
            poser->setEndInterpoleStep(0);
            poser->resetReturn();
            al::startCamera_RS(this, mCameraTicket, -1);
            al::setNerve(this, &NrvTimerGateWaitCameraAreaToIn);
        }
        return;
    }

    if (mSkipLayout->isSkip(getSkipPorts())) {
        mIsSkipped = true;
        al::setNerve(this, &NrvTimerGateWaitCameraFadeOut);
    }
}

/**
 * @brief Move from the camera area to the focus camera.
 */
void TimerGate::exeWaitCameraAreaToIn() {
    if (al::isGreaterEqualStep(this, mAreaCameraOutStep)) {
        if (mIsSetReturnAngles) {
            al::setNerve(this, &NrvTimerGateWaitCameraInHold);
        } else {
            al::setNerve(this, &NrvTimerGateWaitCameraOut);
        }
        return;
    }

    if (mSkipLayout->isSkip(getSkipPorts())) {
        mIsSkipped = true;
        al::setNerve(this, &NrvTimerGateWaitCameraFadeOut);
    }
}

/**
 * @brief Hold the focus camera after the camera area, then return it.
 */
void TimerGate::exeWaitCameraInHold() {
    if (al::isGreaterEqualStep(this, mFocusCameraHoldStep)) {
        if (mIsSetReturnAngles) {
            mCameraTicket->getPoser<al::CameraPoserFix>()->setReturnWithAngles(
                mFocusCameraOutStep, mReturnAngleH, mReturnAngleV, 0.0f);
            al::setNerve(this, &NrvTimerGateWaitCameraReturn);
        } else {
            al::setNerve(this, &NrvTimerGateWaitCameraOut);
        }
    }
}

/**
 * @brief Wait for the focus camera to return to the player.
 */
void TimerGate::exeWaitCameraReturn() {
    if (mCameraTicket->getPoser<al::CameraPoserFix>()->isReturnDone() &&
        al::isGreaterEqualStep(this, 2)) {
        al::setNerve(this, &NrvTimerGateWaitCameraOut);
    }
}

/**
 * @brief End the focus camera and start the timer.
 */
void TimerGate::exeWaitCameraOut() {
    if (al::isFirstStep(this)) {
        mSkipLayout->end();
        mSkipLayout->kill();
    }

    if (al::isGreaterEqualStep(this, mFocusCameraOutStep)) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidon != nullptr) {
            raidon->toggleGameWindow(true);
        }

        if (mSceneLayout != nullptr) {
            mSceneLayout->setTimer(mTimeFrameCount);
            mSceneLayout->showTimer();
        }
    }

    if (al::isStep(this, mIsSkipped ? 30 : mFocusCameraOutStep + 30)) {
        if (al::isActiveCamera(mCameraTicket)) {
            al::endCamera_RS(this, mCameraTicket, -1, false);
        }

        finishCameraMove();
    }
}

/**
 * @brief Fade out to skip the camera, then fade back in (shared by both fade nerves).
 */
void TimerGate::exeWaitCameraFadeOut() {
    auto* sceneLayout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);

    if (al::isNerve(this, &NrvTimerGateWaitCameraFadeIn)) {
        if (al::isFirstStep(this)) {
            al::endCamera_RS(this, mCameraTicket, 0, false);
            sceneLayout->getWipeBlack()->startOpen(sceneLayout->getWipeFrames());
            al::setNerve(this, &NrvTimerGateWaitCameraOut);
        }
        return;
    }

    if (al::isFirstStep(this)) {
        sceneLayout->getWipeBlack()->startClose(sceneLayout->getWipeFrames());
        return;
    }

    if (!sceneLayout->getWipeBlack()->isCloseEnd()) {
        return;
    }

    if (mCameraArea != nullptr && mCameraArea->mIsValid && !mCameraArea->mIsDisabled &&
        mCameraArea->_66) {
        mCameraArea->invalidate();
    }

    if (!al::isActiveCamera(mCameraTicket)) {
        auto* poser = mCameraTicket->getPoser<al::CameraPoserFix>();
        poser->setInterpoleStep(mAreaCameraOutStep);
        poser->setEndInterpoleStep(0);
        poser->resetReturn();
        al::startCamera_RS(this, mCameraTicket, 0);
    }

    al::setNerve(this, &NrvTimerGateWaitCameraFadeIn);
}

/**
 * @brief Fly the rail camera along the race course.
 */
void TimerGate::exeRailMove() {
    if (al::isFirstStep(this)) {
        al::startCamera_RS(this, mRailCameraTicket, -1);
        al::setRailPosToCoord(this, 0.0f);
        mRailMover->setSpeedByTime(mRailCameraDuration, true);
        mRailFront = al::getRailDir(this);
        mRailAngleV = 0.0f;
    }

    if (al::isStep(this, mGoalItemDelay)) {
        al::tryOnStageSwitchInstant(this, "SwitchGoalItemAppearOn");
    }

    if (al::isStep(this, 90)) {
        mRailMover->setSpeedByTime(mRailCameraDuration, false);
    }

    f32 partRate;
    s32 partIndex;
    bool isEnd = mRailMover->moveSyncRailByTimeInOut(this, &partIndex, &partRate, false);
    mRailCameraAt = al::getRailPos(this);
    sead::Vector3f railDir = al::getRailDir(this);
    railDir.normalize();
    al::lerpVec(&mRailFront, mRailFront, railDir, 0.1f);
    mRailFront.normalize();

    f32 angle = sead::Mathf::rad2deg(atan2f(mRailFront.x, mRailFront.z));
    if (al::isStep(this, 30)) {
        mRailAngleH = angle;
    }

    angle = al::lerpDegree(mRailAngleH, angle, 0.2f);
    f32 rad = sead::Mathf::deg2rad(angle);
    mRailCameraPos.x = mRailCameraAt.x - sinf(rad) * mRailCameraDistance;
    mRailCameraPos.z = mRailCameraAt.z - cosf(rad) * mRailCameraDistance;
    mRailAngleH = angle;
    mRailCameraPos.y = mRailCameraAt.y + mRailCameraDistance * 0.34202015f;

    if (isEnd) {
        al::setNerve(this, &NrvTimerGateRailMoveDone);
        return;
    }

    if (mSkipLayout->isSkip(getSkipPorts())) {
        auto* sceneLayout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
        sceneLayout->getWipeBlack()->startClose(sceneLayout->getWipeFrames());
        mIsSkipped = true;
        al::setNerve(this, &NrvTimerGateRailFadeOut);
    }
}

/**
 * @brief Skip the rail camera: move to the end while the screen is black, then fade back in.
 */
void TimerGate::exeRailFadeOut() {
    auto* sceneLayout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (!sceneLayout->getWipeBlack()->isCloseEnd()) {
        return;
    }

    if (al::isLessEqualStep(this, 16)) {
        mReturnRate = 1.0f;
        mReturnEndWait = 10;
        mRailMover->moveSyncRailByPos(this, 1.0f);
        mRailCameraAt = al::getRailPos(this);
        sead::Vector3f railDir = al::getRailDir(this);
        railDir.normalize();
        al::lerpVec(&mRailFront, mRailFront, railDir, 0.1f);
        mRailFront.normalize();

        // The game passes an uninitialized value as the target angle here.
        f32 targetAngleH;
        f32 angleH = al::lerpDegree(mRailAngleH, targetAngleH, 0.2f);
        f32 radH = sead::Mathf::deg2rad(angleH);
        mRailCameraPos.x = mRailCameraAt.x - sinf(radH) * mRailCameraDistance;
        mRailCameraPos.z = mRailCameraAt.z - cosf(radH) * mRailCameraDistance;

        f32 angleV = al::lerpDegree(
            mRailAngleV, sead::Mathf::rad2deg(atan2f(mRailFront.y, mRailFront.x)), 0.2f);
        mRailCameraPos.y =
            mRailCameraAt.y - sinf(sead::Mathf::deg2rad(angleV)) * mRailCameraDistance;
        return;
    }

    mRailCameraPos = mReturnPos;
    mRailCameraAt = mReturnAt;
    if (!al::isGreaterEqualStep(this, 30)) {
        return;
    }

    sceneLayout->getWipeBlack()->startOpen(sceneLayout->getWipeFrames());
    mSkipLayout->end();
    mSkipLayout->kill();
    al::tryOnStageSwitchInstant(this, "SwitchGoalItemAppearOn");
    if (mSceneLayout != nullptr) {
        mSceneLayout->setTimer(mTimeFrameCount);
        mSceneLayout->showTimer();
    }

    if (al::isActiveCamera(mRailCameraTicket)) {
        al::endCamera_RS(this, mRailCameraTicket, 0, false);
    }

    finishCameraMove();
}

/**
 * @brief Return the rail camera from the course end to the player.
 */
void TimerGate::exeRailMoveDone() {
    if (al::isFirstStep(this)) {
        if (!mIsSkipped) {
            mReturnRate = 0.0f;
            mReturnEndWait = 0;
            mReturnStartPos = mRailCameraPos;
            mReturnStartAt = mRailCameraAt;
        }

        mSkipLayout->end();
        mSkipLayout->kill();
        al::tryOnStageSwitchInstant(this, "SwitchGoalItemAppearOn");
    }

    s32 waitStep = mIsSkipped ? 30 : 90;
    if (!al::isGreaterEqualStep(this, waitStep)) {
        return;
    }

    if (al::isStep(this, waitStep)) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidon != nullptr) {
            raidon->toggleGameWindow(true);
        }

        if (mSceneLayout != nullptr) {
            mSceneLayout->setTimer(mTimeFrameCount);
            mSceneLayout->showTimer();
        }
    }

    if (!mIsSkipped) {
        mReturnRate += 1.0f;
        if (mReturnRate > 1.0f) {
            mReturnRate = 1.0f;
        }
    }

    f32 rate = mReturnRate;
    mRailCameraPos = mReturnStartPos * (1.0f - rate) + mReturnPos * rate;
    mRailCameraAt = mReturnStartAt * (1.0f - rate) + mReturnAt * rate;
    if (rate == 1.0f && (mIsSkipped || mReturnEndWait++ >= 10)) {
        if (al::isActiveCamera(mRailCameraTicket)) {
            al::endCamera_RS(this, mRailCameraTicket, -1, false);
        }

        finishCameraMove();
    }
}

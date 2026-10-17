#include "MapObj/Lighthouse.hpp"

#include "Camera/CameraLookAtPoint.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Camera/CameraPoserFix.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/CatGull.hpp"
#include "MapObj/CatGullHolder.hpp"
#include "MapObj/FlingPole.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/Fury/InkPatch.hpp"
#include "MapObj/Fury/InkPatchSpecial.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/SinkedItem.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(Lighthouse, Wait)
NERVE_DECL(Lighthouse, CameraFlagUp)
NERVE_DECL(Lighthouse, CameraReturn)
NERVE_DECL(Lighthouse, SkipWaitBowserExit)
NERVE_DECL(Lighthouse, WaitGigaBellDemo)
NERVE_DECL(Lighthouse, Phase0EndIdle)

/**
 * @brief Skip of the Fury Bowser exit started before the exit camera.
 */
class LighthouseNrvSkipBowserExit : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the lighthouse.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Lighthouse>()->exeSkipWaitBowserExit();
    }
};

NERVE_DECL(Lighthouse, DecideNormalState)
NERVE_DECL(Lighthouse, SkippedLighthouseLight)
NERVE_DECL(Lighthouse, WaitBowserExit)
NERVE_DECL(Lighthouse, CameraShowInkPatch)
NERVE_DECL(Lighthouse, Phase0End)
NERVE_DECL(Lighthouse, WaitPatchFadeOutForSpecialInkPatch)
NERVE_DECL(Lighthouse, CameraWaitSpecialInkPatch)
NERVE_DECL(Lighthouse, CameraInkFadeOut)
NERVE_DECL(Lighthouse, CameraRail)
NERVE_DECL(Lighthouse, WaitCameraSpecialInkPatchFadeOut)
NERVE_DECL(Lighthouse, SkippedFlagUp)
NERVE_DECL(Lighthouse, CameraOut)

/**
 * @brief Camera out after the return camera already reached its end.
 */
class LighthouseNrvCameraOutReturnEnd : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the lighthouse.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Lighthouse>()->exeCameraOut();
    }
};

NERVE_DECL(Lighthouse, CameraFinish)
NERVE_DECL(Lighthouse, GuideMessage)
NERVE_DECL(Lighthouse, WaitForGoalItemArrival)
NERVE_DECL(Lighthouse, LighthouseLight)

NERVES_MAKE_NOSTRUCT(Lighthouse, Wait, CameraFlagUp, CameraReturn, SkipWaitBowserExit,
                     WaitGigaBellDemo, Phase0EndIdle, SkipBowserExit, DecideNormalState,
                     SkippedLighthouseLight, WaitBowserExit, CameraShowInkPatch, Phase0End,
                     WaitPatchFadeOutForSpecialInkPatch, CameraWaitSpecialInkPatch,
                     CameraInkFadeOut, CameraRail, WaitCameraSpecialInkPatchFadeOut,
                     SkippedFlagUp, CameraOut, CameraOutReturnEnd, CameraFinish, GuideMessage,
                     WaitForGoalItemArrival, LighthouseLight)

/// Cat Shine effect of each flag state, as {normal, LOD} pairs.
const char* const sFlagEffectNames[3][2] = {
    {"Wait", "WaitLod01"},
    {"CatShinePartial", "CatShinePartialLod"},
    {"CatShineComplete", "CatShineCompleteLod"},
};

/// Squared horizontal camera distance from which the LOD effects are used.
constexpr f32 cLodDistanceSq = 12000000.0f;

typedef al::FunctorV0M<Lighthouse*, void (Lighthouse::*)()> LighthouseFunctor;

/**
 * @brief Get the controller ports allowed to skip a demo.
 * @return The main controller port as a port mask.
 */
inline sead::BitFlag16 getSkipPorts() {
    return sead::BitFlag16(1 << al::getMainControllerPort());
}

/**
 * @brief Get the demo skip layout of the scene.
 * @param pHolder The scene object holder.
 * @return The demo skip layout.
 */
inline DemoSkipLayout* getSkipLayout(const al::IUseSceneObjHolder* pHolder) {
    return al::tryGetSceneObj<GoalItemHolder>(pHolder, SceneObjID_GoalItemHolder)
        ->getSkipLayout();
}
}  // namespace

/**
 * @brief Construct the lighthouse.
 * @param pName The actor name.
 */
Lighthouse::Lighthouse(const char* pName)
    : al::LiveActor(pName), mPlacementId(new al::PlacementId()) {
    mInk = new al::LiveActor("InkedLighthouse");
    mFadeInDoneFunctor = new LighthouseFunctor(this, &Lighthouse::disasterModeFadeInDoneFunc);
}

/**
 * @brief Called once the screen faded back in after Fury Bowser was driven away.
 */
void Lighthouse::disasterModeFadeInDoneFunc() {
    auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (layout->getWipe()->isCloseEnd()) {
        layout->getWipe()->startOpen(0);
    }

    auto* controller = DisasterModeController::tryGetController(this);
    controller->getSuperBowser()->disappear(false);
    if (al::isNerve(this, &NrvLighthouseSkipWaitBowserExit)) {
        controller->getSuperBowser()->endBowserExitCamera(true);
    } else {
        controller->getSuperBowser()->tryDamageDarkBowser();
    }

    mGoalItem->lighthouseDarkBowserGone(true);
    al::tryDeleteEffectAndParticle(this, "CatShineShineBright");
    al::tryDeleteEffectAndParticle(this, "CatShineShineBrightInk");
    al::requestCaptureScreenCover(this, 3);

    auto* bellManager = GigaBellManager::tryGetManager(this);
    if (bellManager != nullptr && bellManager->tryQueueReturnCutscene(true, false, false)) {
        bellManager->tryQueueReturnCutscene(false, false, false);
        al::setNerve(this, &NrvLighthouseWaitGigaBellDemo);
        return;
    }

    setInkPatchOrNext();
}

/**
 * @brief Initialize the lighthouse, its ink, goal items, flag pole and cameras.
 * @param rInfo The actor init info.
 */
void Lighthouse::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    mUnlockedPhase = SingleModeDataFunction::getUnlockedPhase(this);
    al::initActorWithArchiveName(mInk, rInfo, "LighthouseInk", nullptr);

    if (!al::tryGetPlacementID(mPlacementId, rInfo)) {
        makeActorDead();
        mInk->makeActorDead();
        return;
    }

    s32 zoneNo = mPlacementHolder->getZoneNo();
    s32 islandId = zoneNo - 1;
    bool isActive =
        initActiveScenario(SingleModeDataFunction::getCurActiveScenarioIndex(this, islandId));

    s32 goalItemNum = al::calcLinkChildNum(rInfo, "GoalItem");
    if (goalItemNum >= 1) {
        mGoalItems.allocBuffer(goalItemNum, nullptr);
        for (s32 i = 0; i < goalItemNum; i++) {
            auto* goalItem = new GoalItem("GoalItem");
            al::initLinksActor(goalItem, rInfo, "GoalItem", i);
            goalItem->setLighthouse(this);
            mGoalItems.pushBack(goalItem);
            goalItem->setIslandId(zoneNo);
        }
    }

    if (al::calcLinkChildNum(rInfo, "FlingPole") == 1) {
        mFlingPole = new FlingPole("FlingPole", true);
        al::initLinksActor(mFlingPole, rInfo, "FlingPole", 0);
        mFlingPoleOffset = al::getTrans(mFlingPole) - al::getTrans(this);
        mFlingPole->makeActorDead();
    }

    activateScenarioAnim(false);
    if (isActive) {
        mFinishedMainNum = SingleModeDataFunction::getIslandActiveScenario(this, islandId);
    } else {
        mActiveScenario = mFinishedMainNum;
    }

    mCamera = al::initObjectCamera_RS(this, rInfo, nullptr);
    al::tryGetArg(&mFlagCameraInStep, rInfo, "FlagCameraInStep");
    al::tryGetArg(&mFlagCameraOutStep, rInfo, "FlagCameraOutStep");
    al::tryGetArg(&mFlagCameraHoldStep, rInfo, "FlagCameraHoldStep");
    mCameraReturnStep = 60;
    mCameraInterpoleStep = 0;

    al::CameraPoser_RS* poser = mCamera->getPoser();
    poser->setInterpoleStep(mCameraInterpoleStep);
    poser->setEndInterpoleStep(mCameraInterpoleStep);

    auto* lookAtInterpole = new LookAtPointInterpole();
    mLookAtInterpole = lookAtInterpole;
    lookAtInterpole->mActor = this;
    lookAtInterpole->captureCamera();
    lookAtInterpole->mTicket =
        al::initProgramableCamera_RS(this, rInfo, "LookAtInterpole", &lookAtInterpole->mPos,
                                     &lookAtInterpole->mAt, nullptr);
    lookAtInterpole->mTicket->getPoser()->setInterpoleStep(120);

    makeActorAppeared();
    al::initNerve(this, &NrvLighthouseWait, 0);
    mLayoutInitInfo = al::getLayoutInitInfo(rInfo);
}

/**
 * @brief Set up the ink and model for the island's active scenario.
 * @param scenario The active scenario index.
 * @return True if the island is not inked for this scenario.
 */
bool Lighthouse::initActiveScenario(s32 scenario) {
    s32 islandId = mPlacementHolder->getZoneNo() - 1;
    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, islandId);

    if (getFinshedTotalCount() == scenarioNum) {
        if (mFlingPole != nullptr) {
            mFlingPole->makeActorAppeared();
        }

        mIsInked = false;
        playEffect(FlagState_Complete);
        updateLod();
        return false;
    }

    bool isActive;
    if (SingleModeDataFunction::getFirstAvailableScenario(this, islandId) == scenario) {
        mInk->makeActorAppeared();
        if (mSinkedItem != nullptr && !al::isDead(mSinkedItem)) {
            mSinkedItem->makeActorDead();
        }

        showMainLighthouse(false);
        isActive = false;
        mIsInked = true;
    } else {
        mInk->makeActorDead();
        showMainLighthouse(true);
        isActive = true;
        mIsInked = false;
    }

    playEffect(FlagState_Ink);
    mActiveScenario = scenario;
    setBirdsAppeared();
    return isActive;
}

/**
 * @brief Count the finished scenarios and show the matching Cat Shine effect.
 * @param isActivate Whether to also turn the ink on.
 */
void Lighthouse::activateScenarioAnim(bool isActivate) {
    if (mFlagState == FlagState_Complete || mIsInkMeNot) {
        return;
    }

    s32 islandId = mPlacementHolder->getZoneNo() - 1;
    mFinishedMainNum = 0;
    mFinishedNum = 0;
    if (islandId >= 0) {
        s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, islandId);
        for (s32 i = 0; i < scenarioNum; i++) {
            if (SingleModeDataFunction::isScenarioComplete(this, islandId, i)) {
                mFinishedNum++;
                if (i < 3) {
                    mFinishedMainNum++;
                }
            }
        }
    }

    if (isActivate) {
        turnOnInk(mFinishedMainNum);
    }

    if (mFinishedNum > 4) {
        playEffect(FlagState_Complete);
    } else {
        playEffect(mIsInked ? FlagState_Ink : FlagState_Partial);
    }

    mIsScenarioAnimPending = false;
    updateLod();
}

/**
 * @brief Show the ink or lighthouse model and the flag pole for the current progress.
 */
void Lighthouse::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();

    s32 zoneNo = mPlacementHolder->getZoneNo();
    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, zoneNo - 1);
    setPhaseColor();

    s32 islandId = zoneNo - 1;
    s32 finishedNum = 0;
    if (islandId >= 0) {
        for (s32 i = 0; i < scenarioNum; i++) {
            finishedNum += SingleModeDataFunction::isScenarioComplete(this, islandId, i);
        }
    }

    mInkFinishedNum = finishedNum;
    if (mIsInked) {
        mInk->makeActorAppeared();
        setScenarioButtons(mInk);
        showMainLighthouse(false);
        playEffect(FlagState_Ink);
        if (mFlingPole != nullptr) {
            mFlingPole->makeActorDead();
        }

        if (mSinkedItem != nullptr) {
            mSinkedItem->makeActorDead();
        }
    } else {
        mInk->makeActorDead();
        if (finishedNum == scenarioNum) {
            if (mFlingPole != nullptr) {
                mFlingPole->makeActorAppeared();
            }

            showMainLighthouse(true);
            setScenarioButtons(this);
            mIsSinkedItemKillRequested = true;
            playEffect(FlagState_Complete);
        } else {
            if (al::isHideModel(this)) {
                al::showModel(this);
            }

            setScenarioButtons(this);
            if (mFlingPole != nullptr) {
                mFlingPole->makeActorDead();
            }

            playEffect(FlagState_Partial);
        }
    }

    if (mFlagState == FlagState_Complete) {
        if (mFlingPole != nullptr) {
            auto* controller = DisasterModeController::tryGetController(this);
            if (controller != nullptr && controller->isDisasterMode()) {
                al::tryStartSklAnimIfExist(mFlingPole, "LighthouseFlagFlapDisasterMode");
            } else {
                al::tryStartSklAnimIfExist(mFlingPole, "LighthouseFlagFlap");
            }

            al::tryStartMclAnimIfExist(mFlingPole, "LighthouseFlagWait");
        }
    } else if (mFlingPole != nullptr) {
        al::tryStartVisAnimIfExist(mFlingPole, "LighthouseFlagPoleHide");
        mFlingPole->showFlag(false);
    }

    mIsNoReturn = false;
    _1d2 = false;

    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        controller->registerStateListener(this);
    }
}

/**
 * @brief Color the lighthouse and its ink for the island's phase.
 */
void Lighthouse::setPhaseColor() {
    const char* animName = mIsDisasterPhase ? "PhaseColor_Disaster" : "PhaseColor";
    s32 zoneNo = mPlacementHolder->getZoneNo();
    f32 frame = zoneNo < 5 ? 0.0f : zoneNo < 9 ? 1.0f : 2.0f;

    if (al::tryStartMclAnimIfExist(this, animName)) {
        al::setMclAnimFrameAndStop(this, frame);
    }

    if (al::tryStartVisAnimIfExist(this, animName)) {
        al::setVisAnimFrameAndStop(this, frame);
    }

    if (al::tryStartMtpAnimIfExist(this, animName)) {
        al::setMtpAnimFrameAndStop(this, frame);
    }

    if (mInk != nullptr) {
        if (al::tryStartMclAnimIfExist(mInk, animName)) {
            al::setMclAnimFrameAndStop(mInk, frame);
        }

        if (al::tryStartVisAnimIfExist(mInk, animName)) {
            al::setVisAnimFrameAndStop(mInk, frame);
        }

        if (al::tryStartMtpAnimIfExist(mInk, animName)) {
            al::setMtpAnimFrameAndStop(mInk, frame);
        }
    }
}

/**
 * @brief Light the scenario buttons of every finished scenario.
 * @param pActor The lighthouse model (or the ink) to light the buttons of.
 */
void Lighthouse::setScenarioButtons(al::LiveActor* pActor) {
    s32 islandId = mPlacementHolder->getZoneNo() - 1;
    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, islandId);
    const char* buttonNames[] = {"GoalItem1_Toggle", "GoalItem2_Toggle", "GoalItem3_Toggle",
                                 "GoalItem4_Toggle", "GoalItem5_Toggle"};

    for (s32 i = 0; i < scenarioNum; i++) {
        if (SingleModeDataFunction::isScenarioComplete(this, islandId, i)) {
            al::startVisAnimAndSetFrameAndStop(pActor, buttonNames[i], 1.0f);
        }
    }
}

/**
 * @brief Show or hide the lighthouse model.
 * @param isShow Whether to show the model.
 */
void Lighthouse::showMainLighthouse(bool isShow) {
    if (isShow) {
        if (al::isHideModel(this)) {
            al::showModel(this);
        }

        al::validateCollisionParts(this);
        return;
    }

    if (!al::isHideModel(this)) {
        al::hideModel(this);
    }

    al::invalidateCollisionParts(this);
    al::tryStartSe(this, "InkFountain");
}

/**
 * @brief Switch the Cat Shine effect.
 * @param state The new flag state.
 */
void Lighthouse::playEffect(FlagState state) {
    if (mFlagState == state) {
        return;
    }

    if (mFlagState >= FlagState_Ink) {
        al::tryDeleteEffect(this, sFlagEffectNames[mFlagState][0]);
        al::tryDeleteEffect(this, sFlagEffectNames[mFlagState][1]);
    }

    mPrevFlagState = mFlagState;
    mFlagState = state;
    if (isLod()) {
        al::tryEmitEffect(this, sFlagEffectNames[mFlagState][1], nullptr);
        al::tryDeleteEffect(this, sFlagEffectNames[mFlagState][0]);
    } else {
        al::tryEmitEffect(this, sFlagEffectNames[mFlagState][0], nullptr);
        al::tryDeleteEffect(this, sFlagEffectNames[mFlagState][1]);
    }
}

/**
 * @brief Set the camera points used by the rail camera.
 * @param pPoint The camera points.
 * @param moveStep Frames taken to move to the first point.
 */
void Lighthouse::setupCameraLookAtPoint(CameraLookAtPoint* pPoint, s32 moveStep) {
    mLookAtInterpole->setup(pPoint, moveStep);
    mIsLookAtPointSet = true;
}

/**
 * @brief Notify the goal item once the lighthouse light shone.
 */
void Lighthouse::setLighthouseShone() {
    if (mIsLighthouseShone) {
        return;
    }

    mIsLighthouseShone = true;
    mGoalItem->lighthouseLightShone();
}

/**
 * @brief Show the ink patch removed by this Cat Shine, or move on with the cutscene.
 */
void Lighthouse::setInkPatchOrNext() {
    if (mInkPatch != nullptr) {
        handleInkPatch();
        return;
    }

    if (mFinishedNum == 5) {
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvLighthouseCameraFlagUp);
        return;
    }

    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    if (!al::isActiveCamera(mCamera)) {
        al::startCamera_RS(this, mCamera, -1);
    }

    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    al::requestCaptureScreenCover(this, 4);
    if (mIsReturnSetDir) {
        mLandedPos = al::getTrans(mLandedActor);
        startReturnSetDir(poser, mFlagReturnStep);
    } else {
        poser->setReturn(this, 0, false);
    }

    al::setNerve(this, &NrvLighthouseCameraReturn);
}

/**
 * @brief Start the camera showing the ink patch.
 * @return Always true.
 */
bool Lighthouse::handleInkPatch() {
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    if (mInkPatch != nullptr) {
        mInkPatch->startCamera();
        rc::addDemoActor(mInkPatch);
        mCamera->getPoser<al::CameraPoserFixActor>()->setTargetActor(mInkPatch);
    }

    al::requestCaptureScreenCover(this, 3);
    al::setNerve(this, &NrvLighthouseCameraShowInkPatch);
    return true;
}

/**
 * @brief Update the LOD effects every frame.
 */
void Lighthouse::control() {
    updateLod();
}

/**
 * @brief Push the player away from the lighthouse.
 * @param pSelf The sensor of the lighthouse.
 * @param pOther The other sensor.
 */
void Lighthouse::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgEnemyAttack(pOther, pSelf);
    }
}

/**
 * @brief Move the lighthouse together with its ink, flag pole and sunken item.
 * @param rTrans The new translation.
 */
void Lighthouse::updateLinkedTrans(const sead::Vector3f& rTrans) {
    al::LiveActor::updateLinkedTrans(rTrans);
    if (mInk != nullptr) {
        mInk->updateLinkedTrans(rTrans);
    }

    if (mFlingPole != nullptr) {
        mFlingPole->updateLinkedTrans(rTrans + mFlingPoleOffset);
    }

    if (mSinkedItem != nullptr) {
        mSinkedItem->updateLinkedTrans(rTrans + mSinkedItemOffset);
    }
}

/**
 * @brief Switch the phase colors when disaster mode starts or ends.
 * @param state The new disaster mode state.
 */
void Lighthouse::onDisasterModeStateChange(DisasterModeController::State state) {
    switch (static_cast<s32>(state)) {
    case 1:
    case 3:
        if (!mIsDisasterPhase) {
            return;
        }

        mIsDisasterPhase = false;
        setPhaseColor();
        if (!mIsInked && al::isNerve(this, &NrvLighthouseWait)) {
            al::tryStartMclAnimIfExist(this, "GlassBlink");
        }

        return;
    case 7:
    case 9:
        if (mIsDisasterPhase) {
            return;
        }

        mIsDisasterPhase = true;
        setPhaseColor();
        if (!mIsInked && al::isNerve(this, &NrvLighthouseWait)) {
            al::tryStartMclAnimIfExist(this, "GlassBlink_Disaster");
        }

        return;
    default:
        return;
    }
}

/**
 * @brief Stop the ink pillar.
 */
void Lighthouse::killInkPillar() {
    if (!al::isAlive(mInk)) {
        return;
    }

    mIsInkPillarKilled = true;
    al::tryStartMclAnimIfExist(mInk, "LighthouseInkOff");
    al::tryDeleteEmitterAndParticleAll(this);
}

/**
 * @brief Restart the ink pillar.
 */
void Lighthouse::startInkPillar() {
    if (!al::isAlive(mInk)) {
        return;
    }

    mIsInkPillarKilled = false;
    al::tryStartMclAnimIfExist(mInk, "LighthouseInkOn");
    updateLod();
    mFlagState = FlagState_None;
    playEffect(FlagState_Ink);
}

/**
 * @brief Check whether the camera is far enough for the LOD effects.
 * @return True if the LOD effects should be used.
 */
bool Lighthouse::isLod() const {
    if (al::isExpandedClippingMode(this)) {
        return false;
    }

    const sead::Vector3f& cameraPos = al::getCameraPos_RS(this, 0);
    const sead::Vector3f& trans = al::getTrans(this);
    f32 x = cameraPos.x - trans.x;
    f32 z = cameraPos.z - trans.z;
    return x * x + z * z > cLodDistanceSq;
}

/**
 * @brief Switch between the normal and LOD Cat Shine effects.
 */
void Lighthouse::updateLod() {
    if (mIsInkMeNot) {
        return;
    }

    bool isPrevLod = mIsLodEffect[mPrevFlagState];
    mIsLodEffect[mFlagState] = isLod();

    FlagState state = mFlagState;
    bool isCurLod = mIsLodEffect[state];
    if (mPrevFlagState == state) {
        if (isPrevLod != isCurLod) {
            al::tryDeleteEffect(this, sFlagEffectNames[state][isPrevLod]);
            al::tryEmitEffect(this, sFlagEffectNames[mPrevFlagState][isCurLod], nullptr);
        }

        return;
    }

    if (mPrevFlagState != FlagState_None &&
        sFlagEffectNames[mPrevFlagState][isPrevLod] != nullptr) {
        al::tryDeleteEffect(this, sFlagEffectNames[mPrevFlagState][0]);
        al::tryDeleteEffect(this, sFlagEffectNames[mPrevFlagState][1]);
    }

    const char* effectName = sFlagEffectNames[mFlagState][isCurLod];
    if (effectName != nullptr && !mIsInkPillarKilled) {
        al::tryEmitEffect(this, effectName, nullptr);
    }

    mPrevFlagState = mFlagState;
}

/**
 * @brief Show the partial or complete Cat Shine effect.
 */
void Lighthouse::handleFlagState() {
    if (mFinishedNum == 5) {
        mIsSinkedItemKillRequested = true;
        killSinkedItem();
        playEffect(FlagState_Complete);
    } else {
        playEffect(FlagState_Partial);
    }
}

/**
 * @brief Kill the sunken item.
 */
void Lighthouse::killSinkedItem() {
    if (mSinkedItem != nullptr) {
        mSinkedItem->kill();
        mSinkedItem = nullptr;
    }
}

/**
 * @brief Remove the ink of the lighthouse.
 */
ALWAYS_INLINE inline void Lighthouse::clearInk() {
    if (mIsInked) {
        al::clearMclAnim(this);
        mInk->makeActorDead();
        showMainLighthouse(true);
        al::tryDeleteEmitterAndParticleAll(mInk);
        mIsInked = false;
        setBirdsAppeared();
    }

    al::stopAllSeId(this, "InkFountain", 0, nullptr);
    al::tryDeleteEffect(this, "Wait");
    al::tryDeleteEffect(this, "WaitLod01");
}

/**
 * @brief Jump to the end of the lighthouse light sequence.
 * @return True if the sequence continues with Fury Bowser leaving.
 */
bool Lighthouse::jumpToFinish() {
    alPadRumbleFunction::stopPadRumbleLoop(this, "GigaDrop", al::getTransPtr(this), -1);
    alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
    al::cancelCameraShake(this);
    mIsJumpedToFinish = true;
    mIsFinishBlocked = false;
    handleFlagState();
    setScenarioButtons(this);
    clearInk();

    if (mUnlockedPhase == 0) {
        SingleModeDataFunction::setPhaseEnd(this, true);
        al::setNerve(this, &NrvLighthousePhase0EndIdle);
        return true;
    }

    setLighthouseShone();
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        SuperBowser* superBowser = controller->getSuperBowser();
        bool isLastBowser = superBowser != nullptr ? superBowser->isLastPhase3Bowser() : false;
        if (!(controller->isSuperHardMode() || isLastBowser || !controller->isDisasterMode())) {
            al::setNerve(this, &NrvLighthouseSkipBowserExit);
            return true;
        }
    }

    al::setNerve(this, &NrvLighthouseDecideNormalState);
    return false;
}

/**
 * @brief Make the island's seagulls fly back, or hide them while the island is inked.
 */
void Lighthouse::setBirdsAppeared() {
    s32 zoneNo = mPlacementHolder->getZoneNo();
    auto* holder = al::tryGetSceneObj<CatGullHolder>(this, SceneObjID_CatGullHolder);
    for (s32 i = 0; i < holder->getCatGulls().size(); i++) {
        CatGull* catGull = holder->getCatGulls().at(i);
        if (catGull->getIslandId() != zoneNo) {
            continue;
        }

        if (mIsInked) {
            al::hideModelIfShow(catGull);
        } else {
            catGull->startFlyReturn();
        }
    }
}

/**
 * @brief Wait for the Cat Shine to fly to the lighthouse.
 */
void Lighthouse::exeWaitForGoalItemArrival() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (al::isFirstStep(this)) {
        if (!mIsPhaseOffsetSet && mCameraArea != nullptr) {
            al::CameraPoser_RS* poser =
                getCameraDirector_RS()->findCameraAreaTicket(mCameraArea)->getPoser();
            if (al::isEqualString(poser->getName(), "Fixed")) {
                static_cast<al::CameraPoserFix*>(poser)->setPhaseOffsets(mHeightOffset,
                                                                         mDistanceOffset);
                mIsPhaseOffsetSet = true;
            }
        }

        if (controller != nullptr) {
            controller->pause(false);
        }

        return;
    }

    if (al::isStep(this, 1) && controller != nullptr) {
        controller->forceKillSuperBowserAttacks();
    }
}

/**
 * @brief Light the lighthouse up with the arrived Cat Shine.
 */
void Lighthouse::exeLighthouseLight() {
    if (al::isFirstStep(this) && mIsInked) {
        setScenarioButtons(this);
    }

    if (getSkipLayout(this)->isSkip(getSkipPorts())) {
        al::setNerve(this, &NrvLighthouseSkippedLighthouseLight);
        return;
    }

    mGoalItem->tryTriggerGoalItemLightStart(al::getNerveStep(this));
    if (mIsInked) {
        if (al::isStep(this, 213)) {
            al::startCameraShakeByAction(this, "最強", "", -1, 0);
        }
    } else if (al::isStep(this, 183)) {
        setScenarioButtons(this);
        al::startCameraShakeByAction(this, "最強", "", -1, 0);
    }

    if (al::isStep(this, 270)) {
        alPadRumbleFunction::stopPadRumbleLoop(this, "GigaDrop", al::getTransPtr(this), -1);
        al::cancelCameraShake(this);
    }

    if (al::isStep(this, 213)) {
        alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
        if (mIsInked) {
            alPadRumbleFunction::startPadRumbleLoopNo3D(this, "GigaDrop", al::getTransPtr(this),
                                                        -1, false);
            clearInk();
        }
    }

    if (mIsInked) {
        if (al::isStep(this, 200)) {
            const char* seNames[] = {"InkReveal", "InkReveal2", "InkReveal3", "InkReveal4",
                                     "InkReveal5"};
            al::tryStartSe(this, seNames[mCurrentScenarioId]);
            tryStartGlassBlinkAnim();
        }
    } else if (al::isStep(this, 180)) {
        al::tryStartSe(this, "LighthouseShineArriveNoInk");
    }

    if (al::isStep(this, 1)) {
        alPadRumbleFunction::startPadRumbleLoopNo3D(this, "SuperShell", al::getTransPtr(this),
                                                    -1, false);
        al::tryEmitEffect(this,
                          mFlagState != FlagState_Ink ? "CatShineShineBright" :
                                                        "CatShineShineBrightInk",
                          nullptr);
    }

    if (!al::isGreaterEqualStep(this, 330)) {
        return;
    }

    setLighthouseShone();
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        SuperBowser* superBowser = controller->getSuperBowser();
        bool isLastBowser = superBowser != nullptr ? superBowser->isLastPhase3Bowser() : false;
        if (!(controller->isSuperHardMode() || isLastBowser || !controller->isDisasterMode())) {
            if (mIsJumpedToFinish || controller->isSuperBowserLeaving()) {
                al::setNerve(this, &NrvLighthouseSkipBowserExit);
            } else {
                al::setNerve(this, &NrvLighthouseWaitBowserExit);
            }

            return;
        }
    }

    al::setNerve(this, &NrvLighthouseDecideNormalState);
}

/**
 * @brief Start the glass blink animation for the current phase.
 */
void Lighthouse::tryStartGlassBlinkAnim() {
    if (mIsDisasterPhase) {
        al::tryStartMclAnimIfExist(this, "GlassBlink_Disaster");
    } else {
        al::tryStartMclAnimIfExist(this, "GlassBlink");
    }
}

/**
 * @brief Fade out and jump to the end of the skipped light sequence.
 */
void Lighthouse::exeSkippedLighthouseLight() {
    auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (al::isFirstStep(this)) {
        mGoalItem->lighthouseLightSequenceSkipped();
        alPadRumbleFunction::stopPadRumbleLoop(this, "GigaDrop", al::getTransPtr(this), -1);
        alPadRumbleFunction::stopPadRumbleLoop(this, "SuperShell", al::getTransPtr(this), -1);
        layout->getWipe()->startClose(layout->getWipeFrames());
        return;
    }

    if (!layout->getWipe()->isCloseEnd()) {
        return;
    }

    setScenarioButtons(this);
    tryStartGlassBlinkAnim();
    if (jumpToFinish()) {
        return;
    }

    layout->getWipe()->startOpen(layout->getWipeFrames());
}

/**
 * @brief Decide how the cutscene continues when Fury Bowser does not leave.
 */
void Lighthouse::exeDecideNormalState() {
    al::isFirstStep(this);
    if (al::isStep(this, 0)) {
        handleFlagState();
    }

    if (al::isGreaterEqualStep(this, 0) && !mIsFinishBlocked) {
        al::tryDeleteEffect(this, "CatShineShineBright");
        al::tryDeleteEffect(this, "CatShineShineBrightInk");
        setInkPatchOrNext();
    }
}

/**
 * @brief Wait for Fury Bowser to leave the island.
 */
void Lighthouse::exeWaitBowserExit() {
    auto* controller = DisasterModeController::tryGetController(this);
    SuperBowser* superBowser = controller->getSuperBowser();
    if (al::isStep(this, 0)) {
        handleFlagState();
    }

    if (al::isStep(this, 0)) {
        al::requestCaptureScreenCover(this, 3);
        setLighthouseShone();
        if (mUnlockedPhase != 0) {
            if (al::isActiveCamera(mCamera)) {
                al::endCamera_RS(this, mCamera, -1, false);
            }

            const sead::Vector3f& trans = al::getTrans(this);
            const sead::Vector3f& bowserTrans = al::getTrans(superBowser);
            sead::Vector3f dir(trans.x - bowserTrans.x, 0.0f, trans.z - bowserTrans.z);
            dir.normalize();
            superBowser->setFacingDirection(dir);
            superBowser->startBowserExitCamera(false, false, true);
        } else {
            al::setFixedActor(mCamera, superBowser, 13000.0f, -24.0f, 5.0f, nullptr);
            al::setFixActorCameraOffset(mCamera, {1200.0f, 3500.0f, 0.0f});
            if (!al::isActiveCamera(mCamera)) {
                al::startCamera_RS(this, mCamera, -1);
            }

            const sead::Vector3f& trans = al::getTrans(this);
            const sead::Vector3f& bowserTrans = al::getTrans(superBowser);
            sead::Vector3f dir(trans.x - bowserTrans.x, 0.0f, trans.z - bowserTrans.z);
            dir.normalize();
            sead::Quatf quat = sead::Quatf::unit;
            al::makeQuatFrontUp(&quat, dir, sead::Vector3f::ey);
            al::setQuat(superBowser, quat);
            rc::addDemoActor(superBowser);
        }
    }

    auto* goalItemHolder = al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
    bool isSkip = false;
    if (al::isGreaterEqualStep(this, 0) && controller->isDisasterMode() &&
        goalItemHolder->getSkipLayout()->isSkip(getSkipPorts())) {
        goalItemHolder->getSkipLayout()->end();
        isSkip = true;
    }

    if (isSkip || al::isGreaterEqualStep(this, 330)) {
        if (mUnlockedPhase == 0) {
            if (isSkip || controller->isBowserHidden()) {
                al::requestCaptureScreenCover(this, 3);
                al::setNerve(this, &NrvLighthousePhase0End);
            }

            return;
        }

        if (isSkip) {
            al::setNerve(this, &NrvLighthouseSkipWaitBowserExit);
            return;
        }
    }

    if (!controller->isBowserHidden()) {
        return;
    }

    mGoalItem->lighthouseDarkBowserGone(false);
    al::requestCaptureScreenCover(this, 3);
    auto* bellManager = GigaBellManager::tryGetManager(this);
    if (mUnlockedPhase != 0) {
        controller->getSuperBowser()->endBowserExitCamera(true);
    }

    al::tryDeleteEffectAndParticle(this, "CatShineShineBright");
    al::tryDeleteEffectAndParticle(this, "CatShineShineBrightInk");
    goalItemHolder->getSkipLayout()->end();
    if (bellManager != nullptr && bellManager->tryQueueReturnCutscene(true, false, false)) {
        al::requestCaptureScreenCover(this, 5);
        bellManager->tryQueueReturnCutscene(false, false, false);
        al::setNerve(this, &NrvLighthouseWaitGigaBellDemo);
        return;
    }

    setInkPatchOrNext();
}

/**
 * @brief Wait for the Giga Bell return cutscene to end.
 */
void Lighthouse::exeWaitGigaBellDemo() {
    if (GigaBellManager::tryGetManager(this)->isCutsceneDone()) {
        al::requestCaptureScreenCover(this, 3);
        setInkPatchOrNext();
    }
}

/**
 * @brief End disaster mode right away after a skip.
 */
void Lighthouse::exeSkipWaitBowserExit() {
    auto* controller = DisasterModeController::tryGetController(this);
    if (al::isFirstStep(this)) {
        setLighthouseShone();
        getSkipLayout(this)->end();
        controller->setFadeInDoneFunctor(mFadeInDoneFunctor);
        al::isNerve(this, &NrvLighthouseSkipWaitBowserExit);
        controller->endInstantly(true, false);
    }
}

/**
 * @brief Set up the return camera, aiming its direction at the landing spot.
 * @param pPoser The return camera poser.
 * @param step The return step count.
 */
inline void Lighthouse::startReturnSetDir(al::CameraPoserFixActor* pPoser, s32 step) {
    mLandedPos.y += mReturnOffsetY;
    pPoser->setReturnSetDir(step, al::getCameraPos_RS(this, 0), mLandedPos,
                            al::getCameraAt_RS(this, 0),
                            al::getCameraPos_RS(this, 0) - al::getCameraAt_RS(this, 0),
                            mReturnAngleH, mReturnAngleV, mReturnDirRate, mReturnDistanceRate,
                            mReturnDistance);
}

/**
 * @brief Show the ink patch disappearing.
 */
void Lighthouse::exeCameraShowInkPatch() {
    if (al::isFirstStep(this)) {
        mIsSkipped = false;
        getSkipLayout(this)->startHidden();
    }

    if (al::isStep(this, mInkPatchDisappearStep)) {
        mIsInkPatchKeepModel = mInkPatch->_142;
        mInkPatch->_142 = true;
        mInkPatch->disappear();
    }

    if (getSkipLayout(this)->isSkip(getSkipPorts())) {
        if (mInkLinkId >= 0) {
            mInkPatch->forceDisappear();
        }

        mIsSkipped = true;
    }

    if (mInkPatch->isDone() && al::isGreaterStep(this, mInkPatchWaitStep)) {
        if (!mIsSkipped) {
            mInkPatch->endCamera();
            if (mCameraArea != nullptr) {
                mCameraArea->invalidate();
            }
        }
    } else if (!al::isGreaterStep(this, 90) || !mIsSkipped) {
        return;
    }

    auto* inkPatchSpecial = InkPatchSpecial::tryGetInkPatchSpecial(this);
    if (inkPatchSpecial != nullptr && inkPatchSpecial->isLastUnlockerInkPatch(mInkPatch)) {
        if (mIsSkipped) {
            al::setNerve(this, &NrvLighthouseWaitPatchFadeOutForSpecialInkPatch);
        } else {
            mInkPatch->triggerSpecialInkPatch();
            al::setNerve(this, &NrvLighthouseCameraWaitSpecialInkPatch);
        }

        return;
    }

    if (mFinishedNum == 5) {
        al::requestCaptureScreenCover(this, 4);
        if (!al::isActiveCamera(mCamera)) {
            al::startCamera_RS(this, mCamera, -1);
        }

        al::setNerve(this, &NrvLighthouseCameraFlagUp);
        return;
    }

    if (mIsSkipped) {
        auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
        layout->getWipe()->startClose(layout->getWipeFrames());
        al::setNerve(this, &NrvLighthouseCameraInkFadeOut);
        mInkPatch->_142 = mIsInkPatchKeepModel;
        return;
    }

    al::requestCaptureScreenCover(this, 4);
    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    bool isLookAtPointSet = mIsLookAtPointSet;
    bool isActive = al::isActiveCamera(mCamera);
    if (isLookAtPointSet) {
        if (isActive) {
            al::endCamera_RS(this, mCamera, -1, false);
        }

        mLookAtInterpole->start();
        mInkPatch->_142 = mIsInkPatchKeepModel;
        al::setNerve(this, &NrvLighthouseCameraRail);
        return;
    }

    if (!isActive) {
        al::startCamera_RS(this, mCamera, -1);
    }

    if (mIsReturnSetDir) {
        startReturnSetDir(poser, mReturnStep > 0 ? mReturnStep : 90);
    } else if (!mIsNoReturn) {
        poser->setReturn(this, mIsReturnByStep && !mIsSkipped ? mReturnStep : 0, false);
    }

    mInkPatch->_142 = mIsInkPatchKeepModel;
    al::setNerve(this, &NrvLighthouseCameraReturn);
}

/**
 * @brief Fade out before the special ink patch after a skip.
 */
void Lighthouse::exeWaitPatchFadeOutForSpecialInkPatch() {
    auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (al::isFirstStep(this)) {
        layout->getWipe()->startClose(layout->getWipeFrames());
        return;
    }

    if (!layout->getWipe()->isCloseEnd()) {
        return;
    }

    layout->getWipe()->startOpen(layout->getWipeFrames());
    mInkPatch->endCamera();
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    mInkPatch->triggerSpecialInkPatch();
    al::setNerve(this, &NrvLighthouseCameraWaitSpecialInkPatch);
}

/**
 * @brief Return the camera once the skipped ink patch faded out.
 */
void Lighthouse::exeCameraInkFadeOut() {
    al::isFirstStep(this);
    if (!al::isGreaterEqualStep(this, 12)) {
        return;
    }

    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    mInkPatch->forceDisappear();
    mIsInkFadedOut = true;
    mInkPatch->endCamera();
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    if (!al::isActiveCamera(mCamera)) {
        al::startCamera_RS(this, mCamera, -1);
    }

    if (mIsReturnSetDir) {
        startReturnSetDir(poser, 0);
    } else if (!mIsNoReturn) {
        poser->setReturn(this, 0, false);
    }

    al::setNerve(this, &NrvLighthouseCameraReturn);
}

/**
 * @brief Wait for the special ink patch to finish.
 */
void Lighthouse::exeCameraWaitSpecialInkPatch() {
    if (al::isFirstStep(this)) {
        mIsSkipped = false;
        getSkipLayout(this)->startHidden();
    }

    if (getSkipLayout(this)->isSkip(getSkipPorts())) {
        mInkPatch->forceDisappear();
        mIsSkipped = true;
    }

    if (!mInkPatch->isSpecialInkPatchDone()) {
        return;
    }

    if (al::isGreaterStep(this, mInkPatchWaitStep) && !mIsSkipped) {
        cleanUpWaitSpecialInkPatch();
    } else if (mIsSkipped) {
        al::setNerve(this, &NrvLighthouseWaitCameraSpecialInkPatchFadeOut);
    }
}

/**
 * @brief End the special ink patch and return the camera.
 */
void Lighthouse::cleanUpWaitSpecialInkPatch() {
    mInkPatch->endSpecialInkPatch(mIsInkPatchKeepModel);
    if (!al::isActiveCamera(mCamera)) {
        al::startCamera_RS(this, mCamera, -1);
    }

    if (mFinishedNum == 5) {
        al::setNerve(this, &NrvLighthouseCameraFlagUp);
        return;
    }

    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    if (mIsReturnSetDir) {
        startReturnSetDir(poser, 0);
    } else if (!mIsNoReturn) {
        poser->setReturn(this, mIsReturnByStep ? mReturnStep : 0, false);
    }

    mInkPatch->_142 = mIsInkPatchKeepModel;
    al::setNerve(this, &NrvLighthouseCameraReturn);
}

/**
 * @brief Fade out before ending the skipped special ink patch.
 */
void Lighthouse::exeWaitCameraSpecialInkPatchFadeOut() {
    auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (al::isFirstStep(this)) {
        layout->getWipe()->startClose(layout->getWipeFrames());
        return;
    }

    if (layout->getWipe()->isCloseEnd()) {
        layout->getWipe()->startOpen(layout->getWipeFrames());
        cleanUpWaitSpecialInkPatch();
    }
}

/**
 * @brief Raise the flag on the flag pole.
 */
void Lighthouse::exeCameraFlagUp() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEffectAndParticle(this, "CatShineShineBright");
        al::tryDeleteEffectAndParticle(this, "CatShineShineBrightInk");
        if (!al::isActiveCamera(mCamera)) {
            al::startCamera_RS(this, mCamera, -1);
        }

        auto* goalItemHolder =
            al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
        if (!goalItemHolder->getSkipLayout()->isAlive()) {
            goalItemHolder->getSkipLayout()->startHidden();
        }

        al::setFixActorCameraTarget(mCamera, this);
        al::setFixActorCameraOffset(mCamera, {0.0f, 2100.0f, 0.0f});
        al::setFixActorCameraDistance(mCamera, 3300.0f);
        al::setFixActorCameraAngleH(mCamera, 234.0f);
        al::setFixActorCameraAngleV(mCamera, 31.0f);
        mCamera->getPoser<al::CameraPoserFixActor>()->mIsReturnEnd = false;
        if (mFlingPole != nullptr) {
            mFlingPole->makeActorAppeared();
            rc::addDemoActor(mFlingPole);
            al::tryStartSklAnimIfExist(mFlingPole, "LighthouseRise");
            al::tryStartVisAnimIfExist(mFlingPole, "LighthouseFlagPoleShow");
            mFlingPole->showFlag(true);
        }

        al::tryStartSe(this, "FlagPoleAppear");
        al::changeBgmSituation(this, "VolumeHalf");
        mIsFlagAppeared = false;
    } else if (mIsFlagAppeared) {
        if (getSkipLayout(this)->isSkip(getSkipPorts())) {
            al::setNerve(this, &NrvLighthouseSkippedFlagUp);
            return;
        }

        if (al::isGreaterStep(this, 180)) {
            cleanUpCameraFlag();
        }

        return;
    }

    if (mFlingPole == nullptr || !al::isSklAnimEnd(mFlingPole, 0)) {
        return;
    }

    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr && controller->isDisasterMode()) {
        al::tryStartSklAnimIfExist(mFlingPole, "LighthouseFlagFlapDisasterMode");
    } else {
        al::tryStartSklAnimIfExist(mFlingPole, "LighthouseFlagFlap");
    }

    al::tryStartSe(this, "FlagAppear");
    mIsFlagAppeared = true;
}

/**
 * @brief End the flag camera and return the camera.
 */
void Lighthouse::cleanUpCameraFlag() {
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    getSkipLayout(this)->end();
    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    al::changeBgmSituation(this, "VolumeUp");
    if (mIsReturnSetDir) {
        mLandedPos = al::getTrans(mLandedActor);
        startReturnSetDir(poser, mFlagReturnStep);
    } else if (!mIsNoReturn) {
        poser->setReturn(this, 0, false);
    }

    al::setNerve(this, &NrvLighthouseCameraReturn);
}

/**
 * @brief Fade out before ending the skipped flag camera.
 */
void Lighthouse::exeSkippedFlagUp() {
    auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (al::isFirstStep(this)) {
        layout->getWipe()->startClose(layout->getWipeFrames());
        return;
    }

    if (layout->getWipe()->isCloseEnd()) {
        layout->getWipe()->startOpen(layout->getWipeFrames());
        cleanUpCameraFlag();
    }
}

/**
 * @brief Fade out and end the first phase.
 */
void Lighthouse::exePhase0End() {
    auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
    if (al::isFirstStep(this)) {
        layout->getWipe()->startClose(layout->getWipeFrames());
        return;
    }

    if (layout->getWipe()->isCloseEnd()) {
        SingleModeDataFunction::setPhaseEnd(this, true);
    }
}

/**
 * @brief Idle once the first phase ended.
 */
void Lighthouse::exePhase0EndIdle() {}

/**
 * @brief Play the look-at camera along the camera points.
 */
void Lighthouse::exeCameraRail() {
    al::isFirstStep(this);
    rc::invalidatePlayerInput(this, 2);
    LookAtPointInterpole* lookAtInterpole = mLookAtInterpole;
    lookAtInterpole->update();
    if (!lookAtInterpole->isEnd()) {
        return;
    }

    mLookAtInterpole->end();
    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    al::startCamera_RS(this, mCamera, -1);
    if (mIsReturnSetDir) {
        mLandedPos = al::getTrans(mLandedActor);
        startReturnSetDir(poser, mReturnStep);
    } else if (!mIsNoReturn) {
        poser->setReturn(this, mIsReturnByStep ? mReturnStep : 0, false);
    }

    mInkPatch->_142 = mIsInkPatchKeepModel;
    al::setNerve(this, &NrvLighthouseCameraReturn);
}

/**
 * @brief Return the camera to the player.
 */
void Lighthouse::exeCameraReturn() {
    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    sead::Vector3f target = al::getTrans(mLandedActor);
    target.y += mReturnOffsetY;
    poser->updateReturnCamTarget(target);
    rc::invalidatePlayerInput(this, 2);
    if (al::isFirstStep(this)) {
        getSkipLayout(this)->end();
        al::tryDeleteEffect(this, "CatShineShineBright");
        al::tryDeleteEffect(this, "CatShineShineBrightInk");
    }

    if (mIsSkipped && mIsInkFadedOut) {
        if (al::isFirstStep(this)) {
            mGoalItem->lighthouseSequenceAlmostEnded();
        }

        if (al::isGreaterEqualStep(this, 20)) {
            auto* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);
            if (layout->getWipe()->isCloseEnd()) {
                layout->getWipe()->startOpen(layout->getWipeFrames());
            }

            al::setNerve(this, &NrvLighthouseCameraOut);
            mCamera->getPoser()->setEndInterpoleStep(0);
            al::endCamera_RS(this, mCamera, -1, false);
        }

        return;
    }

    if (!al::isActiveCamera(mCamera)) {
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvLighthouseCameraOut);
        return;
    }

    if (mCamera->getPoser<al::CameraPoserFixActor>()->mIsReturnEnd) {
        al::setNerve(this, &NrvLighthouseCameraOutReturnEnd);
    } else if (mIsNoReturn) {
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvLighthouseCameraOut);
    } else {
        return;
    }

    mCamera->getPoser()->setEndInterpoleStep(0);
    al::endCamera_RS(this, mCamera, mReturnEndStep, false);
}

/**
 * @brief End the goal item cutscene and go back to waiting.
 */
inline void Lighthouse::finishGoalItemCutscene() {
    al::validateClipping(this);
    if (mFlingPole != nullptr) {
        al::validateClipping(mFlingPole);
    }

    mGlobalAlphaLastFrame = 1.0f;
    mGoalItem->finishGoalItemCutscene();
    mGoalItem = nullptr;
    al::setNerve(this, &NrvLighthouseWait);
}

/**
 * @brief Show the guide message of the next Cat Shine.
 */
void Lighthouse::exeGuideMessage() {
    rc::invalidatePlayerInput(this, 2);
    if (!al::isGreaterEqualStep(this, 1)) {
        return;
    }

    if (al::isStep(this, 1) && mGuideMessage != nullptr) {
        rc::appearGuideGameWindowWithConfirm(this, mGuideMessage, true);
        if (mUnlockedPhase == 5 && SingleModeDataFunction::isFirstPhase3BossDefeated(this)) {
            SingleModeDataFunction::setGuideMessageSeen(this, 15, true);
        }
    }

    if (rc::isGuideGameWindowWaitConfirm(this)) {
        return;
    }

    finishGoalItemCutscene();
    rc::unHideGuideGameWindow(this);
    auto* goalItemHolder = al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
    if (goalItemHolder != nullptr) {
        goalItemHolder->appearWindowProcessing();
    }

    auto* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return;
    }

    controller->resume(true);
    if (SingleModeDataFunction::getGoalItemsCollected(this) ==
        SingleModeDataFunction::getMaxCollectableGoalItems()) {
        controller->tryJumpToRainWithFlash();
    }
}

/**
 * @brief End the lighthouse sequence and wait before finishing the camera.
 */
void Lighthouse::exeCameraOut() {
    if (al::isFirstStep(this)) {
        mGoalItem->lighthouseSequenceCompletelyEnded(
            al::isNerve(this, &NrvLighthouseCameraOutReturnEnd));
        al::tryOnStageSwitch(this, "SwitchDemoEndAnimateOn");
    }

    rc::invalidatePlayerInput(this, 2);
    auto* goalItemHolder = al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
    if (goalItemHolder != nullptr) {
        mGuideMessage = goalItemHolder->getNextGoalItemGuideMessage(this);
    }

    s32 waitStep = mGuideMessage == nullptr && mFinishedNum == 5 ? mFlagCameraOutStep : 0;
    if (!al::isGreaterEqualStep(this, waitStep)) {
        return;
    }

    if (!al::isNerve(this, &NrvLighthouseCameraOutReturnEnd)) {
        al::requestCaptureScreenCover(this, 2);
    }

    al::setNerve(this, &NrvLighthouseCameraFinish);
    setScenarioAnim();
}

/**
 * @brief Play the pending Cat Shine effect change.
 */
void Lighthouse::setScenarioAnim() {
    if (mFlagState == FlagState_Complete || !mIsScenarioAnimPending) {
        return;
    }

    mPrevFlagState = mFlagState;
    switch (mFinishedMainNum) {
    case 3:
        if (mFinishedNum > 4) {
            playEffect(FlagState_Complete);
            break;
        }

        [[fallthrough]];
    case 0:
        playEffect(FlagState_Partial);
        break;
    case 1:
        playEffect(FlagState_Partial);
        break;
    case 2:
        playEffect(FlagState_Partial);
        break;
    default:
        return;
    }

    mIsScenarioAnimPending = false;
    updateLod();
}

/**
 * @brief Finish the camera sequence.
 */
void Lighthouse::exeCameraFinish() {
    if (al::isLessEqualStep(this, 0)) {
        return;
    }

    mIsCameraSequenceStarted = false;
    if (mIsNoReturn) {
        _1d4 = false;
    }

    auto* goalItemHolder = al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);
    if (goalItemHolder != nullptr) {
        mGuideMessage = goalItemHolder->getNextGoalItemGuideMessage(this);
    }

    if (mGuideMessage != nullptr) {
        al::setNerve(this, &NrvLighthouseGuideMessage);
        return;
    }

    finishGoalItemCutscene();
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        controller->resume(true);
    }
}

/**
 * @brief Wait for the next Cat Shine.
 */
void Lighthouse::exeWait() {
    if (al::isFirstStep(this)) {
        if (mIsInked) {
            if (mSinkedItem != nullptr) {
                mSinkedItem->makeActorDead();
            }
        } else {
            tryStartGlassBlinkAnim();
            if (mSinkedItem != nullptr) {
                mSinkedItem->appear();
            }
        }
    }

    if (mSinkedItem != nullptr && mIsSinkedItemKillRequested) {
        killSinkedItem();
    }
}

/**
 * @brief Check whether the goal item camera sequence started.
 * @return True while the sequence runs.
 */
bool Lighthouse::isCameraSequenceStarted() {
    return mIsCameraSequenceStarted;
}

/**
 * @brief Set the scenario whose Cat Shine is being brought.
 * @param scenarioId The scenario id (1-based).
 */
void Lighthouse::setCurrentScenarioID(s32 scenarioId) {
    mCurrentScenarioId = scenarioId - 1;
}

/**
 * @brief Keep the ink off the lighthouse.
 * @param isInkMeNot Whether the lighthouse stays clear of ink.
 */
void Lighthouse::setInkMeNot(bool isInkMeNot) {
    mIsInkMeNot = isInkMeNot;
    if (!mIsInked || !isInkMeNot) {
        return;
    }

    al::clearMclAnim(this);
    mInk->makeActorDead();
    showMainLighthouse(true);
    setScenarioButtons(this);
    al::stopAllSeId(this, "InkFountain", 0, nullptr);
    mIsInked = false;
    playEffect(FlagState_Partial);
    setBirdsAppeared();
}

/**
 * @brief Set the link id of the ink patch.
 * @param linkId The link id.
 */
void Lighthouse::setInkLinkID(s32 linkId) {
    mInkLinkId = linkId;
}

/**
 * @brief Set the ink patch removed by the next Cat Shine.
 * @param pInkPatch The ink patch.
 * @param disappearStep Step at which the patch disappears.
 * @param waitStep Steps to wait before moving on.
 */
void Lighthouse::setInkPatch(InkPatch* pInkPatch, s32 disappearStep, s32 waitStep) {
    if (pInkPatch != nullptr && al::isAlive(pInkPatch)) {
        mInkPatch = pInkPatch;
        mInkPatchDisappearStep = disappearStep;
        mInkPatchWaitStep = waitStep;
    } else {
        mInkPatch = nullptr;
    }
}

/**
 * @brief Set how the camera returns to the player.
 * @param isReturnSetDir Whether to return with a set direction.
 * @param angleH The return angle (horizontal).
 * @param angleV The return angle (vertical).
 * @param dirRate The direction rate.
 * @param distanceRate The distance rate.
 * @param isReturnByStep Whether to use the return step.
 * @param returnStep The return step count.
 * @param returnOffsetY The height offset of the return target.
 * @param returnEndStep The interpolation step when ending the return camera.
 */
void Lighthouse::setExitAngleSettings(bool isReturnSetDir, f32 angleH, f32 angleV, f32 dirRate,
                                      f32 distanceRate, bool isReturnByStep, s32 returnStep,
                                      f32 returnOffsetY, s32 returnEndStep) {
    mReturnAngleH = angleH;
    mIsReturnSetDir = isReturnSetDir;
    mReturnAngleV = angleV;
    mReturnDirRate = dirRate;
    mIsReturnByStep = isReturnByStep;
    mReturnDistanceRate = sead::Mathf::max(distanceRate, 0.12f);
    mReturnStep = returnStep;
    mReturnOffsetY = returnOffsetY;
    mReturnEndStep = returnEndStep;
}

/**
 * @brief Set where the player landed.
 * @param rPos The landing position.
 */
void Lighthouse::setLandedPos(sead::Vector3f& rPos) {
    mLandedPos = rPos;
}

/**
 * @brief Set the camera return position.
 * @param rPos The return position.
 * @param distance The return distance.
 * @param isKeep Whether to keep the current return position.
 */
void Lighthouse::setReturnPos(sead::Vector3f& rPos, f32 distance, bool isKeep) {
    if (isKeep) {
        return;
    }

    mLandedPos.x = rPos.x;
    mLandedPos.z = rPos.z;
    mReturnDistance = distance;
}

/**
 * @brief Set the phase offsets of the area camera.
 * @param distanceOffset The distance offset.
 * @param heightOffset The height offset.
 */
void Lighthouse::setOffsets(f32 distanceOffset, f32 heightOffset) {
    mDistanceOffset = distanceOffset;
    mHeightOffset = heightOffset;
}

/**
 * @brief Store the current camera pose in the lighthouse camera.
 */
void Lighthouse::storeCamera() {
    mCamera->getPoser<al::CameraPoserFixActor>()->storeCamera(al::getCameraPos_RS(this, 0),
                                                               al::getCameraAt_RS(this, 0));
}

/**
 * @brief Start the cutscene of a Cat Shine flying to the lighthouse.
 * @param pGoalItem The Cat Shine.
 */
void Lighthouse::startGoalItemArrivalCutscene(GoalItem* pGoalItem) {
    if (mCurrentScenarioId > 4 || mIsCameraSequenceStarted) {
        return;
    }

    mGoalItem = pGoalItem;
    auto* poser = mCamera->getPoser<al::CameraPoserFixActor>();
    if (mCameraArea != nullptr) {
        mCameraArea->validate();
        mCameraArea->_77 = true;
    } else {
        al::startCamera_RS(this, mCamera, -1);
        if (mUnlockedPhase == 0) {
            al::setFixedActor(mCamera, this, 4500.0f, -100.0f, 0.0f, nullptr);
            al::setFixActorCameraOffset(mCamera, {0.0f, 1300.0f, 900.0f});
            poser->setTargetActor(this);
            poser->storeCamera(al::getCameraPos_RS(this, 0), al::getCameraAt_RS(this, 0));
            mLandedPos = al::getCameraAt_RS(this, 0);
            poser->setDistance(mFinishedNum == 5 ? 6000.0f : 5000.0f);
            poser->setInterpoleStep(mFinishedNum == 5 ? mFlagCameraInStep : 0);
            poser->setEndInterpoleStep(0);
        }
    }

    al::setNerve(this, &NrvLighthouseWaitForGoalItemArrival);
    mIsCameraSequenceStarted = true;
    mIsGoalItemArrived = false;
    mIsPhaseOffsetSet = false;
    mIsJumpedToFinish = false;
    _1d4 = false;
    _1d7 = false;
    mIsSkipped = false;
    mIsInkFadedOut = false;
    mIsLighthouseShone = false;
    al::invalidateClipping(this);
    if (mFlingPole != nullptr) {
        al::invalidateClipping(mFlingPole);
    }
}

/**
 * @brief Set the item sunk next to the lighthouse.
 * @param pSinkedItem The sunken item.
 */
void Lighthouse::setSinkedItem(SinkedItem* pSinkedItem) {
    if (pSinkedItem == nullptr) {
        return;
    }

    mSinkedItemOffset = al::getTrans(pSinkedItem) - al::getTrans(this);
    mSinkedItem = pSinkedItem;
}

/**
 * @brief Recount the finished scenarios and request the matching effect change.
 * @return False if the flag is already complete.
 */
bool Lighthouse::isOkayToChangeFlag() {
    if (mFlagState == FlagState_Complete) {
        return false;
    }

    s32 islandId = mPlacementHolder->getZoneNo() - 1;
    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, islandId);
    s32 finishedMainNum = 0;
    s32 finishedNum = 0;
    for (s32 i = 0; i < scenarioNum; i++) {
        if (SingleModeDataFunction::isScenarioComplete(this, islandId, i)) {
            finishedNum++;
            if (i < 3) {
                finishedMainNum++;
            }
        }
    }

    if (finishedNum == 5 || finishedMainNum > mFinishedMainNum) {
        mFinishedMainNum = finishedMainNum;
        mFinishedNum = finishedNum;
    }

    mIsScenarioAnimPending = true;
    return true;
}

/**
 * @brief Count the finished scenarios of the island.
 * @return The number of finished scenarios.
 */
s32 Lighthouse::getFinshedTotalCount() {
    s32 islandId = mPlacementHolder->getZoneNo() - 1;
    if (islandId < 0) {
        return 0;
    }

    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, islandId);
    s32 count = 0;
    for (s32 i = 0; i < scenarioNum; i++) {
        count += SingleModeDataFunction::isScenarioComplete(this, islandId, i);
    }

    return count;
}

/**
 * @brief Cover the lighthouse with ink while scenarios remain.
 * @param finishedCount The number of finished scenarios.
 */
void Lighthouse::turnOnInk(s32 finishedCount) {
    bool isInked = mIsInked;
    mIsInked = finishedCount != 5;
    if (mPlacementHolder->getZoneNo() == 1 && mUnlockedPhase <= 7 && mUnlockedPhase != 0) {
        mIsInked = false;
        return;
    }

    if (isInked || finishedCount == 5 || mIsInkMeNot) {
        return;
    }

    mInk->makeActorAppeared();
    showMainLighthouse(false);
    if (mSinkedItem != nullptr) {
        mSinkedItem->makeActorDead();
    }

    setScenarioButtons(mInk);
    mInkFinishedNum = finishedCount;
    playEffect(FlagState_Ink);
}

/**
 * @brief Set the island's active scenario.
 * @param scenario The active scenario index.
 */
void Lighthouse::setActiveScenario(s32 scenario) {
    s32 finishedNum = getFinshedTotalCount();
    if (finishedNum > mInkFinishedNum) {
        turnOnInk(finishedNum);
    }

    mActiveScenario = scenario;
    setBirdsAppeared();
}

/**
 * @brief Start lighting up once the Cat Shine arrived.
 * @param isSkip Whether the light sequence was skipped.
 */
void Lighthouse::setGoalItemArrived(bool isSkip) {
    mIsGoalItemArrived = true;
    if (isSkip) {
        al::setNerve(this, &NrvLighthouseSkippedLighthouseLight);
    } else {
        al::setNerve(this, &NrvLighthouseLighthouseLight);
    }
}

/**
 * @brief Log a visit of the island.
 * @param islandId The island id.
 * @param isPlayTime Whether to log the play time instead of the visit event.
 */
void Lighthouse::logIslandVisit(s32 islandId, bool, bool isPlayTime) {
    if (isPlayTime) {
        SingleModeDataFunction::setIslandPlayTime(this);
        return;
    }

    SingleModeDataFunction::reportIslandEvent(this, this, 0, islandId);
}

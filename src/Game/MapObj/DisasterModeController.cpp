#include "MapObj/DisasterModeController.hpp"

#include <cstring>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <math/seadMathCalcCommon.h>
#include <prim/seadSafeString.h>

#include "AreaObj/DisasterModeArea.hpp"
#include "AreaObj/IslandArea.hpp"
#include "Boss/SuperBowserShell.hpp"
#include "Demo/DemoAnimatic.hpp"
#include "Demo/DemoCutscene.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Enemy/SuperBowserGiantFireballState.hpp"
#include "Enemy/SuperBowserLaserState.hpp"
#include "Layout/IslandMap.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "MapObj/Fury/DisasterBlockDirector.hpp"
#include "MapObj/Fury/DisasterLightning.hpp"
#include "MapObj/Fury/DisasterSpikeDirector.hpp"
#include "MapObj/Fury/GameSkyProjection.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/GraphicsAreaController.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "MapObj/OceanWater.hpp"
#include "MapObj/SplatterPlotter.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DisasterModeController, Disaster)
NERVE_DECL(DisasterModeController, FirstAppearCutscene)
NERVE_DECL(DisasterModeController, DisasterTransitionWipeInDemo)
NERVE_DECL(DisasterModeController, Prosperity)
NERVE_DECL(DisasterModeController, AnticipationFast)
NERVE_DECL(DisasterModeController, DisasterTransitionInstant)
NERVE_DECL(DisasterModeController, ProsperityTransitionWipeIn)
NERVE_DECL(DisasterModeController, Anticipation)
NERVE_DECL(DisasterModeController, ProsperityTransitionWipeOut)
NERVE_DECL(DisasterModeController, PostBattlePeaceDelay)

/**
 * @brief Return to prosperity once the time expiry cutscene ended.
 */
class DisasterModeControllerNrvProsperityTransitionWipeInAfterLeave : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the controller.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DisasterModeController>()->exeProsperityTransitionWipeIn();
    }
};

NERVE_DECL(DisasterModeController, AnticipationTransition)
NERVE_DECL(DisasterModeController, NoOp)
NERVE_DECL(DisasterModeController, DisasterTransitionWipeIn)
NERVE_DECL(DisasterModeController, DisasterTransitionWipeOut)
NERVE_DECL(DisasterModeController, DisasterTransitionWipeOutDemo)
NERVE_DECL(DisasterModeController, LeaveCutscene)

NERVES_MAKE_NOSTRUCT(DisasterModeController, Disaster, FirstAppearCutscene,
                     DisasterTransitionWipeInDemo, Prosperity, AnticipationFast,
                     DisasterTransitionInstant, ProsperityTransitionWipeIn, Anticipation,
                     ProsperityTransitionWipeOut, PostBattlePeaceDelay,
                     ProsperityTransitionWipeInAfterLeave, AnticipationTransition, NoOp,
                     DisasterTransitionWipeIn, DisasterTransitionWipeOut,
                     DisasterTransitionWipeOutDemo, LeaveCutscene)

using ControllerFunctor =
    al::FunctorV0M<DisasterModeController*, void (DisasterModeController::*)()>;

/// Name of the rain effect.
constexpr const char* cRainEffectName = "EffectObjDarkBowserRain";
/// Name of the music of the first disaster cutscene.
constexpr const char* cFirstDemoBgmName = "DisasterKoopaFirstDemo";

/// Cutscene id of the first disaster.
constexpr s32 cCutsceneIdFirstDisaster = 2;
/// Cutscene id of the first hard disaster.
constexpr s32 cCutsceneIdFirstHardDisaster = 3;
/// Cutscene id of the first super hard disaster.
constexpr s32 cCutsceneIdFirstSuperHardDisaster = 4;
/// Cutscene id of the Giga Bell lock count increase.
constexpr s32 cCutsceneIdLockCountIncrement = 17;
/// Cutscene id of the time expiry.
constexpr s32 cCutsceneIdTimeExpire = 19;

/**
 * @brief Start a music track without any fade.
 * @param pActor The actor playing the music.
 * @param pName The name of the music.
 */
void startBgmNoFade(const al::LiveActor* pActor, const char* pName) {
    al::BgmPlayingRequest request(pName, -1, 0, 0);
    al::startBgm(pActor, request);
}

/**
 * @brief Start the music of the first disaster cutscene.
 * @param pActor The actor playing the music.
 * @param fadeOutFrames The frames to fade the music out over.
 */
void startFirstDemoBgm(const al::LiveActor* pActor, s32 fadeOutFrames) {
    al::stopBgm(pActor, "BeforeDisasterKoopa", -1, -1);
    al::changeLineAutoStopMode(pActor, cFirstDemoBgmName, true);
    al::startBgm(pActor, al::BgmPlayingRequest(cFirstDemoBgmName, -1, 0, fadeOutFrames));
}

/**
 * @brief Check whether an actor has a drawn model.
 * @param pActor The actor.
 * @return True if the actor has a model.
 */
bool isExistModelG3D(const al::LiveActor* pActor) {
    return pActor->getModelKeeper() != nullptr &&
           pActor->getModelKeeper()->getModelCafe()->getModelG3D() != nullptr;
}

/**
 * @brief Get the part of the story a phase belongs to.
 * @param phase The phase.
 * @return The part of the story, or -1 for boss phases.
 */
s32 getPhasePart(s32 phase) {
    switch (phase) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 3:
        return 2;
    case 5:
        return 3;
    case 8:
        return 4;
    default:
        return -1;
    }
}

/**
 * @brief Check whether a phase belongs to the first parts of the story.
 * @param phase The phase.
 * @return True for the first parts of the story.
 */
bool isEarlyPhase(s32 phase) {
    switch (phase) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        return true;
    default:
        return false;
    }
}
}  // namespace

/**
 * @brief Get the disaster mode controller of the scene.
 * @param pUser Object with access to the scene object holder.
 * @return The controller, or nullptr when the scene has none.
 */
DisasterModeController* DisasterModeController::tryGetController(
    const al::IUseSceneObjHolder* pUser) {
    return al::tryGetSceneObj<DisasterModeController>(pUser, SceneObjID_DisasterModeController);
}

/**
 * @brief Construct the controller.
 * @param pName Name of the actor.
 */
DisasterModeController::DisasterModeController(const char* pName)
    : al::LiveActor(pName), mBlockDirector(new DisasterBlockDirector()) {}

/**
 * @brief Kill every attack of Fury Bowser: fireballs, laser and spikes.
 */
void DisasterModeController::forceKillSuperBowserAttacks() {
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->forceKillFireballs();
    }

    forceKillSuperBowserLaser();

    if (mSpikeDirector != nullptr) {
        mSpikeDirector->forceKillFallingDisasterSpikes();
        mSpikeDirector->forceKillLaunchSpikes();
    }
}

/**
 * @brief Kill Fury Bowser's laser.
 */
void DisasterModeController::forceKillSuperBowserLaser() {
    if (mpSuperBowser != nullptr && mpSuperBowser->getLaserState() != nullptr) {
        mpSuperBowser->getLaserState()->forceKill();
    }
}

/**
 * @brief Switch Fury Bowser to his second form.
 * @param isV2 Whether to use the second form.
 */
void DisasterModeController::setSuperBowserV2(bool isV2) {
    mpSuperBowser->switchToV2(isV2);
}

/**
 * @brief Show or hide the skies and the ocean.
 * @param isEnable Whether the skies are shown.
 */
void DisasterModeController::setSkyEnable(bool isEnable) {
    if (mIsSkyEnable == isEnable) {
        return;
    }

    OceanWater* ocean = OceanWater::getOceanWater(this);
    mIsSkyEnable = isEnable;

    if (isEnable) {
        if (ocean != nullptr) {
            ocean->show();
        }

        if (mIsSkyDisaster) {
            setSkyDisaster();
            return;
        }

        showProsperitySky();
        return;
    }

    if (ocean != nullptr) {
        ocean->hide();
    }

    if (mSkyDisaster != nullptr) {
        mSkyDisaster->kill();
    }

    if (mSkyDisasterHard != nullptr) {
        mSkyDisasterHard->kill();
    }

    if (mSkyDisasterSuperHard != nullptr) {
        mSkyDisasterSuperHard->kill();
    }

    if (mSkyLake != nullptr) {
        mSkyLake->kill();
    }
}

/**
 * @brief Show the sky of the current disaster difficulty.
 */
void DisasterModeController::setSkyDisaster() {
    mIsSkyDisaster = true;
    if (!mIsSkyEnable) {
        return;
    }

    if (mSkyLake != nullptr && mSkyDisaster != nullptr) {
        mSkyLake->kill();

        GameSkyProjection* sky;
        if (mMode == Mode_Hard) {
            mGraphicsAreaController->switchToHardDisaster();
            sky = mSkyDisasterHard;
        } else if (mMode == Mode_SuperHard) {
            mGraphicsAreaController->switchToSuperHardDisaster();
            sky = mSkyDisasterSuperHard;
        } else {
            mGraphicsAreaController->switchToNormalDisaster();
            sky = mSkyDisaster;
        }

        sky->appear();
    }

    if (mSkyFinalCutscene != nullptr) {
        mSkyFinalCutscene->kill();
    }
}

/**
 * @brief Fade the graphics back to normal and show the sky of the prosperity.
 */
void DisasterModeController::setSkyProsperity() {
    mGraphicsAreaController->triggerFadeOverFramesFrom(1);
    mGraphicsAreaController->setLerp(1.0f);
    showProsperitySky();
}

/**
 * @brief Fade the graphics back to normal and show the sky of the final cutscene.
 */
void DisasterModeController::setSkyFinalCutscene() {
    mGraphicsAreaController->triggerFadeOverFramesFrom(1);
    mGraphicsAreaController->setLerp(1.0f);
    mIsSkyDisaster = false;
    if (!mIsSkyEnable) {
        return;
    }

    if (mSkyLake != nullptr && mSkyDisaster != nullptr) {
        mSkyLake->kill();
        mSkyDisasterHard->kill();
        mSkyDisasterSuperHard->kill();
        mSkyDisaster->kill();
    }

    if (mSkyFinalCutscene != nullptr) {
        mSkyFinalCutscene->appear();
    }
}

/**
 * @brief Destroy the controller.
 */
DisasterModeController::~DisasterModeController() = default;

/**
 * @brief Initialize the controller: timings, skies, Black Sun, cutscenes and effects.
 * @param rInfo The actor init info.
 */
void DisasterModeController::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "DisasterModeController", nullptr);
    mAllActorGroup = rInfo.mLiveActorGroup;
    makeActorAppeared();
    al::hideModelIfShow(this);
    al::invalidateClipping(this);
    al::setSceneObj(this, this, SceneObjID_DisasterModeController);

    {
        s32 prosperityTransitionSeconds;
        s32 prosperitySeconds;
        s32 anticipationTransitionSeconds;
        s32 anticipationSeconds;
        s32 disasterTransitionSeconds;
        s32 disasterSeconds;
        f32 anticipationPercentage;
        s32 shellPhaseCount;
        al::tryGetArg(&prosperityTransitionSeconds, rInfo, "SecondsOfProsperityTransition");
        al::tryGetArg(&prosperitySeconds, rInfo, "SecondsOfProsperity");
        al::tryGetArg(&anticipationTransitionSeconds, rInfo, "SecondsOfAnticipationTransition");
        al::tryGetArg(&anticipationSeconds, rInfo, "SecondsOfAnticipation");
        al::tryGetArg(&disasterTransitionSeconds, rInfo, "SecondsOfDisasterTransition");
        al::tryGetArg(&disasterSeconds, rInfo, "SecondsOfDisaster");
        al::tryGetArg(&anticipationPercentage, rInfo, "AnticipationPercentage");
        al::tryGetArg(&shellPhaseCount, rInfo, "ShellPhaseCount");

        mProsperityTransitionFrames = sead::Mathi::max(prosperityTransitionSeconds * 60, 1);
        mAnticipationTransitionFrames = sead::Mathi::max(anticipationTransitionSeconds * 60, 1);
        mAnticipationFrames = sead::Mathi::max(anticipationSeconds * 60, 1);
        mDisasterTransitionFrames = sead::Mathi::max(disasterTransitionSeconds * 60, 1);
        mDisasterDurationFrames = sead::Mathi::max(disasterSeconds * 60, 1);
        mPeaceFrames = 18000;
        mAnticipationPercentage = anticipationPercentage;
        mShellPhaseCount = sead::Mathi::max(shellPhaseCount, 1);
    }

    // Accessed through a base pointer, which the original null-checks.
    const al::LiveActor* actor = this;
    mPostBossPeaceFrames = SingleModeDataFunction::getDisasterModePostBossPeaceFrames(actor);
    al::tryGetArg(&mIsBlackSunFloating, rInfo, "DisableClock");
    al::tryGetArg(&mIsStartInDisasterMode, rInfo, "StartInDisasterMode");

    if (mIsStartInDisasterMode || SingleModeDataFunction::getIsDisasterMode(actor)) {
        al::initNerve(this, &NrvDisasterModeControllerDisasterTransitionInstant, 1);
    } else {
        al::initNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeIn, 1);
    }

    al::tryGetArg(&mIsHideShell, rInfo, "HideShell");

    {
        al::ByamlIter initIter;
        al::ByamlIter demoIter;
        if (al::tryGetActorInitFileIter(&initIter, this, "InitDemos", nullptr)) {
            initIter.tryGetIterByKey(&demoIter, "Demo Controls");
            demoIter.tryGetIntByKey(&mDisasterTransitionDemoFrame, "Disaster Transition Frame");
            demoIter.tryGetIntByKey(&mRepelDayTransitionFrame, "Repel Day Transition Frame");
            demoIter.tryGetIntByKey(&mDisappearDayTransitionFrame,
                                    "Disappear Day Transition Frame");
        }
    }

    al::LiveActor* switchUser = this;
    al::tryOffStageSwitch(switchUser, "SwitchAppearOn");
    al::tryOnStageSwitch(switchUser, "SwitchKillOn");
    al::tryOnStageSwitch(switchUser, "ProsperityAudioOn");

    auto* graphicsAreaController = new GraphicsAreaController("GraphicsAreaController");
    mGraphicsAreaController = graphicsAreaController;
    al::initCreateActorNoPlacementInfo(graphicsAreaController, rInfo);
    mGraphicsAreaController->finishInit(rInfo);

    {
        s32 skyNum = al::calcLinkChildNum(rInfo, "SkyChange");
        al::PlacementInfo placementInfo;
        if (skyNum == 4) {
            mSkyLake = new GameSkyProjection("SkyLake");
            al::initLinksActor(mSkyLake, rInfo, "SkyChange", 0);

            mSkyDisaster = new GameSkyProjection("SkyDisaster");
            al::initLinksActor(mSkyDisaster, rInfo, "SkyChange", 1);
            mSkyDisaster->kill();

            mSkyDisasterHard = new GameSkyProjection("SkyDisasterHard");
            al::initLinksActor(mSkyDisasterHard, rInfo, "SkyChange", 2);
            mSkyDisasterHard->kill();

            mSkyDisasterSuperHard = new GameSkyProjection("SkyDisasterSuperHard");
            al::initLinksActor(mSkyDisasterSuperHard, rInfo, "SkyChange", 3);
            mSkyDisasterSuperHard->kill();
        }
    }

    if (al::calcLinkChildNum(rInfo, "SkyFinalCutscene") >= 1) {
        mSkyFinalCutscene = new GameSkyProjection("SkyFinalCutscene");
        al::initLinksActor(mSkyFinalCutscene, rInfo, "SkyFinalCutscene", 0);
        mSkyFinalCutscene->kill();
    }

    if (al::calcLinkChildNum(rInfo, "SuperShell") != 0) {
        mShell = new SuperBowserShell("BlackSun");
        al::initLinksActor(mShell, rInfo, "SuperShell", 0);
        mShell->appear();
    }

    if (rc::isPlessieChase(SingleModeDataFunction::getUnlockedPhase(actor))) {
        mFirstAppearDemo = nullptr;
    } else {
        auto* firstAppearDemo = new DemoCutscene("DemoSingleModeDisasterModeStart",
                                                 static_cast<alSeFunction::DemoType>(5));
        mFirstAppearDemo = firstAppearDemo;
        firstAppearDemo->setPlacementBaseMtx(&sead::Matrix34f::ident);
        firstAppearDemo->setDemoName("DemoSingleModeDisasterModeStart");
        mFirstAppearDemo->init(rInfo);
        mFirstAppearDemo->setUnk32b(true);
        mFirstAppearDemo->setUnk32a(true);
        mFirstAppearDemo->registerFrameHook(
            ControllerFunctor(this, &DisasterModeController::disasterModeCutsceneTransitionFunc),
            mDisasterTransitionDemoFrame);
        mFirstAppearDemo->registerEndDemoHook(
            ControllerFunctor(this, &DisasterModeController::endFirstAppearCutsceneFunc));
        mFirstAppearDemo->registerCancelDemoHook(
            ControllerFunctor(this, &DisasterModeController::cancelFirstAppearCutsceneFunc));

        auto* timeExpireDemo = new DemoAnimatic("DisasterModeTimeExpireAnimatic",
                                                static_cast<alSeFunction::DemoType>(5));
        mTimeExpireDemo = timeExpireDemo;
        timeExpireDemo->setDemoName("DisasterModeTimeExpireAnimatic");
        mTimeExpireDemo->init(rInfo);
        mTimeExpireDemo->setUnk300(true);
        mTimeExpireDemo->setUnk32a(true);
    }

    mSpikeDirector = new DisasterSpikeDirector(this, rInfo);

    auto* splatterPlotter = new SplatterPlotter("SplatterPlotter");
    mSplatterPlotter = splatterPlotter;
    splatterPlotter->init(rInfo);

    if (mIsHideShell) {
        mShell->forceHide();
    }

    mWipeLayout = al::createSimpleLayout("DisasterMode", "DisasterMode",
                                         al::getLayoutInitInfo(rInfo), nullptr);
    al::setEffectFollowMtxPtr(this, cRainEffectName, &mRainMtx);
    mJumpToRainFunctor = new ControllerFunctor(this, &DisasterModeController::jumpToRain);

    mLightnings.allocBuffer(mLightningNum, nullptr);
    for (s32 i = 0; i < mLightningNum; i++) {
        mLightnings.pushBack(new DisasterLightning("DisasterLightning"));
        mLightnings.unsafeAt(i)->init(rInfo);
    }

    initFlow();
}

/**
 * @brief Skip the peace until the rain that precedes the next disaster.
 */
void DisasterModeController::jumpToRain() {
    mDisasterFramesOffset = calcProsperityStartAdditionalFramesMax();
    s32 frames = calcFramesOfProsperity() - mShell->getRainFrames();
    mDisasterFrames = frames;
    mDisasterFramesSync = frames;
    if (mShell != nullptr) {
        mShell->syncToDisasterTimer();
    }
}

/**
 * @brief Set up the prosperity/disaster cycles of the current phase.
 */
void DisasterModeController::initFlow() {
    switch (SingleModeDataFunction::getUnlockedPhase(this)) {
    case 0:
    case 3:
    case 5:
        mFlowNodeNum = 1;
        mFlowNodes[0].set(18000, 2400, false, false);
        mFlowIndex = 0;
        break;
    case 1:
        mFlowNodeNum = 2;
        mFlowNodes[0].set(10800, 2400, false, false);
        mFlowNodes[1].set(18000, 2400, false, false);
        if (SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdFirstDisaster)) {
            mFlowIndex = 1;
        } else {
            mFlowIndex = 0;
        }
        break;
    case 8:
        mFlowNodeNum = 6;
        mFlowNodes[0].set(7200, 2400, false, false);
        mFlowNodes[1].set(7200, 1800, true, false);
        mFlowNodes[2].set(3600, 2400, false, true);
        mFlowNodes[3].set(14400, 1800, true, false);
        mFlowNodes[4].set(14400, 1800, true, false);
        mFlowNodes[5].set(10800, 2400, false, true);
        if (SingleModeDataFunction::getDisasterModePostBossPeaceFrames(this) >= 1) {
            mFlowIndex = 0;
        } else {
            mFlowIndex = SingleModeDataFunction::getDisasterModeFlowIndex(this);
        }
        break;
    default:
        break;
    }

    SingleModeDataFunction::setDisasterModeFlowIndex(this, mFlowIndex);
}

/**
 * @brief Check whether the current phase is a last battle against Fury Bowser.
 * @param accessor Access to the game data.
 * @return True during a last battle.
 */
bool DisasterModeController::isLastBowserBattle(GameDataHolderAccessor accessor) {
    switch (SingleModeDataFunction::getUnlockedPhase(accessor)) {
    case 7:
    case 10:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Check whether a cutscene may start.
 * @return True when no rampage is in progress.
 */
bool DisasterModeController::isReadyForDemo() const {
    switch (mState) {
    case State::Prosperity:
    case State::AnticipationTransition:
    case State::Anticipation:
    case State::DisasterStart:
    case State::RainStart:
        return true;
    default:
        return mpSuperBowser == nullptr || al::isDead(mpSuperBowser);
    }
}

/**
 * @brief Stop the disaster timer and Fury Bowser for a cutscene.
 */
void DisasterModeController::pauseWithSuperBowser() {
    mIsBlackSunFloating = true;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->pauseForDemo();
    }
}

/**
 * @brief Resume the disaster timer and Fury Bowser after a cutscene.
 */
void DisasterModeController::resumeWithSuperBowser() {
    mIsBlackSunFloating = false;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->resumeFromDemo();
    }
}

/**
 * @brief Get the step of the disaster cycle.
 * @return The step of the disaster cycle.
 */
DisasterModeController::State DisasterModeController::getState() {
    return mState;
}

/**
 * @brief Count the frames spent in the current nerve.
 * @return The frames spent in the current nerve.
 */
s32 DisasterModeController::getStateFrame() {
    return al::getNerveStep(this);
}

/**
 * @brief Check whether the game is in the phase before the first disaster.
 * @param accessor Unused access to the game data.
 * @return True before the first disaster.
 */
bool DisasterModeController::isPhaseZero(GameDataHolderAccessor accessor) {
    return SingleModeDataFunction::isPhase0(this);
}

/**
 * @brief Check whether Fury Bowser is being repelled.
 * @return True while Fury Bowser is repelled.
 */
bool DisasterModeController::isRepelling() {
    if (mpSuperBowser == nullptr) {
        return false;
    }

    return mpSuperBowser->isRepelling();
}

/**
 * @brief Check whether Fury Bowser is hidden.
 * @return True while Fury Bowser is hidden.
 */
bool DisasterModeController::isBowserHidden() {
    if (mpSuperBowser == nullptr) {
        return false;
    }

    return mpSuperBowser->isHidden();
}

/**
 * @brief Create Fury Bowser if he does not exist yet.
 * @param rInfo The actor init info.
 * @return Fury Bowser.
 */
SuperBowser* DisasterModeController::tryCreateSuperBowser(const al::ActorInitInfo& rInfo) {
    if (mpSuperBowser != nullptr) {
        return mpSuperBowser;
    }

    bool isLastBattle = isLastBowserBattle(this);
    mpSuperBowser = new SuperBowser("SuperBowser");
    mpSuperBowser->setLastPhase3Bowser(isLastBattle);
    return mpSuperBowser;
}

/**
 * @brief Finish the initialization once every actor is placed.
 */
void DisasterModeController::initAfterPlacement() {
    {
        al::PlacementId placementId;
        al::GraphicsAreaDirector* areaDirector =
            getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector();
        s32 areaNum = areaDirector->getGraphicsAreaNum();
        for (s32 i = 0; i < areaNum; i++) {
            const al::GraphicsAreaInfo* areaInfo = areaDirector->getGraphicsAreaInfoByIndex(i);
            if (areaInfo != nullptr) {
                al::tryGetPlacementID(&placementId, areaInfo->getAreaObj()->getPlacementInfo());
            }
        }
    }

    mGigaBellManager = GigaBellManager::tryGetManager(this);
    if (SingleModeDataFunction::getIsDisasterMode(this) && mGigaBellManager != nullptr) {
        mGigaBellManager->forceUnlockBells();
    }

    if (isGigaBellLockCountIncremented() &&
        !SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdLockCountIncrement)) {
        clearPostBossPeaceFrames();
        mIsJumpToRain = true;
        tryStartAnticipationMusic(true, true);
        if (mShell != nullptr) {
            mShell->setRainFrames(1200);
        }
    }
}

/**
 * @brief Check whether the game is in a part of the story.
 * @param part The part of the story.
 * @param isLockCountIncremented Whether the Giga Bell lock count must have been increased.
 * @return True when the game is in the part.
 */
bool DisasterModeController::isPhasePart(s32 part, bool isLockCountIncremented) const {
    if (getPhasePart(SingleModeDataFunction::getUnlockedPhase(this)) != part) {
        return false;
    }

    auto* manager = al::tryGetSceneObj<GigaBellManager>(this, SceneObjID_GigaBellManager);
    bool isIncremented = manager != nullptr ? manager->hasLockCountBeenIncremented() : false;
    return isIncremented == isLockCountIncremented;
}

/**
 * @brief Clear the peace given after a boss battle.
 */
void DisasterModeController::clearPostBossPeaceFrames() {
    SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, 0);
    mPostBossPeaceFrames = 0;
}

/**
 * @brief Start the music announcing Fury Bowser if it is not playing yet.
 * @param isDelayed Whether to start the music after a short delay.
 * @param isLong Whether to use the long version of the music.
 */
void DisasterModeController::tryStartAnticipationMusic(bool isDelayed, bool isLong) {
    if (mIsDisasterForeshadow) {
        return;
    }

    if (isPhaseZero(this)) {
        return;
    }

    if (isLastBowserBattle(this)) {
        return;
    }

    s32 delay = isDelayed ? 20 : 0;
    const char* name;
    if (isEarlyPhase(SingleModeDataFunction::getUnlockedPhase(this))) {
        name = "BeforeDisasterKoopa";
    } else {
        name = isLong ? "BeforeDisasterKoopa" : "BeforeDisasterKoopaShort";
    }

    al::startBgm(this, al::BgmPlayingRequest(name, 120, delay, 146));
    mIsDisasterForeshadow = true;
}

/**
 * @brief Make the controller appear.
 */
void DisasterModeController::appear() {
    al::LiveActor::appear();
}

/**
 * @brief Set the durations and difficulty of a cycle.
 * @param prosperityFrames The frames of peace.
 * @param disasterFrames The frames of disaster.
 * @param isMini Whether the disaster is a short one.
 * @param isHard Whether the disaster is a hard one.
 */
void DisasterModeController::DisasterModeFlowNode::set(s32 prosperityFrames, s32 disasterFrames,
                                                       bool isMini, bool isHard) {
    this->prosperityFrames = prosperityFrames;
    this->disasterFrames = disasterFrames;
    this->isMini = isMini;
    this->isHard = isHard;
}

/**
 * @brief Queue a music change.
 * @param request The music change.
 * @param isForce Whether to replace an already queued change.
 */
void DisasterModeController::startBgmRequest(BGM_REQUEST request, bool isForce) {
    if (request >= BGM_REQUEST_STOP_DISASTER && !isForce &&
        mBgmRequest >= BGM_REQUEST_STOP_DISASTER) {
        return;
    }

    mBgmRequest = request;

    s32 delay;
    switch (request) {
    case BGM_REQUEST_STOP_DISASTER:
        delay = 820;
        break;
    case BGM_REQUEST_PROSPERITY:
        delay = 0;
        break;
    case BGM_REQUEST_LEAVE_CUTSCENE:
        delay = 940;
        break;
    case BGM_REQUEST_END_INSTANTLY:
    case BGM_REQUEST_END:
        delay = 0;
        break;
    default:
        mBgmRequestDelay = 0;
        return;
    }

    mIsBgmPlaying = false;
    mBgmRequestDelay = delay;
}

/**
 * @brief Apply the queued music change once its delay ran out.
 */
void DisasterModeController::updateBgmRequest() {
    if (mBgmRequestDelay >= 1) {
        if (!mIsPausedByUser) {
            mBgmRequestDelay--;
        }

        return;
    }

    switch (mBgmRequest) {
    case BGM_REQUEST_DISASTER: {
        bool isInvincible = false;
        al::LiveActor* player = al::tryFindNearestPlayerActor(this);
        if (player != nullptr) {
            isInvincible = rc::isPlayerInvincible(player);
        }

        bool isHard = calcHardMode();
        bool isSuperHard = calcSuperHardMode();
        const char* name = isHard ? "DisasterKoopaHard" : "DisasterKoopa";
        name = isSuperHard ? "DisasterKoopaSuperHard" : name;

        al::BgmPlayingRequest request(name, -1, 0, -1);
        if (isInvincible) {
            if (!isSuperHard) {
                request._18 = 0;
                request.fadeInFrames = 60;
            }
        } else {
            if (mIsBeginWithDemo) {
                request.fadeOutFrames = 7;
                mIsBeginWithDemo = false;
            } else {
                request.fadeOutFrames = 10;
            }

            request.startDelayFrames = 70;
        }

        al::startBgm(this, request);
        mIsBgmPlaying = true;
        mBgmRequest = BGM_REQUEST_NONE;
        return;
    }
    case BGM_REQUEST_PROSPERITY:
        mIsBgmPlaying = false;
        al::stopBgm(this, "DarkBowserScaredLong", 0, 0);
        al::stopBgm(this, "DarkBowserScaredShort", 0, 0);
        al::stopBgm(this, "DisasterKoopa", 90, 150);
        al::stopBgm(this, "DisasterKoopaHard", 90, 150);
        al::stopBgm(this, "DisasterKoopaSuperHard", 90, 150);
        mBgmRequest = BGM_REQUEST_NONE;
    case BGM_REQUEST_STOP_DISASTER:
        mIsBgmPlaying = false;
        al::stopBgm(this, "DarkBowserScaredLong", 0, 0);
        al::stopBgm(this, "DarkBowserScaredShort", 0, 0);
        al::stopBgm(this, "DisasterKoopa", 50, 150);
        al::stopBgm(this, "DisasterKoopaHard", 50, 150);
        al::stopBgm(this, "DisasterKoopaSuperHard", 50, 150);
        mBgmRequest = BGM_REQUEST_NONE;
        return;
    case BGM_REQUEST_LEAVE_CUTSCENE:
        mIsBgmPlaying = false;
        al::stopBgm(this, "DarkBowserScaredLong", 0, 0);
        al::stopBgm(this, "DarkBowserScaredShort", 0, 0);
        al::stopBgm(this, "DisasterKoopa", 40, 150);
        al::stopBgm(this, "DisasterKoopaHard", 40, 150);
        al::stopBgm(this, "DisasterKoopaSuperHard", 40, 150);
        return;
    case BGM_REQUEST_END_INSTANTLY:
    case BGM_REQUEST_END:
        mIsBgmPlaying = false;
        al::stopBgm(this, "DarkBowserScaredLong", 40, 150);
        al::stopBgm(this, "DarkBowserScaredShort", 40, 150);
        al::stopBgm(this, "DisasterKoopa", 40, 150);
        al::stopBgm(this, "DisasterKoopaHard", 40, 150);
        al::stopBgm(this, "DisasterKoopaSuperHard", 40, 150);
        mBgmRequest = BGM_REQUEST_NONE;
        return;
    default:
        return;
    }
}

/**
 * @brief Check whether the next disaster is a hard one.
 * @return True for a hard disaster.
 */
bool DisasterModeController::calcHardMode() {
    if (mIsForceMode) {
        return mForceMode == Mode_Hard;
    }

    s32 lockCount = mGigaBellManager != nullptr ? mGigaBellManager->getLockCountMin() : 0;
    if (SingleModeDataFunction::isSuperHardModeOn(this, lockCount)) {
        return true;
    }

    if (isGigaBellLockCountIncremented()) {
        return true;
    }

    if (SingleModeDataFunction::getGoalItemsCollected(this) ==
        SingleModeDataFunction::getMaxCollectableGoalItems()) {
        return false;
    }

    if (mIsAllDisasterShinesRemaining) {
        return true;
    }

    if (mIsAllNekoShinesRemaining) {
        return false;
    }

    return mFlowNodes[mFlowIndex].isHard;
}

/**
 * @brief Check whether the next disaster is a super hard one.
 * @return True for a super hard disaster.
 */
bool DisasterModeController::calcSuperHardMode() {
    if (mIsForceMode) {
        return mForceMode == Mode_SuperHard;
    }

    s32 lockCount = mGigaBellManager != nullptr ? mGigaBellManager->getLockCountMin() : 0;
    return SingleModeDataFunction::isSuperHardModeOn(this, lockCount);
}

/**
 * @brief Switch the ambient sounds between prosperity, anticipation and disaster.
 */
void DisasterModeController::updateAmbientSE() {
    State target;
    if (mIsDisasterModeAnim) {
        target = State::DisasterStart;
    } else {
        target = mIsRaining ? State::Anticipation : State::Prosperity;
    }

    switch (mAmbientSeState) {
    case State::Prosperity:
        if (target != State::Prosperity) {
            mAmbientSeState = State::Anticipation;
            al::tryOffStageSwitchInstant(this, "ProsperityAudioOn");
            al::tryOnStageSwitchInstant(this, "AnticipationAudioOn");
        }
        break;
    case State::Anticipation:
        if (target != State::Anticipation) {
            mAmbientSeState = State::DisasterStart;
            al::tryOffStageSwitchInstant(this, "AnticipationAudioOn");
            al::tryOnStageSwitchInstant(this, "DisasterAudioOn");
        }
        break;
    case State::DisasterStart:
        if (target != State::DisasterStart) {
            mAmbientSeState = State::Prosperity;
            al::tryOffStageSwitchInstant(this, "DisasterAudioOn");
            al::tryOnStageSwitchInstant(this, "ProsperityAudioOn");
        }
        break;
    default:
        break;
    }
}

/**
 * @brief Check whether it is raining.
 * @return True while it rains.
 */
bool DisasterModeController::isRaining() {
    return mIsRaining;
}

/**
 * @brief Check whether Fury Bowser is rampaging.
 * @return True during the disaster.
 */
bool DisasterModeController::isDisasterNerve() {
    return al::isNerve(this, &NrvDisasterModeControllerDisaster);
}

/**
 * @brief Check whether the disaster is starting.
 * @return True from the anticipation transition to the end of the disaster transition.
 */
bool DisasterModeController::isDisasterStarting() {
    return mState >= State::AnticipationTransition && mState <= State::DisasterTransitionEnd;
}

/**
 * @brief Check whether the first disaster cutscene of the current difficulty must play.
 * @return True if the cutscene must play.
 */
bool DisasterModeController::needToPlayFirstDisasterModeCutscene() {
    if (mIsNeedFirstAppearDemo) {
        return true;
    }

    if (calcSuperHardMode()) {
        return !SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdFirstSuperHardDisaster);
    }

    if (isGigaBellLockCountIncremented() && calcHardMode()) {
        return !SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdFirstHardDisaster);
    }

    return !SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdFirstDisaster);
}

/**
 * @brief Start the first disaster cutscene if it must play.
 * @return True if the cutscene started.
 */
bool DisasterModeController::tryPlayFirstDisasterModeCutscene() {
    if (!needToPlayFirstDisasterModeCutscene()) {
        return false;
    }

    mIsNeedFirstAppearDemo = false;
    al::requestCaptureScreenCover(this, 4);
    al::setNerve(this, &NrvDisasterModeControllerFirstAppearCutscene);
    return true;
}

/**
 * @brief Called at the end of the first disaster cutscene.
 */
void DisasterModeController::endFirstAppearCutsceneFunc() {
    al::tryOffStageSwitchInstant(this, "DemoStartOn");

    if (calcSuperHardMode()) {
        SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdFirstSuperHardDisaster);
    } else if (isGigaBellLockCountIncremented() && calcHardMode()) {
        SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdFirstHardDisaster);
    }

    SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdFirstDisaster);
    al::showModelIfHide(mShell);
    al::addDemoActor(mpSuperBowser);
    setState(0, true, 0, false);
    mpSuperBowser->setFirstAppearDone();
    mGraphicsAreaController->setLerpStep(1);

    if (mFirstAppearDemo != nullptr) {
        mFirstAppearDemo->cancelSE();
    }

    if (mGigaBellManager == nullptr || !mGigaBellManager->tryQueueCutscene()) {
        auto* luckyIslandHolder =
            al::tryGetSceneObj<LuckyIslandHolder>(this, SceneObjID_LuckyIslandHolder);
        if (luckyIslandHolder != nullptr && luckyIslandHolder->canStartLuckyIslandDemo()) {
            luckyIslandHolder->queueLuckyIslandDemo();
        }
    }

    al::disableLineChange(this, false);
    mSplatterPlotter->disable(false);
}

/**
 * @brief Jump to the prosperity or to the disaster.
 * @param startFrames The frames of peace to start from.
 * @param isDisaster Whether to jump to the disaster.
 * @param pauseFrames When positive, the disaster timer is stopped.
 * @param isFromCutscene Whether the disaster music continues a cutscene.
 */
void DisasterModeController::setState(s32 startFrames, bool isDisaster, s32 pauseFrames,
                                      bool isFromCutscene) {
    if (mIsStartInDisasterMode) {
        setSkyDisaster();
    } else if (isDisaster) {
        mGraphicsAreaController->triggerFadeOverFramesTo(1);
        mGraphicsAreaController->setLerp(1.0f);
        mDisasterFramesSync = calcCycleFrames();
        if (mShell != nullptr) {
            mShell->forceHide();
        }

        if (al::isNerve(this, &NrvDisasterModeControllerFirstAppearCutscene)) {
            al::changeLineAutoStopMode(this, cFirstDemoBgmName, false);
            al::stopBgm(this, cFirstDemoBgmName, -1, -1);
        }

        al::LiveActor* player = al::tryFindNearestPlayerActor(this);
        bool isVulnerable = player == nullptr || !rc::isPlayerInvincible(player);
        bool isHard = calcHardMode();
        bool isSuperHard = calcSuperHardMode();
        const char* name = isHard ? "DisasterKoopaHard" : "DisasterKoopa";
        name = isSuperHard ? "DisasterKoopaSuperHard" : name;

        al::BgmPlayingRequest request(name, -1, 0, 10);
        if (isFromCutscene) {
            request.fadeInFrames = 60;
            request.startDelayFrames = 5;
            request._18 = 3400000;
        }

        if (!(isVulnerable || isSuperHard)) {
            request._18 = 0;
            request.fadeInFrames = 60;
        }

        al::startBgm(this, request);
        mIsBgmPlaying = true;
        al::changeLineAutoStopMode(this, cFirstDemoBgmName, false);

        if (!mIsPreDisasterModeDone) {
            preDisasterMode();
        }

        appearSuperBowser();
        al::setNerve(this, &NrvDisasterModeControllerDisaster);
        setSkyDisaster();
        al::tryOnStageSwitch(this, "SwitchAppearOn");
        al::tryOffStageSwitch(this, "SwitchKillOn");
    } else {
        setSkyProsperity();
        if (mShell != nullptr && !mIsBlackSunFloating) {
            mShell->returnFromDisaster();
        }

        if (mpSuperBowser != nullptr && mpSuperBowser->isActive()) {
            mpSuperBowser->kill();
        }

        if (mIsBgmPlaying) {
            startBgmRequest(BGM_REQUEST_PROSPERITY, false);
        }

        if (mpSuperBowser != nullptr && mpSuperBowser->isLastPhase3Bowser()) {
            if (mShell != nullptr) {
                mShell->kill();
            }

            al::setNerve(this, &NrvDisasterModeControllerDisasterTransitionInstant);
        } else {
            al::setNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeIn);
        }

        mIsDisasterMode = false;
        mIsDisasterModeAnim = false;
        mStartFrames = startFrames;
        al::tryOffStageSwitch(this, "SwitchKillOn");
        al::tryOnStageSwitch(this, "SwitchKillOn");
    }

    if (pauseFrames >= 1) {
        mIsBlackSunFloating = true;
    }
}

/**
 * @brief Called when the first disaster cutscene is skipped.
 */
void DisasterModeController::cancelFirstAppearCutsceneFunc() {
    al::disableLineChange(this, true);
    al::stopBgm(this, cFirstDemoBgmName, -1, -1);
    mSplatterPlotter->disable(false);
}

/**
 * @brief Called when the first disaster cutscene reaches the disaster transition.
 */
void DisasterModeController::disasterModeCutsceneTransitionFunc() {
    al::setNerve(this, &NrvDisasterModeControllerDisasterTransitionWipeInDemo);
}

/**
 * @brief Set the speed of the wipes.
 * @param wipeInRate The speed of the wipe into the disaster.
 * @param wipeOutRate The speed of the wipe out of the disaster.
 */
void DisasterModeController::setTransitionFrameRate(f32 wipeInRate, f32 wipeOutRate) {
    mWipeInFrameRate = wipeInRate;
    mWipeOutFrameRate = wipeOutRate;
}

/**
 * @brief Update the music, sounds, rain and timers every frame.
 */
void DisasterModeController::control() {
    updateBgmRequest();
    updateAmbientSE();

    if (al::isNerve(this, &NrvDisasterModeControllerProsperity)) {
        if (mDoubleTimeDelay >= 0 && mDoubleTimeDelay-- == 0) {
            mDisasterFrames += 1800;
            mDisasterFramesSync += 1800;
        }
    } else {
        mDoubleTimeDelay = -1;
    }

    if (mpSuperBowser != nullptr &&
        mpSuperBowser->getGiantFireballState()->isShootNowAndForever()) {
        mpSuperBowser->getGiantFireballState()->update();
    }

    checkNoDisasterIslandAreas();
    updateRainEffects();

    if (al::isEffectEmitting(this, cRainEffectName)) {
        mRainMtx.setInverse(*al::getCameraViewMtxPtr(this));
    }

    mSplatterPlotter->update();

    if (mForeshadowOffDelay >= 1 && --mForeshadowOffDelay == 0) {
        forceDisasterForeshadowOff(true, 0);
    }
}

/**
 * @brief Stop the disaster cycle while the player is on an island where disasters cannot happen.
 */
void DisasterModeController::checkNoDisasterIslandAreas() {
    sead::Vector3f playerTrans = al::getTrans(al::tryFindNearestPlayerActor(this));
    auto* area = static_cast<IslandArea*>(
        rc::tryFindAreaObj(this, rc::AreaObjType::IslandArea, playerTrans));
    if (area != nullptr && area->isNoDisaster()) {
        forceDisasterForeshadowOff(false, 0);
    }

    if ((area == nullptr || !area->isNoDisaster()) && mIsForeshadowOff &&
        mIsDisasterProgressEnable) {
        mIsForeshadowOff = false;
        resume(true);
    }
}

/**
 * @brief Start or stop the rain effects depending on where the camera and the player are.
 */
void DisasterModeController::updateRainEffects() {
    const sead::Vector3f& cameraPos =
        getCameraDirector_RS()->getSceneCameraInfo()->getViewAt(0)->getLookAtCam().getPos();
    al::LiveActor* player = al::tryFindNearestPlayerActor(this);

    if (mIsRaining && !rc::isInPlessieTunnel(this, cameraPos)) {
        bool isCameraInNoRainArea = rc::isInNoRainArea(this, cameraPos, true);
        if (player == nullptr || !isCameraInNoRainArea ||
            !rc::isInNoRainArea(this, al::getTrans(player), true)) {
            if (!al::isEffectEmitting(this, cRainEffectName)) {
                al::emitEffect(this, cRainEffectName, nullptr);
                mSplatterPlotter->run(true);
            }

            return;
        }
    }

    if (al::isEffectEmitting(this, cRainEffectName)) {
        al::tryDeleteEffectAndParticle(this, cRainEffectName);
        mSplatterPlotter->run(false);
    }
}

/**
 * @brief Check whether the rain effects are shown.
 * @return True while the rain effects are shown.
 */
bool DisasterModeController::isRainEffectsOn() {
    return al::isEffectEmitting(this, cRainEffectName);
}

/**
 * @brief Stop the disaster cycle and the foreshadowing of the next disaster.
 * @param isForce Whether to skip the wipe back to the prosperity.
 * @param delay When positive, the frames to wait before stopping.
 */
void DisasterModeController::forceDisasterForeshadowOff(bool isForce, s32 delay) {
    if (delay >= 1) {
        mForeshadowOffDelay = delay;
        return;
    }

    if (mIsForeshadowOff) {
        return;
    }

    bool isRaining = mIsRaining;
    if (mpSuperBowser != nullptr && al::isAlive(mpSuperBowser)) {
        mIsSkipProsperityWipe = true;
        endInstantly(true, true);
    } else {
        bool isProsperity = al::isNerve(this, &NrvDisasterModeControllerProsperity);
        if (isRaining || !isProsperity) {
            if (isRaining) {
                mIsJumpToRain = true;
                mDisasterFramesSync = 0;
                mDisasterFrames = 0;
                if (mShell != nullptr) {
                    mShell->syncToDisasterTimer();
                }
            }

            if (al::isNerve(this, &NrvDisasterModeControllerFirstAppearCutscene)) {
                mFirstAppearDemo->forceWait();
            }

            if (al::isNerve(this, &NrvDisasterModeControllerLeaveCutscene)) {
                mTimeExpireDemo->forceWait();
            }

            if (isForce) {
                mIsSkipProsperityWipe = true;
            }

            mIsEnding = true;
            mIsForeshadowOffForced = true;
            endDisasterPhase();
            mIsForeshadowOffForced = false;
            stopForeshadowBgm();
            if (mShell != nullptr) {
                mShell->stopAllSe();
            }
        }
    }

    mIsPausedByUser = false;
    mIsBlackSunFloating = true;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->stopAttackTimer();
    }

    mIsForeshadowOff = true;
}

/**
 * @brief Update the controller while the game is paused.
 * @param isPaused Whether the game is paused.
 */
void DisasterModeController::movementPaused(bool isPaused) {
    if (al::isEffectEmitting(this, cRainEffectName)) {
        sead::Vector3f trans;
        mRainMtx.getTranslation(trans);
        mRainMtx.setInverse(*al::getCameraViewMtxPtr(this));
        mRainMtx.setTranslation(trans);
        if (getEffectKeeper() != nullptr) {
            getEffectKeeper()->update();
        }
    }

    al::LiveActor::movementPaused(isPaused);
}

/**
 * @brief Find the island of the disaster area around the player.
 * @param pActor The actor searching.
 * @param offset Offset added to the player position.
 * @return The id of the island, or -1 when outside of every island.
 */
s32 DisasterModeController::findAreaId(const al::LiveActor* pActor, sead::Vector3f offset) {
    al::AreaObjGroup* group = rc::tryFindAreaObjGroup(pActor, rc::AreaObjType::DisasterModeArea);
    if (group != nullptr) {
        al::LiveActor* player = al::tryFindNearestPlayerActor(pActor);
        if (player != nullptr) {
            auto* area = static_cast<DisasterModeArea*>(
                group->getInVolumeAreaObj(offset + al::getTrans(player)));
            if (area != nullptr) {
                return area->getIslandID();
            }
        }
    }

    return -1;
}

/**
 * @brief Receive a message.
 * @param pMsg The message.
 * @param pOther The sensor sending the message.
 * @param pSelf The sensor receiving the message.
 * @return Always false.
 */
bool DisasterModeController::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                        al::HitSensor* pSelf) {
    return false;
}

/**
 * @brief Receive a message from a screen pointer.
 * @param pMsg The message.
 * @param pPointer The screen pointer.
 * @param pTarget The pointed target.
 * @return True for touch assist messages.
 */
bool DisasterModeController::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                                   al::ScreenPointer* pPointer,
                                                   al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

/**
 * @brief Start the anticipation of the next disaster.
 * @param isWithDemo Whether the first disaster cutscene follows.
 */
void DisasterModeController::begin(bool isWithDemo) {
    mIsBeginWithDemo = isWithDemo;
    mIsNeverEnd = false;
    mIsTimeStopped = false;
    if (mIsDisasterMode) {
        if (mpSuperBowser != nullptr && !mpSuperBowser->isActive()) {
            mpSuperBowser->appear();
        }
    } else {
        al::setNerve(this, &NrvDisasterModeControllerAnticipationTransition);
    }

    if (mIsBeginWithDemo) {
        al::changeLineAutoStopMode(this, cFirstDemoBgmName, true);
    } else {
        tryStartAnticipationMusic(false, false);
    }
}

/**
 * @brief Start the anticipation of the next disaster with the first disaster cutscene.
 */
void DisasterModeController::beginWithDemo() {
    mIsNeedFirstAppearDemo = true;
    begin(false);
}

/**
 * @brief Start the next disaster right away.
 * @param isWithDemo Whether the first disaster cutscene follows.
 */
void DisasterModeController::beginImmediate(bool isWithDemo) {
    mIsBeginWithDemo = isWithDemo;
    mIsNeverEnd = false;
    mIsTimeStopped = false;
    if (mIsDisasterMode) {
        if (mpSuperBowser != nullptr && !mpSuperBowser->isActive()) {
            mpSuperBowser->appear();
        }
    } else {
        al::setNerve(this, &NrvDisasterModeControllerAnticipationFast);
    }
}

/**
 * @brief Debug: start a disaster right away that never ends.
 */
void DisasterModeController::beginImmediateAndNeverEndDebug() {
    mIsNeverEnd = true;
    mIsTimeStopped = false;
    if (mIsDisasterMode) {
        if (mpSuperBowser != nullptr && !mpSuperBowser->isActive()) {
            mpSuperBowser->appear();
        }
    } else {
        al::setNerve(this, &NrvDisasterModeControllerAnticipationFast);
    }

    if (mpSuperBowser != nullptr) {
        mpSuperBowser->setLeaveRequested(true);
    }
}

/**
 * @brief Debug: start a disaster that never ends.
 */
void DisasterModeController::beginAndNeverEndDebug() {
    mIsNeverEnd = true;
    mIsTimeStopped = false;
    if (mIsDisasterMode) {
        if (mpSuperBowser != nullptr && !mpSuperBowser->isActive()) {
            mpSuperBowser->appear();
        }
    } else {
        al::setNerve(this, &NrvDisasterModeControllerAnticipationTransition);
    }

    tryStartAnticipationMusic(false, false);
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->setLeaveRequested(true);
    }
}

/**
 * @brief Restart the disaster timer.
 */
void DisasterModeController::resetDisasterTimer() {
    if (mIsDisasterMode) {
        mDisasterFramesSync = calcCycleFrames();
        mDisasterFrames = 0;
    }
}

/**
 * @brief End the disaster: Fury Bowser leaves.
 */
void DisasterModeController::end() {
    mIsNeverEnd = false;
    mIsTimeStopped = false;
    if (mpSuperBowser != nullptr && mpSuperBowser->isActive()) {
        mpSuperBowser->requestDisappear(&onBowserDisappearStart, &onBowserDisappeared, this,
                                        false);
        mpSuperBowser->end();
    } else {
        endDisasterPhase();
    }

    mIsBlackSunFloating = false;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->setLeaveRequested(false);
    }

    endForeshadow();
}

/**
 * @brief End the foreshadowing of the next disaster.
 */
void DisasterModeController::endForeshadow() {
    mIsDisasterForeshadow = false;
    SingleModeDataFunction::setAutoForeshadow(this, false);
}

/**
 * @brief End the disaster because its time ran out.
 */
void DisasterModeController::endByTime() {
    mIsNeverEnd = false;
    mIsTimeStopped = false;
    mIsBlackSunFloating = false;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->endByTime();
    }

    endForeshadow();
}

/**
 * @brief End the disaster because its time ran out, with the time expiry cutscene.
 */
void DisasterModeController::endWithDemo() {
    mIsNeedFirstAppearDemo = true;
    endByTime();
}

/**
 * @brief Debug: end the disaster and never start another one.
 */
void DisasterModeController::endAndNeverBeginDebug() {
    endInstantly(true, true);
    mIsNeverEnd = false;
    mIsTimeStopped = true;
    mIsBlackSunFloating = true;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->setLeaveRequested(false);
    }

    if (mShell != nullptr) {
        mShell->skipTo(0, false, false);
    }
}

/**
 * @brief End the disaster right away.
 * @param isStopBgm Whether to stop the disaster music.
 * @param isDisappear Whether Fury Bowser disappears.
 */
void DisasterModeController::endInstantly(bool isStopBgm, bool isDisappear) {
    mIsNeverEnd = false;
    mIsTimeStopped = false;
    mIsBlackSunFloating = false;
    if (mpSuperBowser != nullptr) {
        mIsEnding = true;
        if (isDisappear) {
            mpSuperBowser->disappear(false);
        }

        endDisasterPhase();
    }

    if (isStopBgm) {
        startBgmRequest(BGM_REQUEST_END_INSTANTLY, false);
    }

    endForeshadow();
}

/**
 * @brief End the disaster by repelling Fury Bowser.
 */
void DisasterModeController::endImmediate() {
    mIsNeverEnd = false;
    mIsTimeStopped = false;
    mIsBlackSunFloating = false;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->repel();
    }

    endForeshadow();
}

/**
 * @brief Called once Fury Bowser disappeared.
 * @param pController The controller.
 */
void DisasterModeController::onBowserDisappeared(void* pController) {
    static_cast<DisasterModeController*>(pController)->endDisasterPhase();
}

/**
 * @brief Stop the disaster timer.
 * @param isByUser Whether the user paused the timer.
 */
void DisasterModeController::pause(bool isByUser) {
    mIsPausedByUser = isByUser;
    mIsBlackSunFloating = true;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->stopAttackTimer();
    }
}

/**
 * @brief Resume the disaster timer.
 * @param isForce Whether to resume a timer paused by the user.
 */
void DisasterModeController::resume(bool isForce) {
    if (mIsPausedByUser && isForce) {
        mIsPausedByUser = false;
    }

    if (mIsForeshadowOff) {
        return;
    }

    mIsBlackSunFloating = false;
    if (mShell != nullptr && mDisasterFrames == 0) {
        mShell->returnFromDisaster();
    }
}

/**
 * @brief Stop or resume the disaster timer.
 */
void DisasterModeController::togglePause() {
    mIsBlackSunFloating = !mIsBlackSunFloating;
}

/**
 * @brief Toggle Fury Bowser's laser attack.
 */
void DisasterModeController::laserAttack() {
    mpSuperBowser->toggleLaserAttack();
}

/**
 * @brief Compute how far the disaster cycle is.
 * @return The rate of the disaster cycle.
 */
f32 DisasterModeController::disasterPercentage() {
    return static_cast<f32>(mDisasterFramesSync) / static_cast<f32>(calcCycleFrames());
}

/**
 * @brief Check whether the Black Sun reached its full size.
 * @return True once the Black Sun reached its full size.
 */
bool DisasterModeController::isShellMax() {
    return static_cast<f32>(mDisasterFramesSync) / static_cast<f32>(mPeaceFrames) > 1.0f;
}

/**
 * @brief Check whether the disaster is about to start.
 * @return True during the anticipation.
 */
bool DisasterModeController::isAnticipation() {
    return al::isNerve(this, &NrvDisasterModeControllerAnticipation);
}

/**
 * @brief Check whether the white transition back to the prosperity is playing.
 * @return True during the white transition.
 */
bool DisasterModeController::isWhiteOut() {
    return al::isNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeOut);
}

/**
 * @brief Check whether a wipe is playing.
 * @return True while a wipe is playing.
 */
bool DisasterModeController::isWipeActive() {
    return al::isAnyActionPlaying(mWipeLayout, nullptr) && !al::isActionEnd(mWipeLayout, nullptr);
}

/**
 * @brief Access the Black Sun.
 * @return The Black Sun, or nullptr when absent.
 */
al::LiveActor* DisasterModeController::getShell() {
    return mShell;
}

/**
 * @brief Stop Fury Bowser's fireballs.
 */
void DisasterModeController::stopFireballs() {
    if (mFireballStopCount != 0) {
        return;
    }

    mFireballStopCount = 1;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->setFireballEnable(false);
    }
}

/**
 * @brief Resume Fury Bowser's fireballs.
 */
void DisasterModeController::startFireballs() {
    if (mFireballStopCount < 1) {
        return;
    }

    mFireballStopCount--;
    if (mpSuperBowser != nullptr) {
        mpSuperBowser->setFireballEnable(true);
    }
}

/**
 * @brief Use the plain wipe for the transitions.
 */
void DisasterModeController::useWipePlain() {
    mIsWipePlain = true;
}

/**
 * @brief Use the Bowser wipe for the transitions.
 */
void DisasterModeController::useWipeBowser() {
    mIsWipePlain = false;
}

/**
 * @brief Draw the disaster timer on screen when the debug display is enabled.
 */
void DisasterModeController::tryDrawDisasterModeTimer() {
    if (!mIsDrawDebugTimer) {
        return;
    }

    f32 width = static_cast<u32>(al::getDebugMenuDisplayWidth());
    f32 height = static_cast<u32>(al::getDebugMenuDisplayHeight());
    sead::Viewport viewport(0.0f, 0.0f, width, height);
    sead::TextWriter::setupGraphics(al::GameFrameworkNx::getDrawContext());
    sead::TextWriter writer(al::GameFrameworkNx::getDrawContext(), &viewport);
    sead::Vector2f cursor(20.0f, 120.0f);
    writer.setCursorFromTopLeft(cursor);

    s32 frames;
    const char* label;
    if (al::isNerve(this, &NrvDisasterModeControllerPostBattlePeaceDelay)) {
        frames = mPostBossPeaceFrames;
        label = "POST BOSS PEACE TIME";
    } else if (al::isNerve(this, &NrvDisasterModeControllerProsperity)) {
        frames = calcFramesOfProsperity();
        label = "TIME UNTIL DISASTER";
    } else if (al::isNerve(this, &NrvDisasterModeControllerDisaster)) {
        frames = mDisasterDurationFrames;
        label = "TIME UNTIL PROSPERITY";
    } else {
        writer.printf("TRANSITIONING...");
        return;
    }

    writer.printf(label);
    frames -= mDisasterFrames;
    if (al::isNerve(this, &NrvDisasterModeControllerProsperity)) {
        frames = calcProsperityStartAdditionalFramesMax() + frames - mDisasterFramesOffset;
    }

    writer.printf(": %02d:%02d", frames / 60, frames % 60);
    if (mIsBlackSunFloating) {
        writer.printf(" (PAUSED)");
    }

    if (al::isNerve(this, &NrvDisasterModeControllerProsperity)) {
        if (mDisasterFramesOffset < calcProsperityStartAdditionalFramesMax()) {
            writer.printf(" (BLACK SUN PAUSE TIME)");
        }
    } else if (al::isNerve(this, &NrvDisasterModeControllerDisaster)) {
        const char* debug = mIsForceMode ? " - DEBUG" : "";
        if (calcSuperHardMode()) {
            writer.printf(" (SUPER HARD%s)", debug);
        } else if (calcHardMode()) {
            writer.printf(" (HARD%s)", debug);
        } else {
            writer.printf(" (NORMAL%s)", debug);
        }

        if (mFlowNodes[mFlowIndex].isMini) {
            writer.printf(" (MINI)", debug);
        }
    }

    if (al::isNerve(this, &NrvDisasterModeControllerDisaster)) {
        cursor.x += 0.0f;
        cursor.y += 20.0f;
        writer.setCursorFromTopLeft(cursor);
        writer.printf("DISASTER TIME: %02d:%02d", mDisasterElapsedFrames / 60,
                      mDisasterElapsedFrames % 60);
        if (mFlowNodeNum >= 2) {
            writer.setCursorFromTopLeft(cursor + sead::Vector2f(0.0f, 20.0f));
            writer.printf("FLOW INDEX: %d/%d", mFlowIndex + 1, mFlowNodeNum);
        }
    }
}

/**
 * @brief Count the frames of peace before the next disaster, including the Black Sun's skip.
 * @return The frames of peace.
 */
s32 DisasterModeController::calcFramesOfProsperity() const {
    return mShell != nullptr ? mPeaceFrames + mShell->getWaitSkipFrames() : mPeaceFrames;
}

/**
 * @brief Count the frames the Black Sun waits at the start of the prosperity.
 * @return The frames the Black Sun waits.
 */
s32 DisasterModeController::calcProsperityStartAdditionalFramesMax() {
    return mShell != nullptr ? sead::Mathi::max(mPreRainFrames - mShell->getWaitSkipFrames(), 0) :
                               mPreRainFrames;
}

/**
 * @brief Start the music of the prosperity.
 */
void DisasterModeController::startBGM() {
    if (isLastBowserBattle(this)) {
        return;
    }

    if (isPhaseZero(this)) {
        tryStartPhase0Music();
    }
}

/**
 * @brief Start the music of the phase before the first disaster.
 */
void DisasterModeController::tryStartPhase0Music() {
    startBgmNoFade(this, "DisasterKoopaPhase0");
}

/**
 * @brief Start the next disaster once.
 */
void DisasterModeController::oneTimeAutoTrigger() {
    mIsOneTimeAutoTrigger = true;
}

/**
 * @brief Wait.
 */
void DisasterModeController::exeWait() {}

/**
 * @brief Wipe back to the prosperity.
 */
void DisasterModeController::exeProsperityTransitionWipeIn() {
    if (al::isStep(this, 0)) {
        changeState(State::ProsperityTransitionStart);
        if (!mIsProsperityTransitionNeeded) {
            al::setNerve(this, &NrvDisasterModeControllerProsperity);
            return;
        }

        if (!mIsTimeJumped) {
            mDisasterFrames = 0;
        }

        bool isAfterLeave =
            al::isNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeInAfterLeave);
        if (mBgmRequest == BGM_REQUEST_NONE && !isAfterLeave && mIsBgmPlaying && !_385) {
            startBgmRequest(BGM_REQUEST_PROSPERITY, false);
        }

        if (!mIsSkipProsperityWipe) {
            mWipeLayout->appear();
            IslandMap::setIslandMapEnable(this, false);
            al::tryStartAction(mWipeLayout, "DisasterStartInPlain", nullptr);
        }
    }

    if (!mIsSkipProsperityWipe && !al::isActionEnd(mWipeLayout, nullptr)) {
        return;
    }

    mGraphicsAreaController->triggerFadeOverFramesFrom(1);
    mGraphicsAreaController->setLerp(1.0f);
    al::tryOffStageSwitchInstant(this, "SwitchAppearOn");
    al::tryOnStageSwitchInstant(this, "SwitchKillOn");
    al::tryOffStageSwitch(this, "ProsperityOn");
    al::tryOnStageSwitch(this, "ProsperityTransitionOn");
    al::tryOffStageSwitch(this, "AnticipationTransitionOn");
    showProsperitySky();
    mIsDisasterMode = false;
    mIsDisasterModeAnim = false;
    mIsPreDisasterModeDone = false;
    tryChangeActorWetMaterial(false);
    changeState(State::Normal);
    al::setNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeOut);

    if (!rc::isAnyActiveDemo(this) && mGigaBellManager != nullptr &&
        mGigaBellManager->tryQueueReturnCutscene(false, true, true)) {
        al::requestCaptureScreenCover(this, 3);
        rc::addDemoActor(this);
    }

    if (isGigaBellLockCountIncremented() && calcSuperHardMode()) {
        mIsTimeJumped = true;
        jumpToRain();
    }
}

/**
 * @brief Switch the materials of every actor between wet and dry.
 * @param isWet Whether the materials are wet.
 */
void DisasterModeController::tryChangeActorWetMaterial(bool isWet) {
    if (mIsWetMaterial == isWet) {
        return;
    }

    if (isWet) {
        for (s32 i = 0; i < mAllActorGroup->getActorCount(); i++) {
            al::LiveActor* actor = mAllActorGroup->getActor(i);
            if (strcmp(actor->getName(), "透明ブンブン[コウラ]") == 0 ||
                strcmp(actor->getName(), "透明ブンブン") == 0 ||
                strcmp(actor->getName(), "透明ブンブン[手]") == 0 ||
                strcmp(actor->getName(), "ガマネカメレオンモデル") == 0) {
                continue;
            }

            if (isExistModelG3D(actor)) {
                al::changeEnvTextureWetObj(actor);
            }
        }

        if (mpSuperBowser != nullptr) {
            al::resetEnvTexture(mpSuperBowser);
        }

        if (mShell != nullptr) {
            al::resetEnvTexture(mShell);
        }
    } else {
        for (s32 i = 0; i < mAllActorGroup->getActorCount(); i++) {
            al::LiveActor* actor = mAllActorGroup->getActor(i);
            if (isExistModelG3D(actor)) {
                al::resetEnvTexture(actor);
            }
        }
    }

    mIsWetMaterial = isWet;
}

/**
 * @brief Start the white transition back to the prosperity.
 */
void DisasterModeController::startWhiteTransitionToProsperity() {
    if (mpSuperBowser != nullptr) {
        al::tryKillEmitterAndParticleAll(mpSuperBowser);
        mpSuperBowser->killHealthBar();
    }

    al::tryStartAction(mWipeLayout, "DisasterStartOutPlain", nullptr);
    al::setActionFrameRate(mWipeLayout, mWipeOutFrameRate, nullptr);
}

/**
 * @brief Fade from the white transition into the prosperity.
 */
void DisasterModeController::exeProsperityTransitionWipeOut() {
    if (al::isStep(this, 0)) {
        changeState(State::ProsperityTransitionEnd);
        if (!mIsSkipProsperityWipe) {
            startWhiteTransitionToProsperity();
        }

        if (mFadeInDoneFunctor != nullptr) {
            (*mFadeInDoneFunctor)();
            mFadeInDoneFunctor = nullptr;
        }

        if (mGigaBellManager != nullptr) {
            mGigaBellManager->resetGigaBells();
        }
    }

    if (!mIsSkipProsperityWipe && !al::isActionEnd(mWipeLayout, nullptr)) {
        return;
    }

    mIsSkipProsperityWipe = false;
    mWipeLayout->kill();
    if (!mIsTimeJumped) {
        mDisasterFrames = mProsperityTransitionFrames;
    }

    auto* raidonSurf = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
    if (raidonSurf != nullptr) {
        raidonSurf->updateSpawns(false);
    }

    al::setNerve(this, &NrvDisasterModeControllerProsperity);
}

/**
 * @brief Check whether the current disaster is a hard one.
 * @return True for a hard disaster.
 */
bool DisasterModeController::isHardMode() const {
    return mMode == Mode_Hard;
}

/**
 * @brief Check whether the current disaster is a super hard one.
 * @return True for a super hard disaster.
 */
bool DisasterModeController::isSuperHardMode() const {
    return mMode == Mode_SuperHard;
}

/**
 * @brief Check whether Fury Bowser is leaving.
 * @return True while Fury Bowser is leaving.
 */
bool DisasterModeController::isSuperBowserLeaving() {
    if (mpSuperBowser != nullptr) {
        if (mpSuperBowser->isLeaving()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Start the cutscene of Fury Bowser leaving.
 * @param isFirst Whether it is the first time Fury Bowser leaves.
 * @return True if the cutscene started.
 */
bool DisasterModeController::setIsFirstDisappearCutscene(bool isFirst) {
    mIsInDisappearDemo = true;
    mFreezeFrames = 30;
    if (!rc::requestStartDemoInGameCutscene(this)) {
        return false;
    }

    mActorSceneInfo->cameraDirector->freezeCameraInput(true);
    if (mGigaBellManager == nullptr || !isFirst ||
        !mGigaBellManager->tryQueueReturnCutscene(false, false, false)) {
        al::addDemoActor(this);
    }

    return true;
}

/**
 * @brief Freeze the camera for a moment after a cutscene during the disaster.
 * @return True during the disaster.
 */
bool DisasterModeController::setPostCutsceneDisasterFreezeTime() {
    if (!al::isNerve(this, &NrvDisasterModeControllerDisaster)) {
        return false;
    }

    mFreezeFrames = 30;
    return true;
}

/**
 * @brief Clear the requests to jump through the disaster cycle.
 */
void DisasterModeController::clearTimeJumpFlags() {
    mIsTimeJumped = false;
    mIsJumpToForeshadow = false;
    mIsJumpToRain = false;
    SingleModeDataFunction::setAutoForeshadow(this, false);
}

/**
 * @brief Debug: skip time.
 * @param seconds The seconds to skip.
 */
void DisasterModeController::skip(s32 seconds) {
    s32 frames = seconds * 60;
    if (al::isNerve(this, &NrvDisasterModeControllerProsperity)) {
        if (mDisasterFramesOffset < calcProsperityStartAdditionalFramesMax()) {
            mDisasterFramesOffset += frames;
            if (mDisasterFramesOffset <= calcProsperityStartAdditionalFramesMax()) {
                return;
            }

            frames = mDisasterFramesOffset - calcProsperityStartAdditionalFramesMax();
            mDisasterFramesOffset = calcProsperityStartAdditionalFramesMax();
        }

        if (frames >= 1) {
            s32 disasterFrames =
                sead::Mathi::min(mDisasterFrames + frames, calcFramesOfProsperity());
            mDisasterFramesSync = disasterFrames;
            mDisasterFrames = disasterFrames;
            clearTimeJumpFlags();
            if (mShell != nullptr) {
                mShell->syncToDisasterTimer();
            }
        }
    } else if (al::isNerve(this, &NrvDisasterModeControllerDisaster)) {
        s32 disasterFrames = sead::Mathi::min(mDisasterFrames + frames, mDisasterDurationFrames);
        mDisasterFramesSync = disasterFrames;
        mDisasterFrames = disasterFrames;
    }
}

/**
 * @brief Debug: force the difficulty of the disasters.
 * @param isForce Whether the difficulty is forced.
 * @param mode The difficulty.
 */
void DisasterModeController::setForceMode(bool isForce, Mode mode) {
    if (mIsDisasterMode) {
        showProsperitySky();
    }

    mIsForceMode = isForce;
    mForceMode = mode;
    mMode = mode;

    if (mIsDisasterMode) {
        setSkyDisaster();
        switch (mode) {
        case Mode_Hard:
        case Mode_SuperHard:
            mIsBlackSunFloating = true;
            if (mpSuperBowser != nullptr) {
                mpSuperBowser->setLeaveRequested(true);
            }
            break;
        case Mode_Normal:
            mIsBlackSunFloating = false;
            if (mpSuperBowser != nullptr) {
                mpSuperBowser->setLeaveRequested(false);
            }
            break;
        default:
            break;
        }
    } else if (isForce) {
        beginImmediate(false);
    }
}

/**
 * @brief Prepare the next disaster: compute its difficulty.
 */
void DisasterModeController::preDisasterMode() {
    mIsAllDisasterShinesRemaining = false;
    mIsAllNekoShinesRemaining = false;
    if (SingleModeDataFunction::getUnlockedPhase(this) == 8) {
        if (SingleModeDataFunction::allDisasterShinesRemaining(this)) {
            mIsAllDisasterShinesRemaining = true;
        } else if (SingleModeDataFunction::allNekoShinesRemaining(this)) {
            mIsAllNekoShinesRemaining = true;
        }
    }

    if (isGigaBellLockCountIncremented() && mShell != nullptr) {
        mShell->setRainFrames(600);
    }

    calcMode();
    mIsPreDisasterModeDone = true;
}

/**
 * @brief Compute the difficulty of the next disaster.
 */
void DisasterModeController::calcMode() {
    if (calcSuperHardMode()) {
        mMode = Mode_SuperHard;
    } else if (calcHardMode()) {
        mMode = Mode_Hard;
    } else {
        mMode = Mode_Normal;
    }
}

/**
 * @brief Move to the next prosperity/disaster cycle.
 */
void DisasterModeController::tryIncrementFlow() {
    if (mFlowNodeNum < 2) {
        return;
    }

    mFlowIndex = (mFlowIndex + 1) % mFlowNodeNum;
    if (SingleModeDataFunction::getUnlockedPhase(this) != 8 &&
        SingleModeDataFunction::getUnlockedPhase(this) == 1 && mFlowIndex == 0) {
        mFlowIndex = 1;
    }

    SingleModeDataFunction::setDisasterModeFlowIndex(this, mFlowIndex);
}

/**
 * @brief Stop the rain and prevent it from starting again.
 */
void DisasterModeController::disableRain() {
    mIsRainDisabled = true;
    stopRain();
}

/**
 * @brief Stop the rain.
 */
void DisasterModeController::stopRain() {
    if (mIsRaining) {
        mIsRaining = false;
        updateRainEffects();
    }
}

/**
 * @brief Halve the time until the next disaster once enough goal items are collected.
 * @return True if the time was halved.
 */
bool DisasterModeController::tryApplyDoubleTime() {
    if (SingleModeDataFunction::getUnlockedPhase(this) == 8) {
        return false;
    }

    if (SingleModeDataFunction::getGoalItemsCollected(this) <
        SingleModeDataFunction::getGigaBellLockCount(GameDataHolderAccessor(this))) {
        return false;
    }

    if (mIsDoubleTime || mIsRaining) {
        return false;
    }

    mDisasterFrames /= 2;
    mIsDoubleTime = true;
    mPeaceFrames /= 2;
    if (mShell != nullptr) {
        if (mShell->getWaitSkipFrames() >= 1) {
            mDisasterFrames += mShell->getWaitSkipFrames() / 2;
        }

        if (mShell != nullptr) {
            mShell->recalculatePhaseChangeFrames();
        }
    }

    s32 offsetMax = calcProsperityStartAdditionalFramesMax();
    if (mDisasterFramesOffset > offsetMax) {
        mDisasterFrames += mDisasterFramesOffset - offsetMax;
        mDisasterFramesOffset = offsetMax;
    }

    if (mShell != nullptr) {
        s32 rainStartFrames = calcFramesOfProsperity() - mShell->getRainFrames();
        if (mDisasterFrames > rainStartFrames) {
            mDisasterFrames = rainStartFrames;
        }
    }

    return true;
}

/**
 * @brief Check whether enough goal items are collected to halve the time until the next disaster.
 * @return True once enough goal items are collected.
 */
bool DisasterModeController::isDoubleTime() const {
    if (SingleModeDataFunction::getUnlockedPhase(this) == 8) {
        return false;
    }

    return SingleModeDataFunction::getGoalItemsCollected(this) >=
           SingleModeDataFunction::getGigaBellLockCount(GameDataHolderAccessor(this));
}

/**
 * @brief Peace: count down to the next disaster.
 */
void DisasterModeController::exeProsperity() {
    if (al::isStep(this, 0)) {
        mIsPreDisasterModeDone = false;
        IslandMap::setIslandMapEnable(this, true);
        tryChangeActorWetMaterial(false);
        changeState(State::Prosperity);
        mPeaceFrames = mFlowNodes[mFlowIndex].prosperityFrames;
        mIsDoubleTime = false;
        al::tryOffStageSwitch(this, "AnticipationTransitionOn");
        al::tryOnStageSwitch(this, "ProsperityOn");
        if (mShell != nullptr) {
            mShell->recalculatePhaseChangeFrames();
        }

        if (mStartFrames >= 1) {
            tryApplyDoubleTime();
            s32 startFrames = mStartFrames;
            s32 offset = sead::Mathi::min(startFrames, calcProsperityStartAdditionalFramesMax());
            mDisasterFramesOffset = offset;
            mDisasterFrames = mStartFrames - offset;
            mDisasterFramesSync = mStartFrames - offset;
            mStartFrames = 0;
            if (mShell != nullptr) {
                mShell->syncToDisasterTimer();
            }
        } else if (mIsTimeJumped) {
            mIsTimeJumped = false;
        } else {
            mDisasterFramesSync = 0;
            mDisasterFrames = 0;
            mDisasterFramesOffset = 0;
            if (mShell != nullptr) {
                mShell->returnFromDisaster();
            }
        }

        mGraphicsAreaController->setLerpStep(-1);
        if (mPostBossPeaceFrames >= 1 && !isLastBowserBattle(this)) {
            al::setNerve(this, &NrvDisasterModeControllerPostBattlePeaceDelay);
            return;
        }
    }

    if (tryApplyDoubleTime() && mShell != nullptr) {
        mShell->syncToDisasterTimer();
    }

    if (mFreezeFrames >= 1 && --mFreezeFrames == 0) {
        rc::requestEndDemoInGameCutscene(this);
        mActorSceneInfo->cameraDirector->freezeCameraInput(false);
        mIsInDisappearDemo = false;
    }

    if (isGigaBellLockCountIncremented() &&
        !SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdLockCountIncrement)) {
        startRain();
        return;
    }

    if (rc::isActiveDemo(this)) {
        return;
    }

    if (calcSuperHardMode()) {
        if (mDisasterFrames < calcFramesOfProsperity() - mShell->getRainFrames() &&
            !mIsBlackSunFloating) {
            mIsJumpToRain = true;
        }
    }

    if (mIsJumpToRain) {
        mIsJumpToRain = false;
        jumpToRain();
    }

    if (mDisasterFramesSync < getForeshadowStartFrame()) {
        if (mIsJumpToForeshadow || SingleModeDataFunction::isAutoForeshadow(this)) {
            if (mIsJumpToForeshadow) {
                s32 frames = calcFramesOfProsperity() - 1800;
                mDisasterFrames = frames;
                mDisasterFramesSync = frames;
            } else if (SingleModeDataFunction::isAutoForeshadow(this)) {
                s32 frames = getForeshadowStartFrame();
                mDisasterFrames = frames;
                mDisasterFramesSync = frames;
            }

            mIsJumpToForeshadow = false;
            mDisasterFramesOffset = calcProsperityStartAdditionalFramesMax();
            if (mShell != nullptr) {
                mShell->syncToDisasterTimer();
            }
        }
    } else if (!SingleModeDataFunction::isAutoForeshadow(this)) {
        SingleModeDataFunction::setAutoForeshadow(this, true);
    }

    if (mpSuperBowser != nullptr && mpSuperBowser->isLastPhase3Bowser()) {
        beginImmediate(false);
        return;
    }

    if (mIsTimeStopped) {
        return;
    }

    if (!mIsBlackSunFloating) {
        if (mDisasterFrames == calcFramesOfProsperity() || mIsOneTimeAutoTrigger) {
            if (mIsOneTimeAutoTrigger) {
                _248 = false;
                mIsOneTimeAutoTrigger = false;
            }

            al::setNerve(this, &NrvDisasterModeControllerAnticipationTransition);
        }
    }

    if (mIsTimeStopped || mIsBlackSunFloating) {
        return;
    }

    if (mDisasterFramesOffset < calcProsperityStartAdditionalFramesMax()) {
        mDisasterFramesOffset++;
    } else {
        s32 nextFrames = mDisasterFrames + 1;
        s32 prosperityFrames = calcFramesOfProsperity();
        mDisasterFrames = prosperityFrames < nextFrames ? prosperityFrames : nextFrames;
        mDisasterFramesSync++;
    }

    SingleModeDataFunction::recordDisasterMode(
        this, SingleModeDataFunction::DisasterForceSetting_Controller, this);
}

/**
 * @brief Start the rain that announces the next disaster.
 */
void DisasterModeController::startRain() {
    if (mIsRaining || mIsRainDisabled) {
        return;
    }

    mIsRaining = true;
    notifyStateChange(State::RainStart);
    updateRainEffects();
}

/**
 * @brief Count the frames of peace after which the disaster is foreshadowed.
 * @return The frames of peace before the foreshadowing.
 */
s32 DisasterModeController::getForeshadowStartFrame() {
    if (mShell == nullptr) {
        return 0;
    }

    return calcFramesOfProsperity() - mShell->getStep4Frames();
}

/**
 * @brief The Black Sun prepares to launch Fury Bowser.
 */
void DisasterModeController::exeAnticipationTransition() {
    if (al::isStep(this, 0)) {
        preDisasterMode();
        startRain();
        changeState(State::AnticipationTransition);
        al::tryOffStageSwitch(this, "AnticipationOn");
        al::tryOnStageSwitch(this, "AnticipationTransitionOn");
        if (mShell != nullptr && mpSuperBowser != nullptr) {
            if (tryPlayFirstDisasterModeCutscene()) {
                return;
            }

            mShell->preLaunch(&onShellPreLaunched, this);
        }
    }

    mDisasterFramesSync++;
}

/**
 * @brief Called once the Black Sun is ready to launch Fury Bowser.
 * @param pController The controller.
 */
void DisasterModeController::onShellPreLaunched(void* pController) {
    auto* controller = static_cast<DisasterModeController*>(pController);
    if (controller->mShell != nullptr && controller->mpSuperBowser != nullptr) {
        if (controller->mIsHideShell) {
            onShellLaunched(controller);
        } else {
            controller->mShell->launch(&onShellLaunched, controller);
        }
    }

    al::setNerve(controller, &NrvDisasterModeControllerAnticipation);
}

/**
 * @brief The first disaster cutscene plays.
 */
void DisasterModeController::exeFirstAppearCutscene() {
    if (al::isStep(this, 0)) {
        if (!rc::requestStartDemoInGameCutscene(this)) {
            al::setNerve(this, &NrvDisasterModeControllerFirstAppearCutscene);
            return;
        }

        al::tryOnStageSwitchInstant(this, "DemoStartOn");
        mFirstAppearDemo->setUnk302(false);
        mFirstAppearDemo->startDemo();
        startFirstDemoBgm(this, mIsBeginWithDemo ? 5 : 10);
        mIsBeginWithDemo = false;
        if (rc::isActiveDemo(mFirstAppearDemo)) {
            rc::addDemoActor(this);
        }

        mShell->forceHide();
        changeState(State::Anticipation);
        getSceneInfo()->cameraDirector->storeCamera();
        mFreezeFrames = 30;
        mIsInFirstAppearDemo = true;
        mSplatterPlotter->disable(true);
    }

    if (mFirstAppearDemo->isEndDemo()) {
        al::changeLineAutoStopMode(this, cFirstDemoBgmName, false);
        al::stopBgm(this, cFirstDemoBgmName, -1, -1);
    }
}

/**
 * @brief The time expiry cutscene plays.
 */
void DisasterModeController::exeLeaveCutscene() {
    if (al::isFirstStep(this)) {
        startBgmRequest(BGM_REQUEST_LEAVE_CUTSCENE, false);
    }

    if (mTimeExpireDemo->isEndDemo()) {
        SingleModeDataFunction::setHasSeenCutscene(this, cCutsceneIdTimeExpire);
        al::setNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeInAfterLeave);
        _249 = false;
    }
}

/**
 * @brief The Black Sun launched Fury Bowser, who is about to appear.
 */
void DisasterModeController::exeAnticipation() {
    if (al::isStep(this, 0)) {
        changeState(State::Anticipation);
        al::tryOffStageSwitch(this, "DisasterTransitionOn");
        al::tryOnStageSwitch(this, "AnticipationOn");
        mDisasterFrames = 0;
    }

    if ((!mIsBlackSunFloating || mGoalItemDisasterTrigger) &&
        mDisasterFrames == mAnticipationFrames) {
        al::setNerve(this, &NrvDisasterModeControllerNoOp);
    }

    mDisasterFrames = sead::Mathi::min(mAnticipationFrames, mDisasterFrames + 1);
    mDisasterFramesSync++;
}

/**
 * @brief Skip the anticipation and launch Fury Bowser right away.
 */
void DisasterModeController::exeAnticipationFast() {
    if (!al::isStep(this, 0)) {
        return;
    }

    preDisasterMode();
    al::tryOffStageSwitch(this, "AnticipationOn");
    al::tryOnStageSwitch(this, "AnticipationTransitionOn");
    mDisasterFrames = 0;
    if (!mIsSkipAnticipationFade) {
        mGraphicsAreaController->triggerFadeOverFramesTo(mAnticipationTransitionFrames +
                                                         mDisasterTransitionFrames);
    }

    if (!isPhaseZero(this) && !isLastBowserBattle(this)) {
        al::changeLineAutoStopMode(this, cFirstDemoBgmName, false);
    }

    if (!mIsHideShell) {
        mDisasterFramesSync = calcFramesOfProsperity();
    }

    al::tryOffStageSwitch(this, "DisasterTransitionOn");
    al::tryOnStageSwitch(this, "AnticipationOn");
    mDisasterFrames = 0;
    if (mShell != nullptr && mpSuperBowser != nullptr) {
        if (mIsHideShell) {
            onShellLaunched(this);
        } else {
            mShell->launch(&onShellLaunched, this);
        }
    }

    if (!mIsSkipAnticipationFade) {
        mGraphicsAreaController->pauseFade();
    }

    onShellLaunched(this);
    onShellLaunched(this);
}

/**
 * @brief Called once the Black Sun launched Fury Bowser.
 * @param pController The controller.
 */
void DisasterModeController::onShellLaunched(void* pController) {
    auto* controller = static_cast<DisasterModeController*>(pController);
    controller->mShell->forceHide();
    al::setNerve(controller, &NrvDisasterModeControllerDisasterTransitionWipeIn);
}

/**
 * @brief Jump into the disaster without any transition.
 */
void DisasterModeController::exeDisasterTransitionInstant() {
    if (!al::isStep(this, 0)) {
        return;
    }

    al::tryOffStageSwitch(this, "SwitchKillOn");
    al::tryOnStageSwitch(this, "SwitchAppearOn");
    al::tryOffStageSwitch(this, "DisasterOn");
    al::tryOffStageSwitch(this, "DisasterLastPhase3On");
    mDisasterFrames = 0;
    mGraphicsAreaController->triggerFadeOverFramesTo(1);
    mGraphicsAreaController->setLerp(1.0f);
    mDisasterFramesSync = calcCycleFrames();
    setSkyDisaster();
    if (!mIsPreDisasterModeDone) {
        preDisasterMode();
    }

    appearSuperBowser();
    al::setNerve(this, &NrvDisasterModeControllerDisaster);
    if (isLastBowserBattle(this)) {
        startChaseBgm();
    }

    al::setNerve(this, &NrvDisasterModeControllerDisaster);
}

/**
 * @brief Start the music of the final chase.
 */
void DisasterModeController::tryStartChaseMusic() {
    startBgmNoFade(this, "DisasterKoopaChase");
}

/**
 * @brief Wipe into the disaster.
 */
void DisasterModeController::exeDisasterTransitionWipeIn() {
    if (al::isStep(this, 0)) {
        changeState(State::DisasterTransitionStart);
        mDisasterFrames = 0;
        mDisasterFramesSync = calcCycleFrames();
        mWipeLayout->appear();
        if (isLastBowserBattle(this)) {
            startChaseBgm();
        } else {
            startBgmRequest(BGM_REQUEST_DISASTER, false);
        }

        IslandMap::setIslandMapEnable(this, false);
        al::tryStartAction(mWipeLayout, "DisasterStartInPlain", nullptr);
        al::startSe(mpSuperBowser, "Thunder", nullptr);
    }

    if (!al::isActionEnd(mWipeLayout, nullptr)) {
        return;
    }

    mGraphicsAreaController->triggerFadeOverFramesTo(1);
    mGraphicsAreaController->setLerp(1.0f);
    al::tryOffStageSwitch(this, "SwitchKillOn");
    al::tryOnStageSwitch(this, "SwitchAppearOn");
    al::tryOffStageSwitch(this, "DisasterOn");
    al::tryOffStageSwitch(this, "DisasterLastPhase3On");
    setSkyDisaster();
    tryChangeActorWetMaterial(true);
    mIsDisasterModeAnim = true;
    changeState(State::Disaster);
    al::setNerve(this, &NrvDisasterModeControllerDisasterTransitionWipeOut);
}

/**
 * @brief Wipe out of the transition, Fury Bowser appears.
 */
void DisasterModeController::exeDisasterTransitionWipeOut() {
    if (al::isStep(this, 0)) {
        changeState(State::DisasterTransitionEnd);
        mDisasterFrames = 0;
        mDisasterFramesSync = calcCycleFrames();
        mWipeLayout->appear();
        al::tryStartAction(mWipeLayout, "DisasterStartOutPlain", nullptr);
        appearSuperBowser();
    }

    if (al::isActionEnd(mWipeLayout, nullptr)) {
        mWipeLayout->kill();
        al::setNerve(this, &NrvDisasterModeControllerDisaster);
    }
}

/**
 * @brief Wipe into the disaster during the first disaster cutscene.
 */
void DisasterModeController::exeDisasterTransitionWipeInDemo() {
    if (al::isStep(this, 0)) {
        changeState(State::DisasterTransitionStart);
        mDisasterFrames = 0;
        mGraphicsAreaController->triggerFadeOverFramesTo(1);
        mGraphicsAreaController->setLerp(1.0f);
        mDisasterFramesSync = calcCycleFrames();
        mWipeLayout->appear();
        al::tryStartAction(mWipeLayout, "DisasterStartInPlain", nullptr);
    }

    if (!al::isActionEnd(mWipeLayout, nullptr)) {
        return;
    }

    al::tryOffStageSwitchInstant(this, "DemoStartOn");
    al::tryOffStageSwitch(this, "SwitchKillOn");
    al::tryOnStageSwitch(this, "SwitchAppearOn");
    al::tryOffStageSwitchInstant(this, "DisasterOn");
    al::tryOffStageSwitch(this, "DisasterLastPhase3On");
    setSkyDisaster();
    mIsDisasterModeAnim = true;
    changeState(State::Disaster);
    al::setNerve(this, &NrvDisasterModeControllerDisasterTransitionWipeOutDemo);
}

/**
 * @brief Wipe out of the transition during the first disaster cutscene.
 */
void DisasterModeController::exeDisasterTransitionWipeOutDemo() {
    if (al::isStep(this, 0)) {
        changeState(State::DisasterTransitionEnd);
        mDisasterFrames = 0;
        mGraphicsAreaController->triggerFadeOverFramesTo(1);
        mGraphicsAreaController->setLerp(1.0f);
        mDisasterFramesSync = calcCycleFrames();
        mWipeLayout->appear();
        al::tryStartAction(mWipeLayout, "DisasterStartOutPlain", nullptr);
        tryChangeActorWetMaterial(true);
    }

    if (al::isActionEnd(mWipeLayout, nullptr)) {
        mWipeLayout->kill();
        al::setNerve(this, &NrvDisasterModeControllerNoOp);
    }
}

/**
 * @brief Fury Bowser rampages.
 */
void DisasterModeController::exeDisaster() {
    if (al::isStep(this, 0)) {
        IslandMap::setIslandMapEnable(this, true);
        changeState(State::DisasterStart);
        mDisasterDurationFrames = mFlowNodes[mFlowIndex].disasterFrames;
        startLightning();
        startRain();
        al::tryOffStageSwitch(this, "ProsperityTransitionOn");
        al::tryOnStageSwitch(this, "DisasterOn");
        if (mpSuperBowser != nullptr && mpSuperBowser->isLastPhase3Bowser()) {
            al::tryOnStageSwitch(this, "DisasterLastPhase3On");
        }

        mSpikeDirector->begin();
        mIsDisasterMode = true;
        mIsDisasterModeAnim = true;
        mDisasterFrames = 0;
        mDisasterElapsedFrames = 0;
        _1d0 = 0;
        mGraphicsAreaController->setLerpStep(-1);
        mIsProsperityTransitionNeeded = true;
        mIsDisasterForeshadow = false;
        SingleModeDataFunction::setAutoForeshadow(GameDataHolderAccessor(this), false);
        SingleModeDataFunction::recordDisasterMode(
            GameDataHolderAccessor(this), SingleModeDataFunction::DisasterForceSetting_Off, this);
        if (mpSuperBowser != nullptr) {
            mpSuperBowser->setUseSecondaryFlow(mFlowNodes[mFlowIndex].isMini);
        }

        auto* raidonSurf = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidonSurf != nullptr) {
            raidonSurf->updateSpawns(false);
        }

        auto* gigaBellManager =
            al::tryGetSceneObj<GigaBellManager>(this, SceneObjID_GigaBellManager);
        if (gigaBellManager != nullptr) {
            gigaBellManager->disasterModeStart();
        }

        tryChangeActorWetMaterial(true);
        if (mIsInFirstAppearDemo) {
            mGraphicsAreaController->setLerpStep(1);
            if (rc::isAnyActiveDemo(this)) {
                if (rc::isActiveDemoCutscene(this)) {
                    rc::requestEndDemoCutscene(this);
                    rc::setImmediateSwitchFlag(this);
                } else {
                    rc::requestEndDemoInGameCutscene(this);
                }
            }

            rc::requestStartDemoInGameCutscene(this);
            rc::setDemoFullEffectUpdate(this, true);
            mActorSceneInfo->cameraDirector->freezeCameraInput(true);
            al::addDemoActor(this);
            al::addDemoActor(mpSuperBowser);
            mpSuperBowser->activateCamera();
        }

        if (isLastBowserBattle(this)) {
            mSkyLake->kill();
            mSkyDisaster->kill();
            mSkyDisasterHard->kill();
            mSkyDisasterSuperHard->appear();
            mGraphicsAreaController->switchToSuperHardDisaster();
        }
    }

    sead::Vector3f cameraPos =
        getCameraDirector_RS()->getSceneCameraInfo()->getViewAt(0)->getLookAtCam().getPos();
    SuperBowserLaserState* laserState = mpSuperBowser->getLaserState();
    if (rc::isInPlessieTunnel(this, cameraPos)) {
        if (laserState != nullptr && laserState->isEnableLaserLightEffects()) {
            mpSuperBowser->enableLaserLightEffects(false);
        }
    } else {
        if (laserState != nullptr && !laserState->isEnableLaserLightEffects()) {
            mpSuperBowser->enableLaserLightEffects(true);
        }
    }

    mDisasterElapsedFrames++;
    if (mFreezeFrames >= 1) {
        if (--mFreezeFrames == 0) {
            rc::requestEndDemoInGameCutscene(this);
            mActorSceneInfo->cameraDirector->freezeCameraInput(false);
            mIsInFirstAppearDemo = false;
        }

        if (al::isStep(this, 1)) {
            mpSuperBowser->deactivateCamera();
        }
    }

    if (al::isStep(this, 100)) {
        notifyStateChange(State::DisasterLong);
    }

    if (mpSuperBowser != nullptr && !mpSuperBowser->isDoingEndingPreparations() &&
        mpSuperBowser->isLeaveRequested()) {
        mIsBlackSunFloating = true;
        mpSuperBowser->requestDisappear(&onBowserDisappearStart, &onBowserDisappeared, this,
                                        false);
    }

    mSpikeDirector->update();
    if (mIsLightning) {
        updateLightning();
    }

    if (mIsBlackSunFloating || mIsNeverEnd || mMode == Mode_Hard || mMode == Mode_SuperHard) {
        return;
    }

    if (mDisasterFrames < mDisasterDurationFrames) {
        mDisasterFrames = sead::Mathi::min(mDisasterDurationFrames, mDisasterFrames + 1);
        mDisasterFramesSync++;
    }

    if (mpSuperBowser != nullptr && mFlowNodes[mFlowIndex].isMini &&
        mDisasterFrames >= mDisasterDurationFrames) {
        mpSuperBowser->cancelFlow();
    }
}

/**
 * @brief Start the lightning strikes.
 */
void DisasterModeController::startLightning() {
    mLightningTimer = 0;
    randomizeLightningInterval();
    mIsLightning = true;
}

/**
 * @brief Called once Fury Bowser starts to disappear.
 * @param pController The controller.
 */
void DisasterModeController::onBowserDisappearStart(void* pController) {}

/**
 * @brief Strike lightning around the player at random intervals.
 */
void DisasterModeController::updateLightning() {
    if (rc::isAnyActiveDemo(this)) {
        return;
    }

    if (mLightningTimer++ < mLightningInterval) {
        return;
    }

    mLightningTimer = 0;
    randomizeLightningInterval();

    al::LiveActor* player = al::tryFindNearestPlayerActor(this);
    sead::Vector3f pos = sead::Vector3f::zero;
    if (player != nullptr) {
        const sead::Vector3f& playerTrans = al::getTrans(player);
        sead::Vector2f offset = sead::Vector2f::zero;
        pos.set(playerTrans.x, 0.0f, playerTrans.z);
        al::getRandomOnCircle(&offset, 80000.0f);
        pos += sead::Vector3f(offset.x, 0.0f, offset.y);
    }

    s32 index = -1;
    for (s32 i = 0; i < mLightningNum; i++) {
        if (al::isDead(mLightnings[i])) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        return;
    }

    DisasterLightning* lightning = mLightnings[index];
    if (lightning == nullptr) {
        return;
    }

    al::setTrans(lightning, pos);
    if (mSkyDisasterSuperHard != nullptr && al::isAlive(mSkyDisasterSuperHard)) {
        lightning->appear(2);
    } else if (mSkyDisasterHard != nullptr && al::isAlive(mSkyDisasterHard)) {
        lightning->appear(1);
    } else {
        lightning->appear(0);
    }
}

/**
 * @brief Do nothing.
 */
void DisasterModeController::exeNoOp() {
    // The original only queries the step, without acting on it.
    al::isStep(this, 0);
}

/**
 * @brief Peace given after a boss battle before the disaster cycle resumes.
 */
void DisasterModeController::exePostBattlePeaceDelay() {
    if (mPostBossPeaceFrames < 1 ||
        (mpSuperBowser != nullptr && mpSuperBowser->isLastPhase3Bowser())) {
        al::setNerve(this, &NrvDisasterModeControllerProsperity);
        return;
    }

    if (mFreezeFrames >= 1 && --mFreezeFrames == 0) {
        rc::requestEndDemoInGameCutscene(this);
        mActorSceneInfo->cameraDirector->freezeCameraInput(false);
        mIsInDisappearDemo = false;
    }

    if (rc::isActiveDemo(this)) {
        return;
    }

    mPostBossPeaceFrames--;
    SingleModeDataFunction::setDisasterModePostBossPeaceFrames(this, mPostBossPeaceFrames);
}

/**
 * @brief Check whether the disaster is in progress.
 * @return True during the disaster.
 */
bool DisasterModeController::isInDisasterOrInstant() const {
    if (mIsDisasterMode) {
        return true;
    }

    if (al::isNerve(this, &NrvDisasterModeControllerDisasterTransitionInstant)) {
        return true;
    }

    return al::isNerve(this, &NrvDisasterModeControllerDisaster);
}

/**
 * @brief Turn the anticipation switch on and start the music announcing Fury Bowser.
 */
void DisasterModeController::triggerAnticipationSwitch() {
    al::tryOnStageSwitch(this, "AnticipationTransitionOn");
    tryStartAnticipationMusic(false, false);
}

/**
 * @brief Jump to the foreshadowing of the next disaster.
 */
void DisasterModeController::jumpToForeshadow() {
    if (al::isNerve(this, &NrvDisasterModeControllerProsperity)) {
        al::setNerve(this, &NrvDisasterModeControllerProsperity);
    } else {
        endInstantly(true, true);
    }

    mIsJumpToForeshadow = true;
    stopForeshadowBgm();
}

/**
 * @brief Jump to the rain that precedes the next disaster, with a white flash if needed.
 */
void DisasterModeController::tryJumpToRainWithFlash() {
    if (mDisasterFrames <= calcFramesOfProsperity() - mShell->getRainFrames()) {
        if (mpSuperBowser != nullptr && !mpSuperBowser->isHidden()) {
            mpSuperBowser->disappear(false);
        }

        clearPostBossPeaceFrames();
        if (mIsForeshadowOff) {
            mIsJumpToRain = true;
        }
    } else {
        mIsTimeJumped = true;
        mIsProsperityTransitionNeeded = true;
        mFadeInDoneFunctor = mJumpToRainFunctor;
        al::setNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeIn);
    }
}

/**
 * @brief Pick the frames before the next lightning strike.
 */
void DisasterModeController::randomizeLightningInterval() {
    mLightningInterval = al::getRandom(40, 55);
}

/**
 * @brief Stop the lightning strikes.
 */
void DisasterModeController::stopLightning() {
    mIsLightning = false;
}

/**
 * @brief Show the sky of the prosperity.
 */
inline void DisasterModeController::showProsperitySky() {
    mIsSkyDisaster = false;
    if (!mIsSkyEnable) {
        return;
    }

    if (mSkyLake != nullptr && mSkyDisaster != nullptr) {
        mSkyLake->appear();
        mSkyDisasterHard->kill();
        mSkyDisasterSuperHard->kill();
        mSkyDisaster->kill();
    }

    if (mSkyFinalCutscene != nullptr) {
        mSkyFinalCutscene->kill();
    }
}

/**
 * @brief End the disaster and go back to the prosperity.
 */
inline void DisasterModeController::endDisasterPhase() {
    mIsProsperityTransitionNeeded = true;
    stopLightning();
    stopRain();
    if (!mIsForeshadowOffForced) {
        tryIncrementFlow();
    }

    al::setNerve(this, &NrvDisasterModeControllerProsperityTransitionWipeIn);
    _249 = false;
    mIsEnding = false;
    mIsBlackSunFloating = false;
}

/**
 * @brief Stop the music announcing Fury Bowser.
 */
inline void DisasterModeController::stopForeshadowBgm() {
    if (mIsDisasterForeshadow) {
        al::stopBgm(this, "BeforeDisasterKoopa", 10, -1);
        al::stopBgm(this, "BeforeDisasterKoopaShort", 10, -1);
        al::changeLineAutoStopMode(this, cFirstDemoBgmName, false);
        mIsDisasterForeshadow = false;
    }
}

/**
 * @brief Start the music of the final chase.
 */
inline void DisasterModeController::startChaseBgm() {
    al::startBgm(this, al::BgmPlayingRequest("DisasterKoopaChase", -1, 0, 0));
}

/**
 * @brief Make Fury Bowser appear if he is not out yet.
 */
inline void DisasterModeController::appearSuperBowser() {
    if (mpSuperBowser != nullptr && !mpSuperBowser->isActive()) {
        mpSuperBowser->appear();
        mPlessieTunnelArea = al::tryFindPlessieTunnelAreaObj(this, "GraphicsArea");
    }
}

/**
 * @brief Check whether the Giga Bell lock count was increased in the current phase.
 * @return True once the lock count was increased.
 */
inline bool DisasterModeController::isGigaBellLockCountIncremented() const {
    if (SingleModeDataFunction::getUnlockedPhase(this) != 5) {
        return false;
    }

    auto* manager = al::tryGetSceneObj<GigaBellManager>(this, SceneObjID_GigaBellManager);
    return manager != nullptr && manager->hasLockCountBeenIncremented();
}

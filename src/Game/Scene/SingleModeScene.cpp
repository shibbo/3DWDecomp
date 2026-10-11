#include "Scene/SingleModeScene.hpp"

#include <nn/oe.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Bgm/PlayerBigBgmController.hpp"
#include "Boss/SuperBowser.hpp"
#include "Boss/SuperBowserShell.hpp"
#include "Bgm/PlayerInvincibleBgmController.hpp"
#include "Camera/CameraPoserFactory.hpp"
#include "Camera/PlayerCameraTarget.hpp"
#include "Demo/DemoCutscene.hpp"
#include "Demo/DemoPlayerModelDirector.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Demo/StageStartEventBase.hpp"
#include "Demo/StageStartPhase0Demo.hpp"
#include "Layout/CameraChangeLayout.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/PauseMenu.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/IslandMap.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Layout/WindowProcessing.hpp"
#include "Layout/Switch/SnapshotLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/AudioDirector.hpp"
#include "Library/Base/HashCodeUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/CameraPoserSceneInfo_RS.hpp"
#include "Library/Clipping/ClippingDirectorBase.hpp"
#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Execute/ExecuteDirector.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Shadow/ShadowDirector.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/MapObj/FixMapParts.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveStateCtrl.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Nfp/NfpDirector.hpp"
#include "Library/Obj/FootPrintServer.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Screen/ScreenCoverCtrl.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraTurnInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Camera/SceneCameraCtrl.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/Stage/StageResourceKeeper.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/BoxPropeller.hpp"
#include "MapObj/ChikaChikaBlockSynchronizer.hpp"
#include "MapObj/CoinRotater.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcAssistDirectorList.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/Fury/InkPatchSpecial.hpp"
#include "MapObj/IslandAreaWatcher.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/OceanWater.hpp"
#include "MapObj/OceanWaterDeferred.hpp"
#include "MapObj/ScoreHolder.hpp"
#include "MapObj/PlayerCrown.hpp"
#include "MapObj/ShardsWatcherHolder.hpp"
#include "MapObj/SingleModeCheckpoint.hpp"
#include "MapObj/StampDirector.hpp"
#include "NPC/IslandHolder.hpp"
#include "NPC/ShadowMarioDirector.hpp"
#include "Player/FurEnv.hpp"
#include "Player/Giga/PlayerActionGraphBuilder.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerGroupSceneObj.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Player/PlayerCooperation.hpp"
#include "Player/PlayerFireBallAppearWatcher.hpp"
#include "Player/PlayerProcess.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Camera/Holder/CameraResourceHolder.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/LiveActor/ActorExecuteFunction.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/PlayerStocker.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Camera/IntroFlyOverCamera.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneEventMessageSender.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "Scene/IslandWarpState.hpp"
#include "Scene/SingleModeDrawer3D.hpp"
#include "Scene/SnapshotState.hpp"
#include "System/Application.hpp"
#include "System/Data/OceanScenarioList.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/GameDataFile.hpp"
#include "System/IslandDataList.hpp"
#include "System/PlayLogFunction.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "System/ScenarioList.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoActorGroupUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/SceneUtil.hpp"

/**
 * Declares a SingleModeScene nerve whose execute function has a different name than the nerve.
 * @param Action The nerve name.
 * @param Func The SingleModeScene::exe* function the nerve runs.
 */
#define SINGLE_MODE_SCENE_NERVE(Action, Func)                                                     \
    class SingleModeSceneNrv##Action : public al::Nerve {                                          \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<SingleModeScene>()->exe##Func();                                    \
        }                                                                                          \
    };

/**
 * Defines a constant SingleModeScene nerve object, which the game keeps next to its vtable.
 * @param Action The nerve name.
 */
#define SINGLE_MODE_SCENE_NERVE_CONST(Action)                                                      \
    const SingleModeSceneNrv##Action NrvSingleModeScene##Action{};

/**
 * Defines a SingleModeScene nerve object, which the game keeps with the other non-constant
 * nerves after the vtables.
 * @param Action The nerve name.
 */
#define SINGLE_MODE_SCENE_NERVE_MAKE(Action) SingleModeSceneNrv##Action NrvSingleModeScene##Action{};

namespace {
NERVE_DECL(SingleModeScene, CaptureMode)
NERVE_DECL(SingleModeScene, Start)
NERVE_DECL(SingleModeScene, StartPhase0Demo)
NERVE_DECL(SingleModeScene, Restart)
NERVE_DECL(SingleModeScene, Goal)
NERVE_DECL(SingleModeScene, ReenterStage)
NERVE_DECL(SingleModeScene, Retire)
SINGLE_MODE_SCENE_NERVE(GameChange, GameEnd)
SINGLE_MODE_SCENE_NERVE(LoadGame, GameEnd)
NERVE_DECL(SingleModeScene, DemoIntro)
NERVE_DECL(SingleModeScene, Play)
SINGLE_MODE_SCENE_NERVE(PhaseEnd, GameEnd)
NERVE_DECL(SingleModeScene, DemoChangePlayer)
NERVE_DECL(SingleModeScene, DemoCamera)
NERVE_DECL(SingleModeScene, DemoMovingCamera)
NERVE_DECL(SingleModeScene, DemoScene)
NERVE_DECL(SingleModeScene, DemoCutscene)
NERVE_DECL(SingleModeScene, DemoInGameCutscene)
NERVE_DECL(SingleModeScene, DemoPlayerScene)
NERVE_DECL(SingleModeScene, DemoFadeTransition)
NERVE_DECL(SingleModeScene, DemoKoopaJrPhaseIntro)
NERVE_DECL(SingleModeScene, Pause)
NERVE_DECL(SingleModeScene, PauseIslandMap)
NERVE_DECL(SingleModeScene, PauseItemSelect)
NERVE_DECL(SingleModeScene, PauseWindowMessage)
NERVE_DECL(SingleModeScene, WaitForKoopaJrOptionsIntro)
SINGLE_MODE_SCENE_NERVE(DemoIslandMapUnlock, DemoIslandMap2Part)
SINGLE_MODE_SCENE_NERVE(DemoIslandMapUnlockPreGame, DemoIslandMap2Part)
NERVE_DECL(SingleModeScene, DemoIslandMap)
SINGLE_MODE_SCENE_NERVE(DemoFadeTransitionToIslandMap, DemoFadeTransition)
SINGLE_MODE_SCENE_NERVE(DemoFadeTransitionFromIslandMap, DemoFadeTransition)
NERVE_DECL(SingleModeScene, DemoIslandMapPreGame)
NERVE_DECL(SingleModeScene, DemoKoopaJrPhaseIntro2Part)
NERVE_DECL(SingleModeScene, IslandWarp)
NERVE_DECL(SingleModeScene, PreTimeUp)
NERVE_DECL(SingleModeScene, KoopaJrOptionsIntro)

SINGLE_MODE_SCENE_NERVE_MAKE(CaptureMode)
SINGLE_MODE_SCENE_NERVE_MAKE(Start)
SINGLE_MODE_SCENE_NERVE_MAKE(StartPhase0Demo)
SINGLE_MODE_SCENE_NERVE_MAKE(Restart)
SINGLE_MODE_SCENE_NERVE_CONST(Goal)
SINGLE_MODE_SCENE_NERVE_CONST(ReenterStage)
SINGLE_MODE_SCENE_NERVE_MAKE(Retire)
SINGLE_MODE_SCENE_NERVE_MAKE(GameChange)
SINGLE_MODE_SCENE_NERVE_MAKE(LoadGame)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoIntro)
SINGLE_MODE_SCENE_NERVE_MAKE(Play)
SINGLE_MODE_SCENE_NERVE_CONST(PhaseEnd)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoChangePlayer)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoCamera)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoMovingCamera)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoScene)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoCutscene)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoInGameCutscene)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoPlayerScene)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoFadeTransition)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoKoopaJrPhaseIntro)
SINGLE_MODE_SCENE_NERVE_MAKE(Pause)
SINGLE_MODE_SCENE_NERVE_MAKE(PauseIslandMap)
SINGLE_MODE_SCENE_NERVE_MAKE(PauseItemSelect)
SINGLE_MODE_SCENE_NERVE_MAKE(PauseWindowMessage)
SINGLE_MODE_SCENE_NERVE_MAKE(WaitForKoopaJrOptionsIntro)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoIslandMapUnlock)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoIslandMapUnlockPreGame)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoIslandMap)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoFadeTransitionToIslandMap)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoFadeTransitionFromIslandMap)
SINGLE_MODE_SCENE_NERVE_CONST(DemoIslandMapPreGame)
SINGLE_MODE_SCENE_NERVE_MAKE(DemoKoopaJrPhaseIntro2Part)
SINGLE_MODE_SCENE_NERVE_MAKE(IslandWarp)
SINGLE_MODE_SCENE_NERVE_CONST(PreTimeUp)
SINGLE_MODE_SCENE_NERVE_MAKE(KoopaJrOptionsIntro)

/** Names of the ocean quadrants in the ocean scenario list of the ocean stage. */
const char* const cOceanQuadrantNames[] = {"QuadrantNW", "QuadrantNE", "QuadrantSW",
                                           "QuadrantSE"};

/** Unlocked phase of the Plessie chase after the first Fury Bowser fight. */
constexpr s32 cPhasePlessieChase1 = 7;
/** Unlocked phase of the Plessie chase after the second Fury Bowser fight. */
constexpr s32 cPhasePlessieChase2 = 10;
/** Front direction of the main player when a Bowser Jr. cutscene starts. */
sead::Vector3f sPlayerFrontDir;

/** Event type of the start event showing the stage timer. */
constexpr s32 cStartEventTypeTimer = 4;

/**
 * Gets the application's game framework.
 * @return The game framework, or nullptr if the framework is not a GameFrameworkNx.
 */
al::GameFrameworkNx* getFramework() {
    return sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
}

/**
 * Gets the application's game framework, which always exists while a scene is alive.
 * @return The game framework, or nullptr if the framework is not a GameFrameworkNx.
 */
al::GameFrameworkNx* getFrameworkAlive() {
    sead::Framework& framework = *Application::instance()->getFramework();
    return sead::DynamicCast<al::GameFrameworkNx>(&framework);
}

/**
 * Gets the render buffer the framework currently draws to.
 * @param pFramework The game framework.
 * @return The render buffer of the docked or handheld display.
 */
agl::RenderBuffer* getRenderBuffer(const al::GameFrameworkNx* pFramework) {
    return pFramework->getCurrentRenderBuffer();
}

/**
 * Updates the audio demo type of the scene if the demo director changed it.
 * @param pAudioDemoType The audio demo type of the scene.
 * @param pScene The scene.
 */
void updateAudioDemoType(s32* pAudioDemoType, const al::Scene* pScene) {
    if (pScene->getDemoDirector()->isChangedAudioDemoType()) {
        *pAudioDemoType = pScene->getDemoDirector()->getAudioDemoType();
        pScene->getDemoDirector()->resetChangedAudioDemoType();
    }
}

/**
 * Cancels the binding of the player by Plessie when a demo starts.
 * @param pHolder The scene object holder.
 */
void cancelRaidonSurfBind(const al::IUseSceneObjHolder* pHolder) {
    RaidonSurf* raidonSurf = al::tryGetSceneObj<RaidonSurf>(pHolder, SceneObjID_RaidonSurf);

    if (raidonSurf != nullptr) {
        raidonSurf->setBindCanceled(true);
    }
}

/**
 * Sets whether every alive player plays the player change demo.
 * @param pHolder The player holder.
 * @param isChange Whether the change demo plays.
 */
void setPlayerChangeDemoAll(al::PlayerHolder* pHolder, bool isChange) {
    for (s32 i = 0; i < pHolder->getPlayerNum(); i++) {
        auto* player = static_cast<PlayerActor*>(pHolder->getPlayer(i));

        if (player != nullptr && !al::isDead(player)) {
            player->setTitleDemoChange(isChange);
        }
    }
}

/**
 * Sets the camera input sensitivity of the camera director.
 * @param pDirector The camera director.
 * @param sensitivity The sensitivity from the options.
 */
void setCameraSensitivity(al::CameraDirector_RS* pDirector, s32 sensitivity) {
    // The request parameter holder starts with the sensitivity.
    *reinterpret_cast<s32*>(pDirector->getSceneCameraCtrl()->getRequestParamHolder()) = sensitivity;
}
}  // namespace

/**
 * Constructs the single mode scene.
 * @param pName The scene name.
 */
SingleModeScene::SingleModeScene(const char* pName)
    : InGameSceneBase(pName), mIsPrepareDestroyed(false), mIsRequestSave(false), mIsSaving(false),
      mIsNarrowPlace(false), mGameDataHolder(nullptr), mIslandDataList(nullptr), mOceanScenarioList(nullptr),
      mStartEvent(nullptr), mPhase0StartDemo(nullptr),
      mIntroFlyOverCamera(nullptr), mLastCheckpointIndex(-1), mLastCheckpointZoneIndex(-1),
      mUnlockedIslandNum(0), _244(-1), mPlayerCrown(nullptr), mPlayerAliveWatcher(nullptr), mDrcAssistDirectorList(nullptr),
      mInvalidateInputFrames(0), mDemoCutscene(nullptr),
      mWipeFadeBlack(nullptr), mWipeWorldJump(nullptr),
      mSceneLayout(nullptr), mWipeMiss(nullptr), mInvincibleBgmController(nullptr),
      mBigBgmController(nullptr), mPauseMenu(nullptr), mPausePort(-1),
      mScreenCaptureExecutor(nullptr), mNetworkSystem(nullptr), mNfpDirector(nullptr),
      mPlayerCameraTarget(nullptr), _320(-1), _328(nullptr), mStampDirector(nullptr),
      mSnapshotState(nullptr), mSnapshotLayout(nullptr), mUnlockedPhase(0), mIsLastBowserBattle(false),
      mIsPhase0Start(false), mIsInGameCutsceneSwitch(false), mDrawer3D(nullptr), mIsGigaBellUnlockCutscene(false) {
    mWindowProcessing = nullptr;
    mGuideGameWindow = nullptr;
    mShadowMarioDirector = nullptr;
    mAudioDemoType = 0;
    mRaidonSurf = nullptr;
    mIslandMap = nullptr;
    mIslandDataList = nullptr;
    mDemoBaseMtx.makeIdentity();
    mViewport.set(0.0f, 0.0f, 1280.0f, 720.0f);
    mIslandKeeper = new IslandKeeper(256);
    mShardsWatcherHolder = new ShardsWatcherHolder(48);
    mIsUseCameraRS = true;
}

/**
 * Checks whether a scenario of an island is complete.
 * @param scenarioNo The island number (1-based).
 * @param shineNum The scenario number (1-based).
 * @return Whether the scenario is complete.
 */
bool SingleModeScene::isScenarioComplete(s32 scenarioNo, s32 shineNum) {
    if (shineNum < 1) {
        return false;
    }

    return SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(mGameDataHolder),
                                                      scenarioNo - 1, shineNum - 1);
}

/**
 * Destroys the single mode scene and restores the settings changed by the scene.
 */
SingleModeScene::~SingleModeScene() {
    finishSave();
    mGameDataHolder->setPlayerHolder(nullptr);

    if (al::isNerve(this, &NrvSingleModeSceneCaptureMode)) {
        al::ClippingDirectorBase::sLODDisabled = false;
        al::ClippingDirectorBase::sCollisionForcedOn = false;
        mSnapshotState->kill();
    }

    nn::oe::SetAlbumImageOrientation(nn::album::ImageOrientation_None);
    SingleModeDataFunction::calculateAllShineCollected(GameDataHolderAccessor(mGameDataHolder));
    al::GameFrameworkNx* framework = getFrameworkAlive();
    framework->_27c = true;
    framework->_27b = false;
    rc::setMainPlayerActor(nullptr);
    mLiveActorKit->getEffectSystem()->endScene();

    if (mNfpDirector != nullptr) {
        mNfpDirector->stop(true);
    }

    getFrameworkAlive()->mIsClearRenderBuffer = true;
    mGameDataHolder->setIslandDataList(nullptr);
    mGameDataHolder->setOceanScenarioList(nullptr);

    if (mStampDirector != nullptr) {
        delete mStampDirector;
        mStampDirector = nullptr;
    }
}

/**
 * Waits for the save started by the scene to finish.
 */
void SingleModeScene::finishSave() {
    while (mIsSaving) {
        if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
            mIsSaving = false;
        }
    }
}

/**
 * Creates the island data list from the island list of the main stage.
 */
void SingleModeScene::initIslandDataList() {
    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);
    al::PlacementInfo listInfo;
    s32 count = 0;
    bool isExist = al::tryGetPlacementInfoAndCount(&listInfo, &count, stageInfo, "IslandList");

    if (isExist && count >= 1) {
        mIslandDataList = new IslandDataList(count + 1);

        for (s32 i = 0; i < count; i++) {
            al::PlacementInfo placementInfo;
            if (!al::tryGetPlacementInfoByIndex(&placementInfo, listInfo, i)) {
                continue;
            }

            if (!isValidPlacement(placementInfo)) {
                continue;
            }

            const char* objectName = nullptr;
            if (!al::tryGetObjectName(&objectName, placementInfo)) {
                continue;
            }

            s32 zoneId = -1;
            if (!placementInfo.getPlacementIter().tryGetIntByKey(&zoneId, "ZoneID")) {
                continue;
            }

            const al::StageInfo* islandStageInfo = al::findStageInfo(this, 0, objectName);
            if (islandStageInfo != nullptr) {
                mIslandDataList->RegisterIslandData(islandStageInfo, zoneId);
            }
        }

        mIslandDataList->endInit();
    } else {
        mIslandDataList = new IslandDataList();
    }

    mGameDataHolder->setIslandDataList(mIslandDataList);
}

/**
 * Initializes the resource keeper of the ocean stage; the base scene has none.
 */
void SingleModeScene::initOceanResourceKeeper() {}

/**
 * Finds the stage holding the ocean scenarios.
 * @return The main stage.
 */
const al::StageInfo* SingleModeScene::findOceanStageInfo() {
    return al::getStageInfoMap(this, 0);
}

/**
 * Creates the scenario lists of the ocean quadrants from the ocean scenarios of the ocean stage.
 */
void SingleModeScene::initOceanScenarios() {
    al::ByamlIter rootIter(findOceanStageInfo()->getResource()->tryGetByml("OceanScenarios"));
    al::ByamlIter quadrantIter;
    al::ByamlIter scenarioIter;
    const char* displayName = "";
    s32 quadrantNum = rootIter.getSize();
    mOceanScenarioList = new OceanScenarioList(quadrantNum);
    al::setSceneObj(this, mOceanScenarioList, SceneObjID_OceanScenarioList);

    for (s32 i = 0; i < quadrantNum; i++) {
        rootIter.tryGetIterByKey(&quadrantIter, cOceanQuadrantNames[i]);
        s32 scenarioNum = quadrantIter.getSize();
        mOceanScenarioList->addList(new ScenarioList(scenarioNum));

        for (s32 j = 0; j < scenarioNum; j++) {
            quadrantIter.tryGetIterByIndex(&scenarioIter, j);
            const char* scenarioName = "MissingName";
            const char* scenarioType = "InvalidType";
            displayName = "";
            s32 scenarioId = -1;
            s32 unlockScenarioId = -1;
            bool isDisasterDisable = false;
            bool isUpdateLevelData = false;
            scenarioIter.tryGetIntByKey(&scenarioId, "ScenarioID");
            scenarioIter.tryGetStringByKey(&scenarioName, "ScenarioName");
            scenarioIter.tryGetStringByKey(&displayName, "ScenarioDisplayName");
            scenarioIter.tryGetStringByKey(&scenarioType, "ScenarioType");
            scenarioIter.tryGetIntByKey(&unlockScenarioId, "UnlockScenarioID");
            scenarioIter.tryGetBoolByKey(&isDisasterDisable, "DisasterDisable");
            scenarioIter.tryGetBoolByKey(&isUpdateLevelData, "UpdateLevelData");
            OceanScenarioList::tryGetOceanScenarioList(this, i)
                ->getScenarioDataByIndex(j)
                ->init(scenarioId, scenarioName, displayName, scenarioType, unlockScenarioId,
                       isUpdateLevelData, isDisasterDisable);
        }
    }

    mGameDataHolder->setOceanScenarioList(mOceanScenarioList);
}

/**
 * Checks whether a placement list is valid in the current phase.
 * @param rInfo The placement info of the list.
 * @return Whether the list is placed.
 */
bool SingleModeScene::isValidPlacementParent(const al::PlacementInfo& rInfo) const {
    if (mUnlockedPhase == cPhasePlessieChase2 || mUnlockedPhase == cPhasePlessieChase1) {
        bool isDisablePlessieChase = false;
        if (al::tryGetArg(&isDisablePlessieChase, rInfo, "isDisablePlessieChase") &&
            isDisablePlessieChase) {
            return false;
        }
    }

    return SingleModeDataFunction::isValidPlacement(al::tryGetLayerIDbyParents(rInfo),
                                                    mUnlockedPhase, mIsLastBowserBattle);
}

/**
 * Checks whether a placement is valid in the current phase.
 * @param rInfo The placement info.
 * @return Whether the object is placed.
 */
bool SingleModeScene::isValidPlacement(const al::PlacementInfo& rInfo) const {
    if (mUnlockedPhase == cPhasePlessieChase2 || mUnlockedPhase == cPhasePlessieChase1) {
        bool isDisablePlessieChase = false;
        if (al::tryGetArg(&isDisablePlessieChase, rInfo, "isDisablePlessieChase") &&
            isDisablePlessieChase) {
            return false;
        }
    }

    return SingleModeDataFunction::isValidPlacement(al::tryGetLayerID(rInfo), mUnlockedPhase,
                                                    mIsLastBowserBattle);
}

/**
 * Creates the 3D view renderer of the scene.
 * @param pInfo The graphics system info.
 * @return The view renderer.
 */
al::ViewRenderer* SingleModeScene::createViewRenderer(al::GraphicsSystemInfo* pInfo) {
    mDrawer3D = new SingleModeDrawer3D(pInfo);
    return mDrawer3D;
}

/**
 * Deletes the 3D view renderer of the scene.
 * @param pRenderer Unused.
 */
void SingleModeScene::deleteViewRenderer(al::ViewRenderer* pRenderer) {
    if (mDrawer3D != nullptr) {
        delete mDrawer3D;
        mDrawer3D = nullptr;
    }
}

/**
 * Initializes the scene: stage resources, kits, scene objects, layouts, placement and the
 * start nerve.
 * @param rInfo The scene init info.
 */
void SingleModeScene::init(const al::SceneInitInfo& rInfo) {
    rInfo.mSceneName.cstr();
    mStageName = rInfo.mStageName;
    const char* stageName = mStageName.cstr();
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mGameDataHolder->initIsPhase0();
    mGameDataHolder->setUnknownFlags61And62(false, false);
    mUnlockedPhase = SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder));
    mIsLastBowserBattle = DisasterModeController::isLastBowserBattle(GameDataHolderAccessor(mGameDataHolder));
    mGameDataHolder->setSingleMode(true);
    mGameDataHolder->setMapEnabled(true);
    rInfo.mGameSystemInfo->getGamePadSystem()->setMaxNpadNum(2);
    al::initRandomSeed(al::calcHashCode(stageName));
    al::initRandomSeedNonSync(al::calcHashCode(stageName));
    initOceanResourceKeeper();
    initAndLoadStageResource(stageName, 1);
    initIslandDataList();
    al::tryRequestPreLoadFile(this, rInfo, 1, nullptr);
    mNetworkSystem = nullptr;
    mNfpDirector = rInfo.mGameSystemInfo->getNfpDirector();
    initSceneStopCtrl();
    initScreenCoverCtrl();
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    initOceanScenarios();
    al::setSceneObj(this, mIslandDataList, SceneObjID_IslandDataList);
    al::setSceneObj(this, mIslandKeeper, SceneObjID_IslandKeeper);
    al::setSceneObj(this, mShardsWatcherHolder, SceneObjID_ShardsWatcherHolder);
    al::createSceneObj(this, SceneObjID_ScoreHolder);
    al::createSceneObj(this, SceneObjID_GreenStarKeeper);
    al::createSceneObj(this, SceneObjID_IllustItemKeeper);
    al::createSceneObj(this, SceneObjID_ControllerEventWatcher);
    auto* fireBallWatcher = static_cast<PlayerFireBallAppearWatcher*>(
        al::createSceneObj(this, SceneObjID_PlayerFireBallAppearWatcher));
    mDrcAssistDirectorList = new DrcAssistDirectorList(4);
    al::setSceneObj(this, mDrcAssistDirectorList, SceneObjID_DrcAssistDirectorList);
    al::createSceneObj(this, SceneObjID_CloudBonusWatcher);
    al::createSceneObj(this, 57);  // Unknown scene object.
    al::createSceneObj(this, SceneObjID_LuckyIslandHolder);

    for (s32 i = 0; i < 4; i++) {
        mDrcAssistDirectorList->addTouchAssist(al::getPlayerControllerPort(i), isBossScene());
    }

    if (!isBossScene()) {
        mDrcAssistDirectorList->setEnable(false);
    }

    al::setSceneObj(this, mGameDataHolder, SceneObjID_GameDataHolder);
    PlayerStockerFunction::createPlayerStockerSingleMode(this, 4);
    mViewport = *al::getDisplayViewport();
    initSceneAudio(rInfo, mStageName.cstr(), 540, 540, 1, true, "SingleModeScene", 20, 1.0f);

    al::GraphicsInitArg graphicsArg;
    graphicsArg.mViewRendererCreator = new al::ViewRendererCreator();
    graphicsArg.mFar = 100.0f;
    graphicsArg._f = true;
    graphicsArg.mIsStereo = false;
    graphicsArg.mIsUsingViewRenderer = true;
    graphicsArg.setViewNum(2);
    initLiveActorKitWithGraphics(
        graphicsArg, rInfo, 19486, 64, 2, 10000, true,
        al::getStageInfoMap(this, 0)->getResource()->tryGetByml("InitClipping") != nullptr);
    mIslandAreaWatcher = new IslandAreaWatcher(mLiveActorKit->getPlayerHolder());
    al::setSceneObj(this, mIslandAreaWatcher, SceneObjID_IslandAreaWatcher);
    mLiveActorKit->getClippingDirector()->setScreenCoverFrames(&mScreenCoverCtrl->mCoverFrames);
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setSingleMode(true);
    mLiveActorKit->initHitSensorDirector(4, true);

    auto* messageSender = new SceneEventMessageSender();
    messageSender->setActorGroup(mLiveActorKit->getActorGroup());
    al::setSceneObj(this, messageSender, SceneObjID_SceneEventMessageSender);

    if (fireBallWatcher != nullptr) {
        fireBallWatcher->setGraphicsSystemInfo(mLiveActorKit->getGraphicsSystemInfo());
    }

    auto* demoDirector = new ProjectDemoDirector(mLiveActorKit->getPlayerHolder(), 1024);
    demoDirector->getPlayerModelDirector()->requestCreateAllFigure(0, "SMDemo", false);
    mLiveActorKit->setDemoDirector(demoDirector);
    al::setSceneObj(this, new PlayerCooperation(64), SceneObjID_PlayerCooperation);
    al::setSceneObj(this, new FurEnv(mLiveActorKit->getGraphicsSystemInfo()), SceneObjID_FurEnv);
    mDrcAssistDirectorList->setKinopioBrigadeFlag(false);
    mDrcAssistDirectorList->setPlayerHolder(mLiveActorKit->getPlayerHolder());
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    const sead::LookAtCamera& camera = mLiveActorKit->getCameraDirector_RS()->getLookAtMain();
    initSceneAudio3D(rInfo, &camera.getPos(), &camera.getMatrix(),
                     static_cast<const sead::PerspectiveProjection*>(
                         &mLiveActorKit->getCameraDirector_RS()->getProjectionMain()),
                     &camera.getAt(), "ターゲット寄り中間", mLiveActorKit->getAreaObjDirector(),
                     true);
    initAudioKeeper(nullptr);
    mAudioDirector->setPlayerHolder(mLiveActorKit->getPlayerHolder());
    mGameDataHolder->setPlayerHolder(mLiveActorKit->getPlayerHolder());
    getDemoDirector()->setAudioDirector(mAudioDirector);
    mInvincibleBgmController = new PlayerInvincibleBgmController(mAudioDirector);
    mBigBgmController = new PlayerBigBgmController(mAudioDirector);
    al::initItemDirector(this, new ProjectItemDirector(layoutInfo, mGameDataHolder,
                                                       mLiveActorKit->getPlayerHolder(),
                                                       mLiveActorKit->getAreaObjDirector()));
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());

    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    mCameraPoserSceneInfo = new al::CameraPoserSceneInfo_RS();
    mCameraPoserSceneInfo->init(mLiveActorKit->getAreaObjDirector(),
                                mLiveActorKit->getCollisionDirector(), mAudioDirector);
    auto* poserFactory = new al::CameraPoserFactory("CameraPoserFactory");
    al::initCameraDirector_RS(this, mStageName.cstr(), poserFactory,
                              mLiveActorKit->getCameraDirector()->getSceneCameraInfo());
    mLiveActorKit->getCameraDirector()->reviseCameraInfo(
        mLiveActorKit->getCameraDirector_RS()->getSceneCameraInfo());
    mLiveActorKit->setupCameraAreaObjDirector();
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, true);
    al::setSceneObj(this, new CoinRotater(mLiveActorKit->getExecuteDirector()),
                    SceneObjID_CoinRotater);
    ScoreHolderUtil::initScoreHolder(this);
    auto* itemDirector = static_cast<ProjectItemDirector*>(mLiveActorKit->getItemDirector());
    itemDirector->createItemHolder(actorInfo, false);
    al::setSceneObj(this, new al::FootPrintServer(actorInfo, "FootPrint", 32),
                    SceneObjID_FootPrintServer);
    mStampDirector = new rc::StampDirector(
        actorInfo, "Stamp",
        mDrcAssistDirectorList->getDrcAssist(al::getMainControllerPort())->getTouchAssistInfo(),
        30, -1);
    mDrcAssistDirectorList->setStampDirector(mStampDirector);
    al::setSceneObj(this, mStampDirector, SceneObjID_StampDirector);
    mLiveActorKit->getGraphicsSystemInfo()->initStageResource(
        al::tryGetStageResourceDesign(this, 0), mStageName.cstr(), mLiveActorKit, false,
        mUnlockedPhase);
    mShadowMarioDirector =
        new ShadowMarioDirector(99, this, rInfo, mLiveActorKit->getExecuteDirector(),
                                mLiveActorKit->getPlayerHolder(), mGameDataHolder);
    initPlacement(actorInfo);
    initSceneAudioAfterInitPlacement(rInfo);
    mScreenCaptureExecutor = rInfo.mScreenCaptureExecutor;
    setCameraSensitivity(mLiveActorKit->getCameraDirector_RS(),
                         SingleModeDataFunction::getCameraSensitiviy(this));
    mLiveActorKit->getCameraDirector_RS()->setReverseRightAndLeftFlag(
        SingleModeDataFunction::getCameraReverseHorizontal(GameDataHolderAccessor(this)));
    mLiveActorKit->getCameraDirector_RS()->setReverseUpAndDownFlag(
        SingleModeDataFunction::getCameraReverseVertical(GameDataHolderAccessor(this)));
    initIslandKeeper();

    if (al::isExistSceneObj(this, SceneObjID_ChikaChikaBlockSynchronizer)) {
        al::getSceneObj<ChikaChikaBlockSynchronizer>(this, SceneObjID_ChikaChikaBlockSynchronizer)
            ->initAudioKeeper(actorInfo);
    }

    EchoEmitterHolder* emitterHolder = rc::tryGetEmitterHolder(this);

    if (emitterHolder != nullptr) {
        mLiveActorKit->getGraphicsSystemInfo()->setViewIndexedUboArray(
            "EchoBlockEmitterUbo", emitterHolder->getUboArray());
        al::registerExecutorUser(emitterHolder, mLiveActorKit->getExecuteDirector(),
                                 emitterHolder->getSceneObjName());
    }

    mPlayerAliveWatcher =
        new PlayerAliveWatcher(actorInfo, mLiveActorKit->getPlayerHolder(), true, false, false);
    al::setSceneObj(this, mPlayerAliveWatcher, SceneObjID_PlayerAliveWatcher);
    rc::setAliveWatcherToAudio(mPlayerAliveWatcher, mLiveActorKit->getPlayerHolder());
    s32 phase = mUnlockedPhase;
    mSceneLayout = new SingleModeSceneLayout(
        layoutInfo, mGameDataHolder, mStageName.cstr(),
        al::getSceneObj<GreenStarKeeper>(this, SceneObjID_GreenStarKeeper),
        mLiveActorKit->getPlayerHolder(), mPlayerAliveWatcher, itemDirector);
    al::setSceneObj(this, mSceneLayout, SceneObjID_SingleModeSceneLayout);

    if (phase == cPhasePlessieChase2 || phase == cPhasePlessieChase1) {
        mSceneLayout->setAreaNamePhaseStart(false);
    }

    mSnapshotLayout = new SnapshotLayout(layoutInfo, mStampDirector);
    mLiveActorKit->getCameraDirector_RS()->initSnapShotCameraAudioKeeper(mSnapshotLayout);
    al::setSceneObj(this, new GuideGameWindow(layoutInfo, true), SceneObjID_GuideGameWindow);
    mGuideGameWindow = al::getSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
    al::setSceneObj(this,
                    new CameraChangeLayout(layoutInfo, mLiveActorKit->getCameraDirector(),
                                           mGuideGameWindow, false),
                    SceneObjID_CameraChangeLayout);
    mWipeWorldJump = new al::WipeSimple("コースセレクトワールドジャンプワイプ", "WipeFadeWhite",
                                        layoutInfo, "IslandMap");
    mSceneLayout->setAreaNameWipeFadeWhite(mWipeWorldJump);
    initIslandMap(layoutInfo, actorInfo);
    mWipeMiss = new al::WipeSimple("ミスワイプ", "WipeMissSingleMode", layoutInfo, nullptr);
    mWipeFadeBlack = new al::WipeSimple("黒フェードワイプ", "WipeFadeBlack", layoutInfo, "2D");
    mPauseMenu = new PauseMenu(layoutInfo, mGameDataHolder, mPlayerAliveWatcher,
                               mLiveActorKit->getCameraDirector_RS(),
                               mLiveActorKit->getPlayerHolder(),
                               rInfo.mGameSystemInfo->getGamePadSystem(), mScreenCaptureExecutor,
                               mWipeFadeBlack);
    rInfo.mGameSystemInfo->getGamePadSystem()->setPadName(
        0, sead::WSafeString(
               al::getSystemMessageString(mSceneLayout, "SingleMode_PlayerName", 0)));
    rInfo.mGameSystemInfo->getGamePadSystem()->setPadName(
        1, sead::WSafeString(
               al::getSystemMessageString(mSceneLayout, "SingleMode_PlayerName", 1)));
    rInfo.mGameSystemInfo->getGamePadSystem()->set40(true);

    if (!mIsLastBowserBattle) {
        const char* demoName =
            phase == 5 && SingleModeDataFunction::isFirstPhase3BossDefeated(this) ?
                "DemoSingleModeGenericDialogueP3" :
                "DemoSingleModeGenericDialogue";
        mDemoCutscene = new DemoCutscene(demoName, static_cast<alSeFunction::DemoType>(2));
        mDemoCutscene->setDemoName(demoName);
        mDemoCutscene->init(actorInfo);
        mDemoCutscene->setDisableCapture(true);
        mDemoCutscene->setAllowSkip(false);
    }

    endInit(actorInfo, this);
    endInitIslandKeeper();
    setCameraZoneID();
    mLiveActorKit->getCameraDirector_RS()->endInit(mLiveActorKit->getPlayerHolder(),
                                                   mIslandKeeper->getActiveIslandIndex(), false);
    mPlayerCameraTarget = new PlayerCameraTarget(mLiveActorKit->getPlayerHolder()->getPlayer(0));
    al::LiveActorGroup* actorGroup = mLiveActorKit->getActorGroup();

    for (s32 i = 0; i < actorGroup->getActorCount(); i++) {
        al::LiveActor* actor = mLiveActorKit->getActorGroup()->getActor(i);

        if (al::isEqualString(actor->getName(), "SuperBowser")) {
            mPlayerCameraTarget->setBoss(actor);
            break;
        }
    }

    if (mIsPhase0Start) {
        mStartEvent = mPhase0StartDemo;
        initNerve(&NrvSingleModeSceneStartPhase0Demo, 2);
    } else {
        initNerve(&NrvSingleModeSceneStart, 2);

        if (phase == 0) {
            mSceneLayout->startDemo(false, true);
        }
    }

    mSnapshotState = new SnapshotState(mSnapshotLayout, this, mDrcAssistDirectorList,
                                       mStampDirector, mPlayerAliveWatcher);
    al::initNerveState(this, mSnapshotState, &NrvSingleModeSceneCaptureMode, "SnapshotState");
    DisasterModeController* disasterController = DisasterModeController::tryGetController(this);

    if (disasterController != nullptr) {
        disasterController->setState(
            SingleModeDataFunction::getDisasterModeFrames(GameDataHolderAccessor(mGameDataHolder)),
            SingleModeDataFunction::getIsDisasterMode(GameDataHolderAccessor(mGameDataHolder)), -1,
            SingleModeDataFunction::getIsDisasterMode(GameDataHolderAccessor(mGameDataHolder)));
    }

    SingleModeDataFunction::setPhaseEnd(GameDataHolderAccessor(this), false);
    mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer()->setSingleModeRendering();
    BoxPropeller::resetPropellerJumpGuideMessage(GameDataHolderAccessor(this));

    if (!mGameDataHolder->isCourseSelectVisited()) {
        mGameDataHolder->setCourseSelectVisited(true);
    }
}

/**
 * Registers the camera zone of every island to the camera resources.
 */
void SingleModeScene::setCameraZoneID() {
    al::CameraResourceHolder* resourceHolder =
        mLiveActorKit->getCameraDirector_RS()->mResourceHolder;

    for (s32 i = 0; i < mIslandKeeper->getHolderNum(); i++) {
        auto* island = static_cast<const IslandHolder*>(mIslandKeeper->findIsland(i));

        if (island != nullptr) {
            resourceHolder->tryInitZoneID(island->getIslandName(), island->getIslandId());
        }
    }
}

/**
 * Initializes the island keeper with the actor kit of the scene.
 */
void SingleModeScene::initIslandKeeper() {
    mIslandKeeper->init(mLiveActorKit);
}

/**
 * Ends the initialization of the island keeper once every island is placed.
 */
void SingleModeScene::endInitIslandKeeper() {
    mIslandKeeper->updateScenarios();
    mIslandKeeper->endInit(mGameDataHolder, mIntroFlyOverCamera);
}

/**
 * Initializes the island map; the base scene has none.
 * @param rLayoutInfo Unused.
 * @param rActorInfo Unused.
 */
void SingleModeScene::initIslandMap(const al::LayoutInitInfo& rLayoutInfo,
                                    const al::ActorInitInfo& rActorInfo) {
    mIslandMap = nullptr;
}

/**
 * Makes the scene appear: places the players and starts the stage.
 */
void SingleModeScene::appear() {
    SingleModeDataFunction::setDemoWasCancelled(GameDataHolderAccessor(mGameDataHolder), false);
    PlayerEntryFunction::entryPlayer(GameDataHolderAccessor(mGameDataHolder), 0, 0);
    decidePlayerPlacement();
    GameDataFunction::onStageStart(GameDataHolderAccessor(mGameDataHolder));
    al::Scene::appear();
    getFrameworkAlive()->mIsClearRenderBuffer = false;
    mScreenCoverCtrl->requestCaptureScreenCover(4);
}

/**
 * Places the players of the active control users and makes them appear.
 */
void SingleModeScene::decidePlayerPlacement() {
    for (s32 i = 0; i < rc::getPlayerCharacterNumMax(); i++) {
        s32 userId = rc::tryCalcControlUserIdByCharacterType(this, i, false);

        if (userId == -1) {
            continue;
        }

        PlayerActor* player = mPlayers[i];
        GameDataHolderAccessor accessor(mGameDataHolder);
        player->setViewMtx(&mLiveActorKit->getCameraDirector_RS()->mViewMtx);
        player->replaceInputPort(rc::getControlUserPortNumber(accessor, userId));
        s32 figureType = rc::getControlUserFigureType(accessor, userId);
        u32 modelFigureType;

        if (figureType == 1) {
            modelFigureType = 0;
        } else if (figureType == 8 || figureType == 9) {
            modelFigureType = 3;
        } else {
            modelFigureType = figureType;
        }

        rc::initPlayerFigureType(player, modelFigureType, false);

        // Leftover of the 3D World placement of the players side by side.
        const sead::Vector3f& front = player->getProperty()->getFront();
        sead::Vector3f side(front.z, 0.0f, -front.x);
        side.normalize();
        rc::getActiveControlUserNum(accessor);
        rc::calcControlUserDisplayOrder(accessor, userId);

        sead::Vector3f pos = player->getProperty()->getTrans();
        player->getProperty()->mTrans = pos;
        al::setTrans(player, pos);
        player->updatePosture();
        player->appearSingleMode(false);
        player->getModelHolder()->change(modelFigureType);
        player->getModelHolder()->appear();
        mPlayerCameraTarget->setPlayer(player);
        mLiveActorKit->getCameraDirector_RS()->AddCameraTarget(
            mPlayerCameraTarget->getCameraTarget());

        if (rc::calcControlUserDisplayOrder(GameDataHolderAccessor(mGameDataHolder), userId) == 0) {
            rc::setMainPlayerActor(player);
        }

        if (mStartingFigure < 0) {
            continue;
        }

        if (mStartingFigure == 8) {
            rc::tryChangeToGigaClimbMario(nullptr, player, true);
        } else if (mStartingFigure == 7) {
            rc::tryChangeToGigaMario(nullptr, player);
        }
    }

    mPlayerAliveWatcher->appear();
}

/**
 * Ends the scene.
 */
void SingleModeScene::kill() {
    al::Scene::kill();
}

/**
 * Updates the save, the frame skipping of the framework, the camera inputs and the graphics.
 */
void SingleModeScene::control() {
    InkPatchSpecial* inkPatchSpecial = InkPatchSpecial::tryGetInkPatchSpecial(this);

    if (inkPatchSpecial != nullptr) {
        inkPatchSpecial->incProgress();
    }

    if (mIsSaving) {
        updateSave();
    }

    bool isDocked = getFrameworkAlive()->mIsDocked;
    bool isSkipDraw = getFrameworkAlive()->_27b;

    if (isDocked) {
        if (isSkipDraw) {
            al::GameFrameworkNx* framework = getFramework();
            framework->_27b = false;
            framework->_27c = true;
        }
    } else if (!isSkipDraw) {
        al::GameFrameworkNx* framework = getFramework();
        framework->_27b = true;
        framework->_27c = true;
    }

    if (getFrameworkAlive()->_27b &&
        (mScreenCaptureExecutor->isAnyActiveRequest() || mScreenCoverCtrl->mIsRequestCapture ||
         mScreenCoverCtrl->mIsRequestCaptureScene)) {
        al::GameFrameworkNx* framework = getFrameworkAlive();
        framework->_27b = true;
        framework->_27c = true;
    }

    al::CameraDirector_RS* cameraDirector = mLiveActorKit->getCameraDirector_RS();

    if (!al::isNerve(this, &NrvSingleModeSceneRestart) && !cameraDirector->isCameraInputFrozen()) {
        bool is2PAssistMode = mGameDataHolder->is2PAssistMode();
        s32 inputNum = cameraDirector->getActiveInputNum();

        if (is2PAssistMode) {
            if (inputNum != 2) {
                cameraDirector->setActiveInputNum(2);
            }
        } else if (inputNum != 1) {
            cameraDirector->setActiveInputNum(1);
        }
    }

    bool isDrawGraphics = mLiveActorKit->preDrawGraphics();
    al::getSceneObj<PlayerCooperation>(this, SceneObjID_PlayerCooperation)->update();
    FurEnv* furEnv = al::getSceneObj<FurEnv>(this, SceneObjID_FurEnv);

    if (isDrawGraphics) {
        furEnv->update();
    }
}

/**
 * Updates the save started by the scene.
 */
void SingleModeScene::updateSave() {
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        mIsSaving = false;
    }
}

/**
 * Draws the main screen: the 3D view, the stamps, the 2D layouts and the screen captures.
 */
void SingleModeScene::drawMain_() const {
    agl::RenderBuffer* renderBuffer = getFramework()->getCurrentRenderBuffer();
    mViewport.setByFrameBuffer(*renderBuffer);
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFrameworkAlive()->mIsDocked);
    mLayoutKit->setFrameBuffer(renderBuffer, &mViewport);

    const sead::LookAtCamera& camera = mLiveActorKit->getCameraDirector_RS()->getLookAtMain();
    const sead::Matrix44f& projMtx =
        mLiveActorKit->getCameraDirector_RS()->getProjectionMain().getProjectionMatrix();
    alSystemKitFunction::applyViewportTop(mViewport);
    EchoEmitterHolder* emitterHolder = rc::tryGetEmitterHolder(this);

    if (emitterHolder != nullptr) {
        emitterHolder->setToUbo(0, camera.getMatrix(), projMtx);
    }

    if (isDraw3D()) {
        OceanWater* oceanWater = OceanWater::getOceanWater(this);

        if (oceanWater != nullptr) {
            static_cast<OceanWaterDeferred*>(oceanWater)->buildSampleHeightLookup();
        }

        al::LiveActorKit* kit = mLiveActorKit;
        al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
        const al::SceneCameraInfo* cameraInfo = getSceneCameraInfo();
        agl::RenderBuffer* viewRenderBuffer = getFrameworkAlive()->getCurrentRenderBuffer();
        bool isFadeScreen =
            al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder)->isCollectDemo() ||
            al::isNerve(this, &NrvSingleModeSceneRestart);
        viewRenderer->drawView(0, 0, kit, cameraInfo, viewRenderBuffer, mViewport, true,
                               isFadeScreen, static_cast<agl::ShaderMode>(4));
        mStampDirector->draw2D(renderBuffer);
    }

    al::drawKit(this, getDraw2DKitMainName());

    if (mScreenCoverCtrl->mIsRequestCaptureScene) {
        mScreenCoverCtrl->mIsRequestCaptureScene = false;
        mScreenCaptureExecutor->requestCapture(false, 1, false);
    }

    if (mScreenCaptureExecutor->isDraw(1)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     getFrameworkAlive()->getCurrentRenderBuffer(), 1);
        al::drawKit(this, "2DDrawAboveBlur1Pause");
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           getFrameworkAlive()->getCurrentRenderBuffer(), 1);

        if (!al::isNerve(this, &NrvSingleModeSceneCaptureMode)) {
            al::drawKit(this, "2DDrawAboveBlur1");
        }
    }

    if (mScreenCaptureExecutor->isDraw(2)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     getFrameworkAlive()->getCurrentRenderBuffer(), 2);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           getFrameworkAlive()->getCurrentRenderBuffer(), 2);
    }

    al::drawKit(this, "2DDrawAboveBlur2");
    const_cast<sead::PerspectiveProjection&>(
        static_cast<const sead::PerspectiveProjection&>(
            mLiveActorKit->getCameraDirector_RS()->getProjectionMain()))
        .setOffset(sead::Vector2f::zero);
    DisasterModeController* disasterController = DisasterModeController::tryGetController(this);

    if (disasterController != nullptr) {
        disasterController->tryDrawDisasterModeTimer();

        if (disasterController->getSuperBowser() != nullptr) {
            disasterController->getSuperBowser()->tryDrawDebugText();
        }

        if (disasterController->getSuperBowserShell() != nullptr) {
            disasterController->getSuperBowserShell()->tryDrawStateDebug();
        }
    }
}

/**
 * Checks whether the 3D view is drawn.
 * @return false while a menu or the island map covers the screen.
 */
bool SingleModeScene::isDraw3D() const {
    if (al::isNerve(this, &NrvSingleModeScenePause)) {
        return false;
    }

    if (al::isNerve(this, &NrvSingleModeScenePauseIslandMap) && al::isGreaterEqualStep(this, 2)) {
        return false;
    }

    if (al::isNerve(this, &NrvSingleModeSceneDemoIslandMap) && !mWipeFadeBlack->isAlive()) {
        return false;
    }

    if (al::isNerve(this, &NrvSingleModeScenePauseItemSelect) &&
        al::isGreaterEqualStep(this, 2)) {
        return false;
    }

    if (al::isNerve(this, &NrvSingleModeSceneDemoIslandMapUnlock)) {
        return false;
    }

    if (al::isNerve(this, &NrvSingleModeSceneDemoIslandMapUnlockPreGame)) {
        return false;
    }

    if (al::isNerve(this, &NrvSingleModeSceneKoopaJrOptionsIntro)) {
        return false;
    }

    return !al::isNerve(this, &NrvSingleModeSceneLoadGame);
}

/**
 * Gets the name of the 2D kit drawn on the main screen.
 * @return The kit name.
 */
const char* SingleModeScene::getDraw2DKitMainName() const {
    return al::isNerve(this, &NrvSingleModeSceneCaptureMode) ? "２Ｄミス（メイン画面）" :
                                                               "２Ｄベース（メイン画面）";
}

/**
 * Checks whether the scene ended with the goal.
 * @return Whether the scene is in the goal state.
 */
bool SingleModeScene::isGoal() const {
    return al::isNerve(this, &NrvSingleModeSceneGoal);
}

/**
 * Checks whether the scene restarts.
 * @return Whether the scene is in the restart state.
 */
bool SingleModeScene::isRestart() const {
    return al::isNerve(this, &NrvSingleModeSceneRestart);
}

/**
 * Checks whether the scene restarts after a miss.
 * @return Whether the scene is in the restart state.
 */
bool SingleModeScene::isRestartCheck() const {
    return al::isNerve(this, &NrvSingleModeSceneRestart);
}

/**
 * Checks whether the scene changes to another phase.
 * @return false.
 */
bool SingleModeScene::isChangePhase() const {
    return false;
}

/**
 * Checks whether the stage is reentered.
 * @return Whether the scene is in the reenter state.
 */
bool SingleModeScene::isReenterStage() const {
    return al::isNerve(this, &NrvSingleModeSceneReenterStage);
}

/**
 * Checks whether the player retired.
 * @return Whether the scene is in the retire state.
 */
bool SingleModeScene::isRetire() const {
    return al::isNerve(this, &NrvSingleModeSceneRetire);
}

/**
 * Checks whether the phase ended.
 * @return false.
 */
bool SingleModeScene::isPhaseEnd() const {
    return false;
}

/**
 * Checks whether the game ended.
 * @return false.
 */
bool SingleModeScene::isGameEnd() const {
    return false;
}

/**
 * Checks whether the game changes to Super Mario 3D World.
 * @return Whether the scene is in the game change state.
 */
bool SingleModeScene::isGameChange() const {
    return al::isNerve(this, &NrvSingleModeSceneGameChange);
}

/**
 * Checks whether another save file is loaded.
 * @return Whether the scene is in the load game state.
 */
bool SingleModeScene::isLoadGame() const {
    return al::isNerve(this, &NrvSingleModeSceneLoadGame);
}

/**
 * Checks whether the pause menu is opened.
 * @param pPort Set to the port of the controller that opened the menu.
 * @return Whether the pause menu is opened.
 */
bool SingleModeScene::isTriggerPause(s32* pPort) const {
    DisasterModeController* disasterController = DisasterModeController::tryGetController(this);

    if (disasterController != nullptr && disasterController->isWipeActive()) {
        return false;
    }

    u16 portList = rc::getActiveInputPortList(GameDataHolderAccessor(mGameDataHolder));
    s32 mainPort = al::getMainControllerPort();
    s32 subPort = rc::getPadPortByUserId(1);

    if ((portList & (1 << mainPort)) == 0 || !al::isPadConnected(mainPort) ||
        rc::isDeadControlUserInStage(this, mainPort)) {
        return false;
    }

    s32 port;

    if (SingleModeDataFunction::getIs2PAssistMode(this)) {
        if (al::isPadTypeJoyLeft(mainPort)) {
            if (al::isPadTypeJoyLeft(subPort)) {
                if (!al::isPadTriggerMinus(mainPort)) {
                    return false;
                }

                port = mainPort;
            } else {
                if (!al::isPadTriggerPlus(subPort)) {
                    return false;
                }

                port = subPort;
            }
        } else if (al::isPadTriggerPlus(mainPort)) {
            port = mainPort;
        } else {
            if (!al::isPadTriggerPlus(subPort) ||
                (al::isPadTypeJoyRight(mainPort) && al::isPadTypeJoyRight(subPort))) {
                return false;
            }

            port = subPort;
        }
    } else {
        if (!rc::isPadTriggerStart(mainPort)) {
            return false;
        }

        port = mainPort;
    }

    if (pPort != nullptr) {
        *pPort = port;
    }

    return true;
}

/**
 * Checks whether the controller that opened the pause menu closes it.
 * @return Whether the pause menu is closed.
 */
bool SingleModeScene::isTriggerBack() const {
    s32 port = -1;

    if (!isTriggerPause(&port)) {
        return false;
    }

    if (!al::isPadConnected(mPausePort)) {
        return true;
    }

    return port == mPausePort;
}

/**
 * Checks whether the island map is opened.
 * @param pPort Set to the port of the controller that opened the map.
 * @return Whether the island map is opened.
 */
bool SingleModeScene::isTriggerMapMenu(s32* pPort) const {
    if (!SingleModeDataFunction::isMapEnabled(this) || !mSceneLayout->isAlive() ||
        mSceneLayout->getWipe()->isAlive()) {
        return false;
    }

    s32 mainPort = al::getMainControllerPort();
    s32 subPort = rc::getPadPortByUserId(1);
    s32 port;

    if (SingleModeDataFunction::getIs2PAssistMode(this)) {
        if (al::isPadTypeJoyRight(mainPort)) {
            if (al::isPadTypeJoyRight(subPort)) {
                if (!al::isPadTriggerPlus(subPort)) {
                    return false;
                }

                port = subPort;
                goto trigger;
            }
        } else if (al::isPadTypeJoyLeft(mainPort)) {
            if (al::isPadTriggerMinus(mainPort)) {
                if (al::isPadTypeJoyLeft(subPort)) {
                    return false;
                }

                port = mainPort;
                goto trigger;
            }
        } else if (al::isPadTriggerRight(al::getMainControllerPort()) ||
                   al::isPadTriggerMinus(al::getMainControllerPort())) {
            port = mainPort;
            goto trigger;
        }

        if (!al::isPadTriggerMinus(subPort)) {
            return false;
        }

        port = subPort;
    } else {
        if (!al::isPadTriggerRight(al::getMainControllerPort()) &&
            !al::isPadTriggerMinus(al::getMainControllerPort())) {
            return false;
        }

        port = al::getMainControllerPort();
    }

trigger:
    if (pPort != nullptr) {
        *pPort = port;
    }

    return true;
}

/**
 * Checks whether the item stock is opened.
 * @param pPort Set to the port of the controller that opened the item stock.
 * @return Whether the item stock is opened.
 */
bool SingleModeScene::isTriggerItemSelect(s32* pPort) const {
    if (!mSceneLayout->isAlive() || !mSceneLayout->canSpawnItem()) {
        return false;
    }

    al::LiveActor* player = rc::findPlayerFromInputPort(mLiveActorKit->getPlayerHolder(),
                                                         al::getPlayerControllerPort(0));

    if (player == nullptr || rc::isPlayerDamageTrigOn(player)) {
        return false;
    }

    if (rc::isPlayerBinded(player) && !rc::isPlayerOnRaidon(player)) {
        return false;
    }

    u32 itemCount = 0;

    for (s32 i = 0; i < 6; i++) {
        itemCount |= SingleModeDataFunction::getStockItemCountByIndex(GameDataHolderAccessor(mGameDataHolder), i);
    }

    if (itemCount == 0) {
        return false;
    }

    if (pPort != nullptr) {
        *pPort = mSceneLayout->getItemStockControllerID();
    }

    return true;
}

/**
 * Checks whether an item of the item stock is spawned.
 * @param port The port of the controller using the item stock.
 * @return Whether the item is spawned.
 */
bool SingleModeScene::isTriggerItemSpawn(s32 port) const {
    if (!mSceneLayout->isAlive()) {
        return false;
    }

    if (!rc::isPlayerChangeAllowed(mLiveActorKit->getPlayerHolder(),
                                   al::getPlayerControllerPort(0))) {
        return false;
    }

    if (al::isPadTypeJoySingle(port)) {
        return al::isPadHoldX(port);
    }

    return al::isPadHoldUp(port);
}

/**
 * Records the end of the stage before the scene is destroyed.
 */
void SingleModeScene::prepareDestroy() {
    if (mIsPrepareDestroyed) {
        return;
    }

    mIsPrepareDestroyed = true;
    GameDataFunction::onStageEnd(GameDataHolderAccessor(mGameDataHolder));
    al::changeBgmSituation(this, "ChangeHurryToNormal");
    al::changeBgmSituation(this, "OutWater");

    if (al::isNerve(this, &NrvSingleModeSceneRetire)) {
        return;
    }

    isGameEnd();
}

/**
 * Invalidates the inputs of every player for a few frames.
 */
void SingleModeScene::invalidatePlayerInput() {
    s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());

    for (s32 i = 0; i < playerNum; i++) {
        static_cast<PlayerActor*>(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i))
            ->getInput()
            ->invalidateFrame(2);
    }
}

/**
 * Checks whether the start wipe can open.
 * @return Whether the start wipe can open.
 */
bool SingleModeScene::isEnableOpenStartWipe() const {
    if (mStartEvent == nullptr) {
        return true;
    }

    mStartEvent->getEventType();
    return mStartEvent->isEnableOpenStartWipe();
}

/**
 * Plays the start demo of the first phase.
 */
void SingleModeScene::exeStartPhase0Demo() {
    if (al::isFirstStep(this)) {
        mPhase0StartDemo->startDemo();
        rc::requestStartDemoPlayerCutscene(mPhase0StartDemo);
    }

    if (al::isStep(this, 2)) {
        mSceneLayout->startDemo(true, true);
    }

    updatePlay();

    if (mPhase0StartDemo->isEndDemo()) {
        rc::requestEndDemoPlayerCutscene(mPhase0StartDemo);
        mSceneLayout->endDemo(true, false);
        al::setNerve(this, &NrvSingleModeSceneStart);
    }
}

/**
 * Updates the scene while playing: play report, kits, island tracking and checkpoints.
 */
void SingleModeScene::updatePlay() {
    if (mGameDataHolder->isNeedPlayStartReport()) {
        mGameDataHolder->setNeedPlayStartReport(false);

        if (!SingleModeDataFunction::beginPlayReport(GameDataHolderAccessor(mGameDataHolder),
                                                     nullptr, static_cast<preport::KeyEventType>(2),
                                                     1, 0)) {
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(45), 1);
            SingleModeDataFunction::endPlayReport(GameDataHolderAccessor(mGameDataHolder));
        }

        if (!mGameDataHolder->beginPlayReport(static_cast<preport::KeyEventType>(0), 10, 0)) {
            GameDataHolder* holder = mGameDataHolder;
            SingleModeData* singleModeData = holder->getSingleFile();
            GameDataFile* gameDataFile = holder->getGameDataFile(holder->getLastPlayingFileId());
            mGameDataHolder->addSessionId();
            SingleModeDataFunction::setPlayReportData(
                GameDataHolderAccessor(mGameDataHolder), static_cast<preport::Key>(9),
                mGameDataHolder->getLastSingleModePlayingFileID());
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(10),
                                                      singleModeData->getName());
            SingleModeDataFunction::setPlayReportData(
                GameDataHolderAccessor(mGameDataHolder), static_cast<preport::Key>(11),
                gameDataFile->getTotalPlayTimePR(false));
            SingleModeDataFunction::setPlayReportData(
                GameDataHolderAccessor(mGameDataHolder), static_cast<preport::Key>(12),
                singleModeData->getTotalPlayTimePR(true));
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(3),
                                                      SingleModeData::sOptions.mIsCameraReverseHorizontal);
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(4),
                                                      SingleModeData::sOptions.mIsCameraReverseVertical);
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(7),
                                                      SingleModeData::sOptions.mCameraSensitivity);
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(8),
                                                      SingleModeData::sOptions.mAssistModeType);
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      static_cast<preport::Key>(34),
                                                      singleModeData->getGoalItemNum());
            SingleModeDataFunction::setPlayReportData(
                GameDataHolderAccessor(mGameDataHolder), static_cast<preport::Key>(35),
                singleModeData->getCurValidIslandVisited());
            mGameDataHolder->endPlayReport();
        }
    }

    if (al::isStopScene(this)) {
        if (!al::isStopScenePlayers(this)) {
            al::updateKitList(this, "プレイヤー[Movement]");
        }

        al::updateKitList(this, "2D StopScene");
        al::updateEffectHitStop(this);

        if (al::isStopAndUpdateCamera(this)) {
            mLiveActorKit->getCameraDirector_RS()->execute();
        }

        return;
    }

    mLiveActorKit->getCameraDirector_RS()->execute();

    if (al::isNerve(this, &NrvSingleModeScenePlay)) {
        rc::updateDrcAssistDirector(this);
    }

    if (al::isNerve(this, &NrvSingleModeScenePlay) &&
        al::isExistSceneObj(this, SceneObjID_ChikaChikaBlockSynchronizer)) {
        rc::updateChikaChikaSynchronizer(this);
    }

    if (mGameDataHolder->isFreezeMode()) {
        updateFreezeMode(false);
    } else {
        al::updateKit(this);
    }

    mLiveActorKit->getExecuteDirector()->finishExecute();
    mPlayerAliveWatcher->update();

    if (al::isNerve(this, &NrvSingleModeScenePlay)) {
        s32 islandIndex = mIslandAreaWatcher->getActiveIslandIndex();
        bool isLeaveIsland = islandIndex == -1;

        if (islandIndex >= 0) {
            PlayerActor* player = mPlayers[0];

            if (player != nullptr &&
                ((rc::isPlayerOnGround(player) && !rc::isPlayerInWater(player)) ||
                 (rc::isPlayerOnRaidon(player) && rc::isPlayerOnRaidonGround(player)))) {
                mIslandKeeper->setActiveIslandIndex(islandIndex);
            }

            if (islandIndex == 16) {
                isLeaveIsland = true;
            } else if (player != nullptr && rc::isPlayerOnGround(player) &&
                       !rc::isPlayerOnRaidon(player) && !rc::isPlayerInWater(player)) {
                if (SingleModeDataFunction::getScenarioNum(this, islandIndex) != 0) {
                    mSceneLayout->updateCounters(islandIndex);
                    u8 scenarioFlag = SingleModeDataFunction::getScenarioFlag(this, islandIndex);
                    mSceneLayout->updateAreaName(islandIndex);

                    if ((scenarioFlag & 8) == 0 &&
                        SingleModeDataFunction::getShardFlag(this, islandIndex) != 0) {
                        mSceneLayout->showShardCounter();
                    }
                } else {
                    mSceneLayout->hideShardCounter();
                    mSceneLayout->hideShineCounter();
                    mSceneLayout->updateAreaName(islandIndex);
                }

                mAudioDirector->setBgmChangeWatcher(mIslandKeeper->getCurrentIslandId());
            } else {
                mAudioDirector->setBgmChangeWatcher(-1);
            }
        }

        if (isLeaveIsland) {
            mIslandKeeper->setActiveIslandIndex(islandIndex);
            mSceneLayout->fadeOutAreaName(true);
            mSceneLayout->updateCounters(-1);
            mAudioDirector->setBgmChangeWatcher(-1);
        }

        mIslandKeeper->update(islandIndex);
    }

    mLiveActorKit->getClippingDirector()->executeRequestAsyncUpdate();

    s32 zoneIndex;
    s32 checkpointIndex;
    SingleModeDataFunction::getLastCheckpointPass(this, &zoneIndex, &checkpointIndex);

    if (checkpointIndex != mLastCheckpointIndex || zoneIndex != mLastCheckpointZoneIndex) {
        if (mLastCheckpointIndex >= 0 && mLastCheckpointZoneIndex >= 0) {
            CheckpointArray* checkpoints = mCheckpointLists.unsafeAt(mLastCheckpointZoneIndex);

            if (mLastCheckpointIndex < checkpoints->size()) {
                checkpoints->at(mLastCheckpointIndex)->setStateBefore();
            }
        }

        mLastCheckpointIndex = checkpointIndex;
        mLastCheckpointZoneIndex = zoneIndex;
    }

    mLiveActorKit->updateReducedBufferEffect();

    if (mGameDataHolder->isSaveRequested()) {
        mIsRequestSave = true;
        mGameDataHolder->setSaveRequested(false);
    }
}

/**
 * Plays the start event of the stage.
 */
void SingleModeScene::exeStartEvent() {
    if (al::isFirstStep(this)) {
        mStartEvent->startDemo();
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        rc::hideGuideGameWindow(this);

        if (mStartEvent->getEventType() == cStartEventTypeTimer) {
            mSceneLayout->startDemo(false, false);
        } else {
            mSceneLayout->startDemo(true, true);
        }

        al::deactivateAudioEventController(this);
    }

    updateStartEvent();

    if (mStartEvent->isEndDemo()) {
        rc::unHideGuideGameWindow(this);

        if (mStartEvent->getEventType() == cStartEventTypeTimer) {
            mSceneLayout->endDemo(false, false);
        } else {
            mSceneLayout->endDemo(true, false);
        }

        alSeFunction::changeListenerPoserLast(mAudioDirector);
        al::setNerve(this, &NrvSingleModeSceneStart);
    }
}

/**
 * Updates the scene during the start event.
 */
void SingleModeScene::updateStartEvent() {
    mLiveActorKit->getCameraDirector_RS()->execute();

    if (mStartEvent->isEnableMovement()) {
        al::updateKit(this);
    } else {
        mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
        al::updateKitList(this, "Clipping");
        al::updateKitListPaused(this, "ModelUpdate");
        al::updateKitList(this, "デモプレイヤーロケーター");
        al::updateKitList(this, "デモプレイヤー前処理");

        if (mStartEvent->getEventType() == cStartEventTypeTimer) {
            al::updateKitList(this, "プレイヤー[Movement]");
        }

        al::updateKitList(this, "プレイヤー");
        al::updateKitList(this, "プレイヤー装飾");
        al::updateKitList(this, "プレイヤー装飾２");
        al::updateKitList(this, "デモ");
        al::updateKitList(this, "エフェクトオブジェ");
        al::updateKitList(this, "空");
        al::updateKitList(this, "デモオブジェクト");
        al::updateKitList(this, "シャドウマスク");
        al::updateKitList(this, "グラフィックス要求者");
        al::updateKitList(this, "ステージスイッチディレクター");
        al::updateKitList(this, "２Ｄ（ポーズ無視）");
        al::updateEffect(this);
        mLiveActorKit->updateGraphics(false);
    }

    mLiveActorKit->getExecuteDirector()->finishExecute();
    mLiveActorKit->getClippingDirector()->executeRequestAsyncUpdate();
    mLiveActorKit->updateReducedBufferEffect();
}

/**
 * Plays the start event binding the players.
 */
void SingleModeScene::exeStartBindDemo() {
    if (al::isFirstStep(this)) {
        mStartEvent->startDemo();
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ開始デモ", 0,
                                                    false);
        updatePlay();
        mDrcAssistDirectorList->disappearTouchPointer();
        mSceneLayout->startDemo(false, true);
        mPlayerAliveWatcher->startDemo();
        rc::hideGuideGameWindow(this);
        al::deactivateAudioEventController(this);
        return;
    }

    updatePlay();

    if (mStartEvent->isEndDemo()) {
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 60, false);
        mSceneLayout->endDemo(true, false);
        rc::unHideGuideGameWindow(this);
        mPlayerAliveWatcher->endDemo();
        al::offClippingPosAsPlayerPos(this);
        alSeFunction::changeListenerPoserLast(mAudioDirector);
        al::setNerve(this, &NrvSingleModeSceneStart);
    }
}

/**
 * Handles a game over: reports it and restarts the scene.
 */
void SingleModeScene::handleGameOver() {
    mDrcAssistDirectorList->disappearTouchPointerImmediately();
    s32 phase = mUnlockedPhase;

    if (phase == 2 || phase == 4 || phase == 6 || phase == 9) {
        if (!SingleModeDataFunction::beginPlayReport(this, this,
                                                     static_cast<preport::KeyEventType>(21), 3,
                                                     0)) {
            SingleModeDataFunction::setPlayReportData(
                this, static_cast<preport::Key>(19),
                SingleModeDataFunction::getIs2PAssistMode(this));
            SingleModeDataFunction::setPlayReportData(
                this, static_cast<preport::Key>(46),
                SingleModeDataFunction::getBossPlayTime(GameDataHolderAccessor(mGameDataHolder)));
            SingleModeDataFunction::setPlayReportData(this, static_cast<preport::Key>(47), 1);
            SingleModeDataFunction::endPlayReport(this);
        }
    } else {
        SingleModeDataFunction::reportIslandEvent(this, this, 1, -1);
    }

    al::setForceSceneHeapResourceDestroy();
    mSceneLayout->fadeOutAreaName(true);
    al::setNerve(this, &NrvSingleModeSceneRestart);
}

/**
 * Starts the stage BGM.
 */
void SingleModeScene::requestStageBgmStart() {
    al::startBgmWithAreaCheck(this, SingleModeDataFunction::isGigaBellPlayerRespawnPointValid(this),
                              -1, 0, -1);
}

/**
 * Starts the stage: audio, island intro and the play state.
 */
void SingleModeScene::exeStart() {
    if (al::isFirstStep(this)) {
        if (SingleModeDataFunction::isSceneRestart(this)) {
            mAudioDirector->setOverrideFadeInFrames();
        }

        al::activateAudioEventController(this);
        al::changeAudioEffectWithAreaCheck(this);
        mUnlockedIslandNum =
            SingleModeDataFunction::getUnlockedIslandNum(GameDataHolderAccessor(mGameDataHolder));
        invalidatePlayerInput();

        if (mIslandKeeper->tryTriggerIntro()) {
            mSceneLayout->setInDemo(false);
            al::setNerve(this, &NrvSingleModeSceneDemoIntro);
            return;
        }

        bool isDemoIntroChange = checkDemoIntroChange(false);
        requestStageBgmStart();
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);

        if (isDemoIntroChange) {
            updateNerve();
            return;
        }
    }

    invalidatePlayerInput();
    updatePlay();
    mSceneLayout->endDemo(true, false);
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Starts the Bowser Jr. cutscene introducing the current phase, if not seen yet.
 * @param isFromPlay Whether the scene is playing (the cutscene starts after a fade).
 * @return Whether a cutscene starts.
 */
bool SingleModeScene::checkDemoIntroChange(bool isFromPlay) {
    if (mUnlockedPhase == 3 && SingleModeDataFunction::isFirstPhase2BossDefeated(this) &&
        !SingleModeDataFunction::hasSeenCutscene(this, 15)) {
        mDemoCutscene->setCutsceneId(15);
        mSceneLayout->setInDemo(false);
        mSceneLayout->startDemo(true, true);
        rc::hideGuideGameWindow(this);
        al::pausePadRumble(this);
    } else if (mUnlockedPhase == 5 && SingleModeDataFunction::isFirstPhase3BossDefeated(this) &&
               !SingleModeDataFunction::hasSeenCutscene(this, 17)) {
        al::pausePadRumble(this);
        mDemoCutscene->setCutsceneId(17);
        mSceneLayout->setInDemo(false);
        mSceneLayout->startDemo(true, true);
        rc::hideGuideGameWindow(this);
    } else if (mUnlockedPhase == 8 &&
               !SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                        27)) {
        al::pausePadRumble(this);
        mDemoCutscene->setCutsceneId(27);
        mSceneLayout->startDemo(true, true);
        rc::hideGuideGameWindow(this);
    } else {
        return false;
    }

    if (isFromPlay) {
        al::setNerve(this, &NrvSingleModeSceneDemoFadeTransition);

        if (!mScreenCaptureExecutor->isDraw(1)) {
            mScreenCaptureExecutor->requestCapture(false, 1, false);
        }

        mWipeFadeBlack->startClose(-1);
    } else {
        al::setNerve(this, &NrvSingleModeSceneDemoKoopaJrPhaseIntro);
    }

    return true;
}

/**
 * Saves and ends the phase.
 */
void SingleModeScene::handlePhaseEnd() {
    SaveDataAccessFunction::startSaveDataWriteSync(GameDataHolderAccessor(this).getHolder(),
                                                   true);
    al::setNerve(this, &NrvSingleModeScenePhaseEnd);
}

/**
 * Switches to the demo state of the active demo, if any.
 * @return Whether a demo state starts.
 */
bool SingleModeScene::checkDemoChange() {
    const al::Nerve* nerve;

    if (rc::isPlayerChangeDemoAny(mLiveActorKit->getPlayerHolder())) {
        nerve = &NrvSingleModeSceneDemoChangePlayer;
    } else if (rc::isActiveDemoCamera(this)) {
        cancelRaidonSurfBind(this);
        nerve = &NrvSingleModeSceneDemoCamera;
    } else if (rc::isActiveDemoMovingCamera(this)) {
        cancelRaidonSurfBind(this);
        nerve = &NrvSingleModeSceneDemoMovingCamera;
    } else if (rc::isActiveDemoPlayer(this) || rc::isActiveDemoBinding(this)) {
        cancelRaidonSurfBind(this);
        nerve = &NrvSingleModeSceneDemoScene;
    } else if (rc::isActiveDemoCutscene(this)) {
        cancelRaidonSurfBind(this);
        nerve = &NrvSingleModeSceneDemoCutscene;
    } else if (rc::isActiveDemoInGameCutscene(this)) {
        cancelRaidonSurfBind(this);
        nerve = &NrvSingleModeSceneDemoInGameCutscene;
    } else if (rc::isActiveDemoPlayerCutscene(this)) {
        cancelRaidonSurfBind(this);
        nerve = &NrvSingleModeSceneDemoPlayerScene;
    } else {
        return false;
    }

    al::setNerve(this, nerve);
    return true;
}

/**
 * Starts the game over if every player is dead.
 * @return Whether the game is over.
 */
bool SingleModeScene::checkGameOver() {
    if (!mPlayerAliveWatcher->isGameOver()) {
        return false;
    }

    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsAreaDirector()->lockArea();
    handleGameOver();
    return true;
}

/**
 * Updates the play state: saves, pauses, menus, demos and the game over.
 */
void SingleModeScene::exePlay() {
    if (mInvalidateInputFrames >= 1) {
        invalidatePlayerInput();
        mInvalidateInputFrames--;
    }

    updatePlay();

    if (al::isFirstStep(this)) {
        RaidonSurf* raidonSurf = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);

        if (raidonSurf != nullptr) {
            raidonSurf->setBindCanceled(false);
        }

        SingleModeDataFunction::resetSceneRestart(this);
        rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
        OceanWater* oceanWater = OceanWater::getOceanWater(this);

        if (oceanWater != nullptr) {
            oceanWater->setHeightMapScale(1.0f, true);
        }
    }

    if (mIsRequestSave && !mIsSaving) {
        startSave();
    }

    mInvincibleBgmController->update();
    mBigBgmController->update();

    if (SingleModeDataFunction::isPhaseEnd(this)) {
        handlePhaseEnd();
        return;
    }

    if (checkGameOver() || checkDemoChange()) {
        return;
    }

    if (isTriggerPause(&mPausePort) && !mSceneLayout->isDisablePause() &&
        !mSceneLayout->getWipe()->isAlive()) {
        mScreenCaptureExecutor->requestCapture(false, 1, true);
        al::setNerve(this, &NrvSingleModeScenePause);
        return;
    }

    if (isTriggerMapMenu(&mPausePort) && mIslandMap != nullptr &&
        !mSceneLayout->isDisablePause() && mIslandMap->isMapEnable()) {
        mSceneLayout->startPause(true);

        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->startHide(this);
        }

        mScreenCaptureExecutor->requestCapture(true, 1, true);
        al::setNerve(this, &NrvSingleModeScenePauseIslandMap);
        return;
    }

    if (isTriggerItemSelect(&mPausePort)) {
        mSceneLayout->startPause(false);

        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->startHide(this);
        }

        mScreenCaptureExecutor->requestCapture(true, 1, true);
        al::setNerve(this, &NrvSingleModeScenePauseItemSelect);
        return;
    }

    if (al::isPadTriggerDown(al::getMainControllerPort()) && !mSceneLayout->isDisablePause()) {
        mSceneLayout->startPause(true);
        mScreenCoverCtrl->requestCaptureScreenCover(5);
        al::setNerve(this, &NrvSingleModeSceneCaptureMode);
        return;
    }

    if (mGuideGameWindow != nullptr && mGuideGameWindow->isWaitConfirm()) {
        al::setNerve(this, &NrvSingleModeScenePauseWindowMessage);
        return;
    }

    if (PlayLogFunction::isUseDrcOneUser(GameDataHolderAccessor(mGameDataHolder))) {
        s32 port = al::getMainControllerPort();

        if (al::isPadHoldUp(port) || al::isPadHoldDown(port) || al::isPadHoldLeft(port) ||
            al::isPadHoldRight(port)) {
            PlayLogFunction::useCrossKey(GameDataHolderAccessor(mGameDataHolder));
        }
    }

    if (mIslandMap != nullptr) {
        mIslandMap->updatePlayerTracker(false);
    }
}

/**
 * Records the figures of the players and starts the save.
 */
void SingleModeScene::startSave() {
    mIsRequestSave = false;
    mIsSaving = true;

    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        al::LiveActor* player = rc::tryFindAlivePlayerActorFirstByUserId(
            al::getPlayerActor(mLiveActorKit->getPlayerHolder(), 0), i);

        if (player != nullptr) {
            rc::setControlUserFigureType(this, i, rc::getPlayerFigureType(player));
        } else {
            rc::setControlUserFigureType(this, i, rc::getPlayerFigureTypeDefault());
        }
    }

    SaveDataAccessFunction::startSaveDataWriteNoWindow(mGameDataHolder, false, false);
}

/**
 * Plays the player change demo.
 */
void SingleModeScene::exeDemoChangePlayer() {
    if (al::isFirstStep(this)) {
        setPlayerChangeDemoAll(mLiveActorKit->getPlayerHolder(), true);
    }

    if (al::isExistSceneObj(this, SceneObjID_ChikaChikaBlockSynchronizer)) {
        rc::updateSceneStopChikaChikaSynchronizer(this);
    }

    updateDemoChangePlayer();

    if (mPlayerCrown != nullptr && mPlayerCrown->isAttach()) {
        mPlayerCrown->movement();
        mPlayerCrown->calcAnim();
    }

    if (rc::isPlayerChangeDemoAny(mLiveActorKit->getPlayerHolder())) {
        return;
    }

    setPlayerChangeDemoAll(mLiveActorKit->getPlayerHolder(), false);
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Updates the scene during the player change demo.
 */
void SingleModeScene::updateDemoChangePlayer() {
    mLiveActorKit->getClippingDirector()->waitPendingClippingRequest();
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "プレイヤー前処理");
    al::updateKitList(this, "プレイヤー[Movement]");
    al::updateKitList(this, "プレイヤー後処理");
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateEffectPlayer(this);
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    mLiveActorKit->getExecuteDirector()->finishExecute();
    mLiveActorKit->updateGraphics(false);
    mLiveActorKit->updateReducedBufferEffect();
}

/**
 * Waits after the player change demo before going back to the play state.
 */
void SingleModeScene::exeDemoChangePlayerAfter() {
    updateDemoChangePlayerAfter();

    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvSingleModeScenePlay);
    }
}

/**
 * Updates the scene after the player change demo.
 */
void SingleModeScene::updateDemoChangePlayerAfter() {
    al::updateEffectPlayer(this);
    mLiveActorKit->updateReducedBufferEffect();
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
}

/**
 * Plays a camera demo.
 */
void SingleModeScene::exeDemoCamera() {
    if (al::isFirstStep(this)) {
        mAudioDemoType = getDemoDirector()->getAudioDemoType();
        alAudioSystemFunction::startDemo(mAudioDirector,
                                         static_cast<alSeFunction::DemoType>(mAudioDemoType));
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointerEffect();
        mSceneLayout->startDemo(false, true);
        rc::hideGuideGameWindow(this);
        rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);
    }

    updatePlay();
    updateAudioDemoType(&mAudioDemoType, this);

    if (al::isAnyActiveDemo(this)) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    al::setNerve(this, &NrvSingleModeScenePlay);

    if (checkDemoChange()) {
        return;
    }

    mSceneLayout->endDemo(true, false);
    rc::unHideGuideGameWindow(this);
}

/**
 * Plays a moving camera demo.
 */
void SingleModeScene::exeDemoMovingCamera() {
    if (al::isFirstStep(this)) {
        mAudioDemoType = getDemoDirector()->getAudioDemoType();
        alAudioSystemFunction::startDemo(mAudioDirector,
                                         static_cast<alSeFunction::DemoType>(mAudioDemoType));
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointerEffect();
        mSceneLayout->startDemo(false, true);
        rc::hideGuideGameWindow(this);
        rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);
    }

    updateDemoMovingCamera(false);
    updateAudioDemoType(&mAudioDemoType, this);

    if (al::isAnyActiveDemo(this)) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    al::setNerve(this, &NrvSingleModeScenePlay);

    if (checkDemoChange()) {
        return;
    }

    mSceneLayout->endDemo(true, false);
    rc::unHideGuideGameWindow(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
}

/**
 * Updates the scene during a moving camera demo.
 * @param isUpdateAll Whether the enemies, items and players are updated too.
 */
void SingleModeScene::updateDemoMovingCamera(bool isUpdateAll) {
    mLiveActorKit->getCameraDirector_RS()->execute();
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    mLiveActorKit->updatePadRumble();
    al::updateKitList(this, "OceanWaterRequest");
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "ライト管理");
    al::updateKitList(this, "Clipping");
    al::updateKitListPaused(this, "ModelUpdate");
    al::updateKitList(this, "デモ");
    al::updateKitList(this, "空");

    if (isUpdateAll) {
        al::updateDemoActor(this);
    } else {
        al::updateDemoActorWithEffects(this);
    }

    al::updateKitList(this, "デモオブジェクト");
    al::updateKitList(this, "地形オブジェ");
    al::updateKitList(this, "地形オブジェ[Movement]");
    al::updateKitList(this, "コリジョン地形");

    if (isUpdateAll) {
        al::updateKitList(this, "乗り物");
        al::updateKitList(this, "ＮＰＣ");
        al::updateKitList(this, "敵");
        al::updateKitList(this, "敵[Movement]");
        al::updateKitList(this, "敵装飾");
        al::updateKitList(this, "敵装飾[Movement]");
        al::updateKitList(this, "アイテム");
        al::updateKitList(this, "プレイヤー[Movement]");
        al::updateKitList(this, "プレイヤー");
        al::updateKitList(this, "プレイヤー装飾");
        al::updateKitList(this, "プレイヤー装飾２");
    }

    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateKitList(this, "コインローテータ");
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "2D StopScene");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateKitList(this, "Water");
    al::updateEffect(this);
    mLiveActorKit->updateReducedBufferEffect();
    mLiveActorKit->getExecuteDirector()->finishExecute();
    mLiveActorKit->getClippingDirector()->executeRequestAsyncUpdate();
    mLiveActorKit->updateGraphics(false);
}

/**
 * Restores the ocean and the clipping after the intro demo.
 */
void SingleModeScene::finishDemoIntro() {
    OceanWater* oceanWater = OceanWater::getOceanWater(this);

    if (oceanWater != nullptr) {
        oceanWater->setHeightMapScale(1.0f, true);
    }

    mLiveActorKit->getClippingDirector()->setExpandedClippingMode(false);
}

/**
 * Plays the intro fly-over of the current phase.
 */
void SingleModeScene::exeDemoIntro() {
    OceanWater* oceanWater = OceanWater::getOceanWater(this);

    if (al::isFirstStep(this)) {
        getDemoDirector()->setUnknownD4(true);
        mLiveActorKit->getClippingDirector()->setExpandedClippingMode(true);
        mAudioDemoType = getDemoDirector()->getAudioDemoType();
        alAudioSystemFunction::startDemo(mAudioDirector,
                                         static_cast<alSeFunction::DemoType>(mAudioDemoType));
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointerEffect();
        mSceneLayout->startDemo(true, true);
        rc::hideGuideGameWindow(this);
        rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);

        if ((mUnlockedPhase == 1 || mUnlockedPhase == 3 || mUnlockedPhase == 5) &&
            oceanWater != nullptr) {
            oceanWater->setHeightMapScale(0.0f, true);
        }
    }

    if (al::isStep(this, 4)) {
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);
    }

    invalidatePlayerInput();
    updateDemoMovingCamera(true);
    updateAudioDemoType(&mAudioDemoType, this);

    if (mUnlockedPhase == 5) {
        s32 step = al::getNerveStep(this);

        if (oceanWater != nullptr && step == 450) {
            oceanWater->setHeightMapScale(1.0f, false);
        }
    } else if (mUnlockedPhase == 3) {
        s32 step = al::getNerveStep(this);

        if (oceanWater != nullptr && step == 440) {
            oceanWater->setHeightMapScale(1.0f, false);
        }
    } else if (mUnlockedPhase == 1) {
        s32 step = al::getNerveStep(this);

        if (oceanWater != nullptr && step == 510) {
            oceanWater->setHeightMapScale(1.0f, false);
        }
    }

    if (!mIntroFlyOverCamera->isEndCameraPlay()) {
        return;
    }

    al::stopPadRumble(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);

    if (!checkDemoIntroChange(true)) {
        getDemoDirector()->setUnknownD4(false);
        s32 cutsceneId;

        if (mUnlockedPhase == 3) {
            cutsceneId = 24;
        } else if (mUnlockedPhase == 8) {
            cutsceneId = 27;
        } else if (mUnlockedPhase == 5) {
            cutsceneId = 26;
        } else {
            mScreenCoverCtrl->requestCaptureScreenCover(10);
            cutsceneId = -1;
        }

        if (cutsceneId >= 0 &&
            !SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                     cutsceneId)) {
            if (mIntroFlyOverCamera != nullptr) {
                mIntroFlyOverCamera->finishCameraPlay();
            }

            mDemoCutscene->setCutsceneId(cutsceneId);
            al::pausePadRumble(this);
            al::setNerve(this, &NrvSingleModeSceneDemoFadeTransition);
            mScreenCaptureExecutor->requestCapture(false, 1, false);
            mWipeFadeBlack->startClose(-1);
            return;
        }

        if (mIntroFlyOverCamera != nullptr) {
            mIntroFlyOverCamera->finalFinishCameraPlay(true);
        }

        mScreenCaptureExecutor->resetRequest(1);
        alSeFunction::changeListenerPoserLast(mAudioDirector);
        al::setNerve(this, &NrvSingleModeScenePlay);
        SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
        mWindowProcessing->appearWithSystemMessage("SaveSequence", "WindowProcessing_Save", 90,
                                                   false);
        mWindowProcessing->setUnknown121(true);
        mSceneLayout->endDemo(true, true);
        rc::unHideGuideGameWindow(this);
        finishDemoIntro();
    }

    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
}

/**
 * Plays a demo of the players or of a binding object.
 */
void SingleModeScene::exeDemoScene() {
    if (al::isFirstStep(this)) {
        mAudioDemoType = getDemoDirector()->getAudioDemoType();
        alAudioSystemFunction::startDemo(mAudioDirector,
                                         static_cast<alSeFunction::DemoType>(mAudioDemoType));
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointer();
        mSceneLayout->startDemo(false, true);
        rc::hideGuideGameWindow(this);
        mPlayerAliveWatcher->startDemo();
        rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);
    }

    updatePlay();
    updateAudioDemoType(&mAudioDemoType, this);

    if (al::isAnyActiveDemo(this)) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    mSceneLayout->endDemo(true, false);
    rc::unHideGuideGameWindow(this);
    mPlayerAliveWatcher->endDemo();
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Plays a cutscene.
 */
void SingleModeScene::exeDemoCutscene() {
    if (al::isFirstStep(this)) {
        mAudioDemoType = getDemoDirector()->getAudioDemoType();
        alAudioSystemFunction::startDemo(mAudioDirector,
                                         static_cast<alSeFunction::DemoType>(mAudioDemoType));
        al::changeBgmSituation(this, "VolumeHalf");
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointer();
        mSceneLayout->startDemo(false, true);
        rc::hideGuideGameWindow(this);
        mPlayerAliveWatcher->startDemo();
        rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);
        al::pauseDemoPadRumble(this);
    }

    updateDemoCutscene(false, false);

    if (al::getImmediateDemoSwitch(this)) {
        al::resetImmediateDemoSwitch(this);

        if (rc::isActiveDemoInGameCutscene(this)) {
            mIsInGameCutsceneSwitch = true;
            alSeFunction::changeListenerPoserLast(mAudioDirector);
            al::changeBgmSituation(this, "VolumeUp");
            al::setNerve(this, &NrvSingleModeSceneDemoInGameCutscene);
            return;
        }
    }

    if (SingleModeDataFunction::isPhaseEnd(this)) {
        handlePhaseEnd();
        return;
    }

    updateAudioDemoType(&mAudioDemoType, this);

    if (al::isAnyActiveDemo(this)) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    al::changeBgmSituation(this, "VolumeUp");
    mSceneLayout->endDemo(true, false);
    rc::unHideGuideGameWindow(this);
    mPlayerAliveWatcher->endDemo();
    getDemoDirector()->setUnknownD4(false);
    getDemoDirector()->setUnknownE1(false);
    getDemoDirector()->setUnknownE2(false);
    al::endPauseDemoPadRumble(this);
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Updates the scene during a cutscene.
 * @param isUpdatePlayer Whether the players are updated.
 * @param isCheckStopScene Whether the scene stop is checked when the players are updated.
 */
void SingleModeScene::updateDemoCutscene(bool isUpdatePlayer, bool isCheckStopScene) {
    if ((!isUpdatePlayer || isCheckStopScene) && al::isStopScene(this)) {
        al::updateKitList(this, "2D StopScene");
        al::updateEffectSystem(this);

        if (al::isStopAndUpdateCamera(this)) {
            mLiveActorKit->getCameraDirector_RS()->execute();
        }

        return;
    }

    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    mLiveActorKit->getCameraDirector_RS()->execute();
    mLiveActorKit->updatePadRumble();
    al::updateKitList(this, "OceanWaterRequest");
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "ライト管理");
    al::updateKitList(this, "Clipping");
    al::updateKitListPaused(this, "ModelUpdate");
    updateDemoCutsceneAddOn();
    al::updateKitList(this, "コリジョンディレクター");
    al::updateKitList(this, "デモ");
    al::updateKitList(this, "空");

    if (isUpdatePlayer) {
        al::updateKitList(this, "プレイヤー[Movement]");
        al::updateKitList(this, "プレイヤー");
        al::updateKitList(this, "プレイヤー装飾");
        al::updateKitList(this, "プレイヤー装飾２");
    }

    if (getDemoDirector()->isUnknownE1()) {
        al::updateKitList(this, "アイテム");
    }

    DisasterModeController* disasterController = DisasterModeController::tryGetController(this);

    if (disasterController != nullptr) {
        disasterController->movementPaused(false);
    }

    al::updateKitList(this, "デモオブジェクト");

    if (getDemoDirector()->isUnknownD5()) {
        al::updateHitSensorDirector(this);
    }

    if (getDemoDirector()->isUnknownD4()) {
        al::updateDemoActor(this);
    } else {
        al::updateDemoActorWithEffects(this);
    }

    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateKitList(this, "コインローテータ");
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "2D StopScene");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateKitList(this, "Water");

    if (getDemoDirector()->isUnknownD4()) {
        al::updateEffect(this);
    } else {
        al::updateEffectDemo(this);
    }

    mLiveActorKit->updateReducedBufferEffect();
    mLiveActorKit->getExecuteDirector()->finishExecute();
    mLiveActorKit->getClippingDirector()->executeRequestAsyncUpdate();
    mLiveActorKit->updateGraphics(false);
}

/**
 * Plays a cutscene of the players.
 */
void SingleModeScene::exeDemoPlayerScene() {
    if (al::isFirstStep(this)) {
        if (mIsInGameCutsceneSwitch) {
            s32 audioDemoType = mAudioDemoType;
            mIsInGameCutsceneSwitch = false;

            if (audioDemoType != getDemoDirector()->getAudioDemoType()) {
                alAudioSystemFunction::endDemo(
                    mAudioDirector, static_cast<alSeFunction::DemoType>(mAudioDemoType));
                mAudioDemoType = getDemoDirector()->getAudioDemoType();
                alAudioSystemFunction::startDemo(
                    mAudioDirector, static_cast<alSeFunction::DemoType>(mAudioDemoType));
            }
        } else {
            al::pauseDemoPadRumble(this);
            mAudioDemoType = getDemoDirector()->getAudioDemoType();
            alAudioSystemFunction::startDemo(mAudioDirector,
                                             static_cast<alSeFunction::DemoType>(mAudioDemoType));
            mDrcAssistDirectorList->disappearTouchPointer();
            mSceneLayout->startDemo(false, true);
            rc::hideGuideGameWindow(this);
            mPlayerAliveWatcher->startDemo();
            rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), true);
            rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);
        }
    }

    invalidatePlayerInput();
    updateDemoCutscene(true, true);

    if (SingleModeDataFunction::isPhaseEnd(this)) {
        handlePhaseEnd();
        return;
    }

    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

    if (koopaJr != nullptr && koopaJr->isWaitingForOptionsMenu()) {
        mScreenCaptureExecutor->requestCapture(false, 1, true);
        al::setNerve(this, &NrvSingleModeSceneWaitForKoopaJrOptionsIntro);
        return;
    }

    updateAudioDemoType(&mAudioDemoType, this);

    if (getDemoDirector()->isUnknownE3()) {
        mPlayerAliveWatcher->update();

        if (checkGameOver()) {
            return;
        }
    }

    if (al::isAnyActiveDemo(this)) {
        return;
    }

    getDemoDirector()->setUnknownE3(false);
    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), true);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    mSceneLayout->endDemo(true, false);
    rc::unHideGuideGameWindow(this);
    mPlayerAliveWatcher->endDemo();
    getDemoDirector()->setUnknownD4(false);
    getDemoDirector()->setUnknownE1(false);
    getDemoDirector()->setUnknownE2(false);
    al::endPauseDemoPadRumble(this);
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Plays the Bowser Jr. cutscene introducing a phase.
 */
void SingleModeScene::exeDemoKoopaJrPhaseIntro() {
    if (al::isFirstStep(this)) {
        s32 cutsceneId = mDemoCutscene->getCutsceneId();
        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

        if (koopaJr != nullptr) {
            mDemoCutscene->setHideActor(koopaJr);
        }

        if (cutsceneId != 25) {
            al::calcFrontDir(&sPlayerFrontDir, mLiveActorKit->getPlayerHolder()->getPlayer(0));
            mDemoCutscene->overrideBaseMtx(&mDemoBaseMtx);

            if (mIntroFlyOverCamera != nullptr && rc::isActiveDemoCutscene(mIntroFlyOverCamera)) {
                rc::requestEndDemoCutscene(mIntroFlyOverCamera);
            }

            mDemoCutscene->startDemo();

            if (!alAudioSystemFunction::isInDemo(mAudioDirector) ||
                mAudioDemoType != getDemoDirector()->getAudioDemoType()) {
                if (alAudioSystemFunction::isInDemo(mAudioDirector)) {
                    alAudioSystemFunction::endDemo(mAudioDirector,
                                                   static_cast<alSeFunction::DemoType>(0));
                }

                mAudioDemoType = getDemoDirector()->getAudioDemoType();
                alAudioSystemFunction::startDemo(
                    mAudioDirector, static_cast<alSeFunction::DemoType>(mAudioDemoType));
            }

            rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), true);
            rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);

            if (cutsceneId == 15) {
                goto openWipe;
            }

            if (cutsceneId == 17) {
                DisasterModeController* disasterController =
                    DisasterModeController::tryGetController(this);

                if (disasterController != nullptr) {
                    rc::addDemoActor(disasterController);
                }

                goto openWipe;
            }
        }

        mSceneLayout->setAreaNamePhaseStart(false);
    }

openWipe:
    if (al::isStep(this, 4)) {
        mScreenCaptureExecutor->offDraw(1);
        mWipeFadeBlack->startOpen(-1);
    }

    invalidatePlayerInput();
    updateDemoCutscene(true, false);
    updateAudioDemoType(&mAudioDemoType, this);

    if (!mWipeFadeBlack->isAlive()) {
        if (!mDemoCutscene->isFullyStarted() && !mDemoCutscene->isEndDemo()) {
            return;
        }

        if (!SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                     mDemoCutscene->getCutsceneId())) {
            SingleModeDataFunction::setHasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                       mDemoCutscene->getCutsceneId());
            mDemoCutscene->setGuideWindowState();
            return;
        }

        s32 cutsceneId = mDemoCutscene->getCutsceneId();

        if (cutsceneId == 27 || cutsceneId == 24) {
            if (mGuideGameWindow->isWaitConfirm()) {
                al::disableChangeSituation(this);
                al::disableVolumeChange(this);
                return;
            }

            if (cutsceneId == 27) {
                SingleModeDataFunction::setIslandCheckpointPass(
                    GameDataHolderAccessor(mGameDataHolder), 1);
            }

            SingleModeDataFunction::setHasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                       cutsceneId == 24 ? 21 : 23);
            mPausePort = al::getMainControllerPort();
            mSceneLayout->startPause(true);
            alAudioSystemFunction::endDemo(mAudioDirector,
                                           static_cast<alSeFunction::DemoType>(mAudioDemoType));
            al::offClippingPosAsPlayerPos(this);
            mScreenCaptureExecutor->requestCapture(false, 1, true);

            if (cutsceneId == 24) {
                al::setNerve(this, &NrvSingleModeSceneDemoIslandMapUnlock);
            } else {
                al::setNerve(this, &NrvSingleModeSceneDemoIslandMapUnlockPreGame);
            }

            return;
        }
    }

    if (!mDemoCutscene->isEndDemo()) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), true);
    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

    if (koopaJr != nullptr) {
        al::showModelIfHide(koopaJr);
        al::showShadow(koopaJr);
    }

    SingleModeDataFunction::setHasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                               mDemoCutscene->getCutsceneId());
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    s32 islandMapCutsceneId = -1;

    if (mUnlockedPhase == 3) {
        islandMapCutsceneId = 21;
    } else if (mUnlockedPhase == 8) {
        islandMapCutsceneId = 23;
    } else if (mUnlockedPhase == 5) {
        islandMapCutsceneId = 22;
    }

    if (islandMapCutsceneId >= 0 &&
        !SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                 islandMapCutsceneId)) {
        SingleModeDataFunction::setHasSeenCutscene(GameDataHolderAccessor(mGameDataHolder),
                                                   islandMapCutsceneId);
        mPausePort = al::getMainControllerPort();
        mSceneLayout->startPause(true);

        if (islandMapCutsceneId == 21) {
            mScreenCaptureExecutor->requestCapture(true, 1, true);
            al::setNerve(this, &NrvSingleModeSceneDemoIslandMap);
        } else {
            mScreenCaptureExecutor->requestCapture(false, 1, false);
            al::setNerve(this, &NrvSingleModeSceneDemoFadeTransitionToIslandMap);
            mWipeFadeBlack->startClose(-1);
        }

        return;
    }

    if (mDemoCutscene->getCutsceneId() == 25) {
        mScreenCaptureExecutor->requestCapture(false, 1, false);
        mWipeFadeBlack->startClose(-1);
        al::setNerve(this, &NrvSingleModeSceneDemoFadeTransitionFromIslandMap);
        updatePlay();
    } else {
        mScreenCaptureExecutor->resetRequest(1);
        mScreenCoverCtrl->mIsRequestCaptureScene = false;
        mScreenCoverCtrl->requestCaptureScreenCover(10);
        al::enableChangeSituation(this);
        al::enableVolumeChange(this);
        al::endPausePadRumble(this);
        al::setNerve(this, &NrvSingleModeScenePlay);
    }

    mSceneLayout->endDemo(true, true);
    mSceneLayout->endPause();
    rc::unHideGuideGameWindow(this);
}

/**
 * Plays the second part of the Bowser Jr. cutscene introducing a phase, after the island map.
 */
void SingleModeScene::exeDemoKoopaJrPhaseIntro2Part() {
    if (al::isFirstStep(this)) {
        mScreenCaptureExecutor->offDraw(1);
        alAudioSystemFunction::startDemo(mAudioDirector,
                                         static_cast<alSeFunction::DemoType>(mAudioDemoType));
    }

    updateDemoCutscene(true, false);
    invalidatePlayerInput();

    if (!mDemoCutscene->isEndDemo() || rc::isGuideGameWindowActive(this)) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    mWipeFadeBlack->startClose(-1);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

    if (koopaJr != nullptr) {
        al::showModelIfHide(koopaJr);
        al::showShadow(koopaJr);
    }

    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), true);
    al::setNerve(this, &NrvSingleModeSceneDemoFadeTransitionFromIslandMap);
}

/**
 * Goes back to the play state after the island map shown before the game starts.
 */
void SingleModeScene::exeDemoIslandMapPreGame() {
    updatePauseLayout();

    if (al::isFirstStep(this)) {
        mSceneLayout->prepEndDemo();
        mScreenCoverCtrl->requestCaptureScreenCover(4);
        return;
    }

    al::endPausePadRumble(this);
    al::setNerve(this, &NrvSingleModeScenePlay);
    mScreenCaptureExecutor->offDraw(1);
    al::offClippingPosAsPlayerPos(this);
    mSceneLayout->endDemo(true, true);
    rc::unHideGuideGameWindow(this);
    alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
    al::changeBgmSituation(this, "VolumeUp");

    if (mGuideGameWindow != nullptr) {
        mGuideGameWindow->endHide(this);
    }

    mWindowProcessing->appearWithSystemMessage("SaveSequence", "WindowProcessing_Save", 90, false);
    mWindowProcessing->setUnknown121(true);
    updateLayout();
}

/**
 * Updates the layouts shown on the paused scene.
 */
void SingleModeScene::updatePauseLayout() {
    al::updatePadRumbleDirector(this);
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Updates the 2D layouts of the scene.
 */
void SingleModeScene::updateLayout() {
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "2D StopScene");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Shows the island map once the screen faded out.
 */
void SingleModeScene::exeDemoIslandMap() {
    invalidatePlayerInput();

    if (mWipeFadeBlack->isAlive()) {
        if (al::isFirstStep(this) && mScreenCaptureExecutor->isDraw(1)) {
            mScreenCaptureExecutor->offDraw(1);
        }

        updatePlay();
        return;
    }

    if (!mScreenCaptureExecutor->isDraw(1)) {
        mScreenCaptureExecutor->requestCapture(true, 1, true);
    }

    if (!mIslandMap->isAlive()) {
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, true, 0);
        mIslandMap->startAppear(mPausePort, true);
        al::changeBgmSituation(this, "VolumeHalf");
    }

    updatePauseLayout();

    if (mIslandMap->isEnd()) {
        al::setNerve(this, &NrvSingleModeSceneDemoIslandMapPreGame);
        mSceneLayout->endPause();
        return;
    }

    if (!mIslandMap->isAlive() && mUnlockedPhase == 3) {
        mIslandMap->startAppear(mPausePort, true);
    }
}

/**
 * Shows the island map between the two parts of a Bowser Jr. cutscene.
 */
void SingleModeScene::exeDemoIslandMap2Part() {
    if (al::isFirstStep(this)) {
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, true, 0);
        mIslandMap->startAppear(mPausePort, true);
    }

    updatePauseLayout();

    if (!mIslandMap->isEnd()) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    mSceneLayout->endPause();

    if (al::isNerve(this, &NrvSingleModeSceneDemoIslandMapUnlock)) {
        mDemoCutscene->setCutsceneId(25);
    } else if (al::isNerve(this, &NrvSingleModeSceneDemoIslandMapUnlockPreGame)) {
        mDemoCutscene->setCutsceneId(28);
    }

    alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
    mDemoCutscene->setGuideWindowState();
    al::setNerve(this, &NrvSingleModeSceneDemoKoopaJrPhaseIntro2Part);
}

/**
 * Shows the options menu introduced by Bowser Jr. during his cutscene.
 */
void SingleModeScene::exeWaitForKoopaJrOptionsIntro() {
    if (al::isFirstStep(this)) {
        updateDemoCutscene(true, true);
        mPauseMenu->appearKoopaJrOptions();
    }

    updatePauseLayout();

    if (mPauseMenu->isAlive()) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    mScreenCaptureExecutor->offDraw(1);
    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

    if (koopaJr != nullptr) {
        koopaJr->endOptionsIntro();
    }

    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Shows the options menu introduced by Bowser Jr.
 */
void SingleModeScene::exeKoopaJrOptionsIntro() {
    if (al::isFirstStep(this)) {
        mPauseMenu->appearKoopaJrOptions();
    }

    updatePauseLayout();

    if (mPauseMenu->isAlive()) {
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), true);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    mSceneLayout->endDemo(true, false);
    rc::unHideGuideGameWindow(this);
    mPlayerAliveWatcher->endDemo();
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Plays a cutscene during which the game keeps running.
 */
void SingleModeScene::exeDemoInGameCutscene() {
    if (al::isFirstStep(this)) {
        if (mIsInGameCutsceneSwitch) {
            s32 audioDemoType = mAudioDemoType;
            mIsInGameCutsceneSwitch = false;

            if (audioDemoType != getDemoDirector()->getAudioDemoType()) {
                alAudioSystemFunction::endDemo(
                    mAudioDirector, static_cast<alSeFunction::DemoType>(mAudioDemoType));
                mAudioDemoType = getDemoDirector()->getAudioDemoType();
                alAudioSystemFunction::startDemo(
                    mAudioDirector, static_cast<alSeFunction::DemoType>(mAudioDemoType));
            }
        } else {
            al::pauseDemoPadRumble(this);
            mAudioDemoType = getDemoDirector()->getAudioDemoType();
            alAudioSystemFunction::startDemo(mAudioDirector,
                                             static_cast<alSeFunction::DemoType>(mAudioDemoType));
            mDrcAssistDirectorList->disappearTouchPointer();
            mSceneLayout->startDemo(false, true);
            rc::hideGuideGameWindow(this);
            mPlayerAliveWatcher->startDemo();
            rc::tryPauseAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
            rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), true);
        }

        GigaBellManager* gigaBellManager =
            al::tryGetSceneObj<GigaBellManager>(this, SceneObjID_GigaBellManager);

        if (gigaBellManager != nullptr && gigaBellManager->isInUnlockCutscene()) {
            mIsGigaBellUnlockCutscene = true;
        }
    }

    updateDemoCutscene(false, false);

    if (SingleModeDataFunction::isPhaseEnd(this)) {
        handlePhaseEnd();
        return;
    }

    if (al::getImmediateDemoSwitch(this)) {
        al::resetImmediateDemoSwitch(this);

        if (rc::isActiveDemoPlayerCutscene(this)) {
            mIsInGameCutsceneSwitch = true;
            al::setNerve(this, &NrvSingleModeSceneDemoPlayerScene);
            return;
        }

        if (rc::isActiveDemoInGameCutscene(this)) {
            mIsInGameCutsceneSwitch = true;
            al::setNerve(this, &NrvSingleModeSceneDemoInGameCutscene);
            return;
        }
    }

    updateAudioDemoType(&mAudioDemoType, this);

    if (al::isAnyActiveDemo(this)) {
        return;
    }

    getDemoDirector()->setUnknownD4(false);
    getDemoDirector()->setUnknownE1(false);
    al::endPauseDemoPadRumble(this);
    getDemoDirector()->setUnknownE2(false);

    if (mIsGigaBellUnlockCutscene) {
        mScreenCaptureExecutor->requestCapture(false, 1, false);
        SingleModeDataFunction::setPhaseEnd(this, true);
        kill();
        return;
    }

    al::offClippingPosAsPlayerPos(this);
    rc::tryResumeAllPlayerInvincible(mLiveActorKit->getPlayerHolder(), false);
    alAudioSystemFunction::endDemo(mAudioDirector,
                                   static_cast<alSeFunction::DemoType>(mAudioDemoType));
    mSceneLayout->endDemo(true, false);
    mPlayerAliveWatcher->endDemo();
    rc::unHideGuideGameWindow(this);
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Ends the scene with the goal.
 */
void SingleModeScene::exeGoal() {
    if (al::isFirstStep(this)) {
        // Nothing to start: the goal ends the scene right away.
    }

    kill();
}

/**
 * Fades the screen out and starts the next demo or the island map.
 */
void SingleModeScene::exeDemoFadeTransition() {
    invalidatePlayerInput();
    updatePlay();
    bool isCloseEnd = mWipeFadeBlack->isCloseEnd();
    bool isFromIntro = al::isNerve(this, &NrvSingleModeSceneDemoFadeTransition);

    if (!isCloseEnd) {
        if (isFromIntro && mUnlockedPhase == 3) {
            SingleModeDataFunction::hasSeenCutscene(this, 24);
        }

        return;
    }

    bool isKeepClosed;

    if (isFromIntro) {
        if (mIntroFlyOverCamera != nullptr) {
            mIntroFlyOverCamera->finalFinishCameraPlay(false);
        }

        finishDemoIntro();
        mScreenCaptureExecutor->resetRequest(1);
        isKeepClosed = true;
        al::setNerve(this, &NrvSingleModeSceneDemoKoopaJrPhaseIntro);
    } else {
        if (al::isNerve(this, &NrvSingleModeSceneDemoFadeTransitionToIslandMap)) {
            al::setNerve(this, &NrvSingleModeSceneDemoIslandMap);
        } else {
            mSceneLayout->endDemo(true, true);
            rc::unHideGuideGameWindow(this);
            mWindowProcessing->appearWithSystemMessage("SaveSequence", "WindowProcessing_Save", 90,
                                                       false);
            mWindowProcessing->setUnknown121(true);
            al::endPausePadRumble(this);
            al::setNerve(this, &NrvSingleModeScenePlay);
        }

        isKeepClosed = false;
    }

    if (mDemoCutscene->getCutsceneId() != 25) {
        al::CameraPoser_RS* poser =
            mLiveActorKit->getCameraDirector_RS()->getCurrentTicket()->getPoser();
        al::CameraTurnInfo turnInfo;
        turnInfo.mRequesterName = getName();
        turnInfo.mDir = sPlayerFrontDir;
        turnInfo._14 = 0.0f;
        turnInfo._18 = 1.0f;
        turnInfo._1c = true;
        turnInfo._1d = false;
        poser->requestTurnToDirection(&turnInfo);
    }

    if (!isKeepClosed) {
        mScreenCaptureExecutor->offDraw(1);
        mWipeFadeBlack->startOpen(-1);
    }
}

/**
 * Shows the island map opened from the play state.
 */
void SingleModeScene::exePauseIslandMap() {
    invalidatePlayerInput();

    if (al::isFirstStep(this)) {
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, true, 0);
        mIslandMap->startAppear(mPausePort, false);
        updateLayout();
        al::changeBgmSituation(this, "VolumeHalf");
        al::pausePadRumble(this);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), false);
    }

    updatePauseLayout();

    if (!mIslandMap->isEnd()) {
        return;
    }

    if (mIslandMap->isEndOnWarp() && mIslandWarpState != nullptr) {
        al::setNerve(this, &NrvSingleModeSceneIslandWarp);
        mIslandWarpState->setIslandWarpDest(mIslandMap->getWarpIslandId());
        return;
    }

    al::setNerve(this, &NrvSingleModeScenePlay);
    mScreenCaptureExecutor->offDraw(1);
    mSceneLayout->endPause();
    alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
    al::changeBgmSituation(this, "VolumeUp");

    if (mGuideGameWindow != nullptr) {
        mGuideGameWindow->endHide(this);
    }

    updateLayout();
    al::endPausePadRumble(this);
    rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
}

/**
 * Shows the item stock opened from the play state.
 */
void SingleModeScene::exePauseItemSelect() {
    invalidatePlayerInput();

    if (al::isFirstStep(this)) {
        mSceneLayout->appearItemStock(mPausePort);
        mSceneLayout->pauseTimer(true);
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, true, 0);
        al::pausePadRumble(this);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), false);
        al::changeBgmSituation(this, "VolumeHalf");

        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->startHide(this);
        }
    }

    al::updatePadRumbleDirector(this);
    updateLayout();

    if (mSceneLayout->isItemStockStartClose()) {
        al::changeBgmSituation(this, "VolumeUp");
    }

    if (!mSceneLayout->isItemStockEnd()) {
        return;
    }

    rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
    mScreenCaptureExecutor->offDraw(1);
    mSceneLayout->endPause();

    if (mGuideGameWindow != nullptr) {
        mGuideGameWindow->endHide(this);
    }

    al::setNerve(this, &NrvSingleModeScenePlay);
    al::endPausePadRumble(this);
    alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
    updateLayout();
}

/**
 * Pauses the scene while a guide window waits for a confirmation.
 */
void SingleModeScene::exePauseWindowMessage() {
    if (al::isFirstStep(this)) {
        rc::hideGuideGameWindow(this);
        al::pausePadRumble(this);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), false);
        mSceneLayout->startDemo(false, true);
    }

    invalidatePlayerInput();

    if (mGuideGameWindow->isWindowMessageActive()) {
        updateLayout();
    } else {
        updatePlay();
    }

    if (mGuideGameWindow->isWaitConfirm()) {
        return;
    }

    al::endPausePadRumble(this);
    al::changeBgmSituation(this, "VolumeUp");
    rc::unHideGuideGameWindow(this);
    rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
    al::setNerve(this, &NrvSingleModeScenePlay);
    mSceneLayout->endDemo(true, false);
}

/**
 * Shows the pause menu.
 */
void SingleModeScene::exePause() {
    if (al::isFirstStep(this)) {
        al::pausePadRumble(this);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), false);
        alAudioSystemFunction::pauseSystem(mAudioDirector, this, true, 0);
        s32 islandIndex;

        if (mUnlockedPhase == 0) {
            islandIndex = -1;
        } else if (mUnlockedPhase == cPhasePlessieChase2 ||
                   mUnlockedPhase == cPhasePlessieChase1) {
            islandIndex = mIslandKeeper->getUnknown14();
        } else {
            islandIndex = mIslandKeeper->getCurrentIslandId();
        }

        mGameDataHolder->updatePlayerFigures(-1);
        mPauseMenu->setPauseMenuHeader(islandIndex);
        mPauseMenu->appear(mPausePort);
        return;
    }

    if (al::isStep(this, 2)) {
        mSceneLayout->startPause(true);

        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->startHide(this);
        }
    }

    updateLayout();
    updatePauseLayout();
    s32 port = -1;

    if (isTriggerPause(&port) && (!al::isPadConnected(mPausePort) || port == mPausePort)) {
        if (mPauseMenu->isWait()) {
            mPauseMenu->decideBack();
        }
    } else if (mPauseMenu->isDecideGameChange()) {
        mScreenCaptureExecutor->offDraw(1);
        al::setForceSceneHeapResourceDestroy();
        al::setNerve(this, &NrvSingleModeSceneGameChange);
        return;
    } else if (mPauseMenu->isEndLoad()) {
        al::setForceSceneHeapResourceDestroy();
        al::setNerve(this, &NrvSingleModeSceneLoadGame);
        return;
    }

    if (mPauseMenu->isDecideQuit()) {
        mSceneLayout->endPause();

        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->endHide(this);
        }

        s32 phase = mUnlockedPhase;

        if (phase == 2 || phase == 4 || phase == 6) {
            if (!SingleModeDataFunction::beginPlayReport(
                    this, this, static_cast<preport::KeyEventType>(21), 3, 0)) {
                SingleModeDataFunction::setPlayReportData(
                    this, static_cast<preport::Key>(19),
                    SingleModeDataFunction::getIs2PAssistMode(this));
                SingleModeDataFunction::setPlayReportData(
                    this, static_cast<preport::Key>(46),
                    SingleModeDataFunction::getBossPlayTime(
                        GameDataHolderAccessor(mGameDataHolder)));
                SingleModeDataFunction::setPlayReportData(this, static_cast<preport::Key>(47), 2);
                SingleModeDataFunction::endPlayReport(this);
            }
        } else {
            SingleModeDataFunction::reportIslandEvent(this, this, 2, -1);
        }

        al::setForceSceneHeapResourceDestroy();
        al::setNerve(this, &NrvSingleModeSceneRetire);
        return;
    }

    if (!mPauseMenu->isEnd() && !mPauseMenu->isEndAssistMode()) {
        return;
    }

    mScreenCaptureExecutor->offDraw(1);
    mPauseMenu->kill();
    rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
    bool isDecideBack = mPauseMenu->isDecideBack();
    mSceneLayout->endPause();

    if (isDecideBack) {
        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->endHide(this);
        }

        al::setNerve(this, &NrvSingleModeScenePlay);
    } else {
        al::setNerve(this, &NrvSingleModeScenePlay);

        if (mGuideGameWindow != nullptr) {
            mGuideGameWindow->endHide(this);
        }
    }

    al::endPausePadRumble(this);
    alAudioSystemFunction::pauseSystem(mAudioDirector, this, false, 0);
    updateLayout();
}

/**
 * Plays the miss demo and restarts the scene.
 */
void SingleModeScene::exeRestart() {
    if (al::isFirstStep(this)) {
        mLiveActorKit->getCameraDirector_RS()->setActiveInputNum(0);
        mSceneLayout->kill();
        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

        if (koopaJr != nullptr) {
            rc::addDemoActor(koopaJr);
        }
    }

    updateMissDemo(true, false);

    if (al::isFirstStep(this)) {
        alSeFunction::stopAllSeWithExceptList(mAudioDirector, "ミス", 0);
        alSeFunction::setIsStateAfterGoal(mAudioDirector, true);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ終了", 30,
                                                    false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "ステージ終了", 30,
                                                    false);
        mPlayerAliveWatcher->onGameOver();
        al::disableBgmStart(this);
        al::disableBgmChangeArea(this);
        al::disableLineChange(this, true);
        al::changeAudioEffect(this, nullptr);

        for (s32 i = 0; i < al::getPlayerNumMax(mLiveActorKit->getPlayerHolder()); i++) {
            al::setCameraCalcTargetFlag(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i),
                                        false);
        }
    }

    if (al::isStep(this, 50)) {
        al::stopAllBgm(this, 130);
    }

    if (al::isStep(this, 119)) {
        mWipeMiss->startClose(-1);

        if (mAudioDirector->getSeDirector() != nullptr) {
            al::startSe(this, "PgBowserVoice", nullptr);
        }
    }

    if (al::isStep(this, 190)) {
        SingleModeDataFunction::restartStage(GameDataHolderAccessor(mGameDataHolder));
        kill();
    }
}

/**
 * Updates the scene during the miss demo.
 * @param isUpdateGraphics Whether the graphics are updated.
 * @param isSkipVehicle Whether the vehicles are not updated.
 */
void SingleModeScene::updateMissDemo(bool isUpdateGraphics, bool isSkipVehicle) {
    mLiveActorKit->getClippingDirector()->waitPendingClippingRequest();
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    mLiveActorKit->getCameraDirector_RS()->execute();

    if (!isSkipVehicle) {
        al::updateKitList(this, "乗り物");
    }

    al::updateKitList(this, "デモプレイヤーロケーター");
    al::updateKitList(this, "デモプレイヤー前処理");
    al::updateKitList(this, "プレイヤー前処理");
    al::updateKitList(this, "プレイヤー[Movement]");
    al::updateKitList(this, "プレイヤー");
    al::updateKitList(this, "プレイヤー後処理");
    al::updateKitList(this, "プレイヤー装飾");
    al::updateKitList(this, "プレイヤー装飾２");
    al::updateKitList(this, "デモ");
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateEffectPlayer(this);
    mLiveActorKit->updateReducedBufferEffect();
    al::updateDemoActorWithEffects(this);

    if (al::isLessEqualStep(this, 30)) {
        mPlayerAliveWatcher->update();
        al::updateKitList(this, "２Ｄ");
        al::updateKitList(this, "2D StopScene");
    } else {
        for (s32 i = 0; i < al::getPlayerNumMax(mLiveActorKit->getPlayerHolder()); i++) {
            al::setCameraCalcTargetFlag(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i),
                                        false);
        }
    }

    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    mLiveActorKit->getExecuteDirector()->finishExecute();

    if (isUpdateGraphics) {
        mLiveActorKit->updateGraphics(false);
    }
}

/**
 * Ends the scene to reenter the stage.
 */
void SingleModeScene::exeReenterStage() {
    kill();
}

/**
 * Ends the scene after the player retired.
 */
void SingleModeScene::exeRetire() {
    kill();
}

/**
 * Stops the audio and ends the scene once the game ends.
 */
void SingleModeScene::exeGameEnd() {
    if (al::isFirstStep(this)) {
        alSeFunction::stopAllSeWithExceptList(mAudioDirector, "ミス", 0);
        alSeFunction::setIsStateAfterGoal(mAudioDirector, true);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ終了", 30,
                                                    false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "ステージ終了", 30,
                                                    false);
        mPlayerAliveWatcher->onGameOver();
        al::stopAllBgm(this, -1);
        al::disableBgmStart(this);
        al::changeAudioEffect(this, nullptr);

        for (s32 i = 0; i < al::getPlayerNumMax(mLiveActorKit->getPlayerHolder()); i++) {
            al::setCameraCalcTargetFlag(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i),
                                        false);
        }
    }

    if (al::isNerve(this, &NrvSingleModeSceneLoadGame) && !mPauseMenu->isEndLoad()) {
        return;
    }

    kill();
}

/**
 * Waits before the time up; nothing happens in Bowser's Fury.
 */
void SingleModeScene::exePreTimeUp() {}

/**
 * Handles the time up; nothing happens in Bowser's Fury.
 */
void SingleModeScene::exeTimeUp() {}

/**
 * Runs the snapshot mode.
 */
void SingleModeScene::exeCaptureMode() {
    if (al::isFirstStep(this)) {
        al::pausePadRumble(this);
        al::updateKitList(this, "２Ｄ");
        al::updateKitList(this, "2D StopScene");
        al::ClippingDirectorBase::sLODDisabled = true;
        al::ClippingDirectorBase::sCollisionForcedOn = true;
        al::forceUpdateLOD(mLiveActorKit, true);
        mLiveActorKit->getGraphicsSystemInfo()->getShadowDirector()->setUnknown1ef8(true);
        al::onClippingPosAsPlayerPos(this);
    }

    if (!al::updateNerveState(this) || mSnapshotLayout->isAlive()) {
        return;
    }

    al::ClippingDirectorBase::sLODDisabled = false;
    al::ClippingDirectorBase::sCollisionForcedOn = false;
    al::offClippingPosAsPlayerPos(this);
    al::forceUpdateLOD(mLiveActorKit, false);
    mLiveActorKit->getGraphicsSystemInfo()->getShadowDirector()->setUnknown1ef8(false);
    al::setNerve(this, &NrvSingleModeScenePlay);
    mSceneLayout->endPause();
    al::endPausePadRumble(this);
}

/**
 * Warps the player to the island chosen on the island map.
 */
void SingleModeScene::exeIslandWarp() {
    if (al::isFirstStep(this)) {
        mLiveActorKit->getGraphicsSystemInfo()->getGraphicsAreaDirector()->lockArea();
        mLiveActorKit->getAreaObjDirector()->setEnableAll(false);
        mIslandWarpState->startDemo();
        rc::requestStartDemoPlayerCutscene(mIslandWarpState);
    }

    updateIslandWarp();

    if (!mIslandWarpState->isEndDemo()) {
        return;
    }

    mWipeWorldJump->tryStartOpen(30);
    mInvalidateInputFrames = 60;
    DisasterModeController* disasterController = DisasterModeController::tryGetController(this);

    if (disasterController == nullptr || !disasterController->isDisasterMode()) {
        al::changeIslandMapBgmVolume(this, -1, -1, false);
    }

    mScreenCaptureExecutor->offDraw(1);
    mSceneLayout->endPause();
    mSceneLayout->handleIslandWarp();
    al::endPausePadRumble(this);
    alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
    al::changeBgmSituation(this, "VolumeUp");

    if (mGuideGameWindow != nullptr) {
        mGuideGameWindow->endHide(this);
    }

    updateLayout();
    rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
    rc::requestEndDemoPlayerCutscene(mIslandWarpState);
    mLiveActorKit->getCameraDirector_RS()->freezeCameraInput(false);
    al::setNerve(this, &NrvSingleModeScenePlay);
}

/**
 * Updates the scene during an island warp.
 */
void SingleModeScene::updateIslandWarp() {
    mLiveActorKit->getCameraDirector_RS()->execute();
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "Clipping");
    al::updateKitListPaused(this, "ModelUpdate");
    al::updateKitList(this, "デモプレイヤーロケーター");
    al::updateKitList(this, "デモプレイヤー前処理");
    al::updateKitList(this, "プレイヤー[Movement]");
    al::updateKitList(this, "プレイヤー");
    al::updateKitList(this, "プレイヤー装飾");
    al::updateKitList(this, "プレイヤー装飾２");
    al::updateKitList(this, "デモ");
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "空");
    al::updateKitList(this, "デモオブジェクト");
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffect(this);
    mLiveActorKit->updateGraphics(false);
    mLiveActorKit->getExecuteDirector()->finishExecute();
    mLiveActorKit->getClippingDirector()->executeRequestAsyncUpdate();
    mLiveActorKit->updateReducedBufferEffect();
}

/**
 * Runs the placement steps that precede the object placement; the base scene has none.
 * @param rInfo Unused.
 */
void SingleModeScene::preInitPlacement(const al::ActorInitInfo& rInfo) {}

/**
 * Places the area objects of the stage.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initAreaObj(const al::ActorInitInfo& rInfo) {
    al::initPlacementAreaObj(this, rInfo, nullptr);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
}

/**
 * Places every object of the stage: ocean, zones, goal items, checkpoints, players and demos.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacement(al::ActorInitInfo& rInfo) {
    s32 mapNum = al::getStageInfoMapNum(this);
    preInitPlacement(rInfo);
    initPlacementOceanWater(rInfo);
    initPlacementZoneHolders(rInfo);
    initAreaObj(rInfo);
    initPlacementGoalItems(rInfo);
    mCatGullHolder = al::createSceneObj(this, SceneObjID_CatGullHolder);
    initPlacementIslandFlags(rInfo);
    initPlacementCheckpoint(rInfo);
    auto* selector = static_cast<PlayerRetargettingSelectorSceneObj*>(
        al::createSceneObj(this, SceneObjID_PlayerRetargettingSelector));
    initPlacementPlayer(al::getStageInfoMap(this, 0), rInfo, selector);
    PlayerStockerFunction::tryCreateDoubleMario(this, rInfo, selector, nullptr, false);
    initPlacementGoal(rInfo);

    ProjectActorFactory skyFactory;
    al::initPlacementByStageInfo(al::getStageInfoMap(this, 0), "SkyList", skyFactory, rInfo);

    for (s32 i = 0; i < mapNum; i++) {
        const al::StageInfo* stageInfo = al::getStageInfoMap(this, i);
        initPlacementObject(stageInfo, rInfo, "Map");
        initPlacementRaidonSurf(stageInfo, rInfo);
    }

    for (s32 i = 0; i < al::getStageInfoDesignNum(this); i++) {
        initPlacementObject(al::getStageInfoDesign(this, i), rInfo, "Design");
    }

    for (s32 i = 0; i < al::getStageInfoSoundNum(this); i++) {
        initPlacementObject(al::getStageInfoSound(this, i), rInfo, "Sound");
    }

    initPlacementDisasterModeBowser(rInfo);
    initPlacementIntroCameras(rInfo);
    initPlacementLuckyIsland(rInfo);

    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);
    al::PlacementInfo listInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&listInfo, &count, stageInfo, "DemoObjList");
    ProjectActorFactory factory;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, i);

        if (!isValidPlacement(placementInfo)) {
            continue;
        }

        const char* objectName = nullptr;
        al::getObjectName(&objectName, placementInfo);
        al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &placementInfo);
        DemoActorGroupUtil::initDemoActorGroup(actor, rInfo, placementInfo);

        if (al::isEqualString(objectName, "StageStartEventDemo") ||
            al::isEqualString(objectName, "StageStartEventCamera") ||
            al::isEqualString(objectName, "StageStartEventSound") ||
            al::isEqualString(objectName, "StageStartBindDemoKinopioBrigade") ||
            al::isEqualString(objectName, "StageStartBindDemoKinopioHouse") ||
            al::isEqualString(objectName, "StageStartBindDemoCasinoRoom") ||
            al::isEqualString(objectName, "StageStartBindDemoMysteryHouse")) {
            if (mStartEvent == nullptr) {
                mStartEvent = static_cast<StageStartEventBase*>(actor);
            }
        }
    }

    mIslandWarpState = new IslandWarpState(this);
    mIslandWarpState->init(rInfo);
}

/**
 * Places the goal items (Cat Shines) and assigns them the island of their zone.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementGoalItems(const al::ActorInitInfo& rInfo) {
    ProjectActorFactory factory;
    mGoalItemHolder = al::createSceneObj(this, SceneObjID_GoalItemHolder);
    static_cast<GoalItemHolder*>(mGoalItemHolder)->setWindowProcessing(mWindowProcessing);
    al::LiveActor* goalItems[120] = {};
    al::tryInitPlacementCategory(this, rInfo, 0, "GoalItemList", factory, goalItems, 120);

    for (s32 i = 0; i < 120; i++) {
        auto* goalItem = static_cast<GoalItem*>(goalItems[i]);

        if (goalItem == nullptr) {
            break;
        }

        if (goalItem->getIslandId() == -5) {
            goalItem->setIslandId(goalItem->mPlacementHolder->getZoneNo());
        }
    }
}

/**
 * Places the flags of the islands.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementIslandFlags(const al::ActorInitInfo& rInfo) {
    ProjectActorFactory factory;
    al::PlacementInfo placementInfo;
    al::LiveActor* islandFlags[32];
    al::tryInitPlacementCategory(this, rInfo, 0, "IslandFlagList", factory, islandFlags, 32);
}

/**
 * Places the checkpoint flags and sorts them by zone.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementCheckpoint(const al::ActorInitInfo& rInfo) {
    ProjectActorFactory factory;
    al::PlacementInfo placementInfo;
    s32 mapNum = al::getStageInfoMapNum(this);

    if (mapNum < 1) {
        return;
    }

    mCheckpointLists.allocBuffer(mapNum, nullptr);

    for (s32 i = 0; i < mapNum; i++) {
        mCheckpointLists.pushBack(new CheckpointArray());
        mCheckpointLists[i]->allocBuffer(10, nullptr);
    }

    al::LiveActor* checkpoints[100] = {};
    al::tryInitPlacementCategory(this, rInfo, 0, "CheckPointList", factory, checkpoints, 100);

    for (s32 i = 0; i < 100; i++) {
        al::LiveActor* checkpoint = checkpoints[i];

        if (checkpoint == nullptr) {
            break;
        }

        if (!al::isEqualString(checkpoint->getName(), "SingleModeCheckpointFlag")) {
            continue;
        }

        s32 zoneNo = checkpoint->mPlacementHolder->getZoneNo();

        if (zoneNo > 0) {
            mCheckpointLists[zoneNo - 1]->pushBack(static_cast<SingleModeCheckpoint*>(checkpoint));
        } else {
            doZoneIDCheck();
        }
    }
}

/**
 * Places the players, Bowser Jr. and the start demo of the first phase at the restart point.
 * @param pStageInfo The main stage.
 * @param rInfo The actor init info.
 * @param pSelector The retargetting selector of the players.
 */
void SingleModeScene::initPlacementPlayer(const al::StageInfo* pStageInfo,
                                          const al::ActorInitInfo& rInfo,
                                          PlayerRetargettingSelector* pSelector) {
    al::PlacementInfo listInfo;
    s32 count = 0;
    al::tryGetPlacementInfoAndCount(&listInfo, &count, pStageInfo, "PlayerList");
    al::PlacementInfo playerInfo;

    for (s32 i = 0; i < count; i++) {
        al::getPlacementInfoByIndex(&playerInfo, listInfo, i);

        if (isValidPlacement(playerInfo)) {
            break;
        }
    }

    al::getSceneObj<PlayerGroupSceneObj>(this, SceneObjID_PlayerGroup)->initGroup(128);
    al::setSceneObj(this, new PlayerProcess(mLiveActorKit->getPlayerHolder()),
                    SceneObjID_PlayerProcess);

    al::ActorInitInfo actorInfo;
    bool isGigaBellRespawn = false;
    bool isGenericRespawn = false;

    if (allowRestartPoint() && mUnlockedPhase != 0 && !mIsLastBowserBattle) {
        s32 zoneIndex;
        s32 checkpointIndex;
        SingleModeDataFunction::getLastCheckpointPass(this, &zoneIndex, &checkpointIndex);

        if ((zoneIndex | checkpointIndex) >= 0 && zoneIndex < mCheckpointLists.size()) {
            CheckpointArray* checkpoints = mCheckpointLists.unsafeAt(zoneIndex);

            for (s32 i = 0; i < checkpoints->size(); i++) {
                SingleModeCheckpoint* checkpoint = checkpoints->at(i);

                if (checkpoint->getCheckpointId() == checkpointIndex + 1) {
                    actorInfo = *checkpoint->getPlayerInfo();
                    checkpoint->setStateAfter();
                    goto placePlayers;
                }
            }

            actorInfo.initNoViewId(&playerInfo, rInfo);
        } else if (SingleModeDataFunction::isGigaBellPlayerRespawnPointValid(this)) {
            actorInfo.initNoViewId(&playerInfo, rInfo);
            isGigaBellRespawn = true;
        } else {
            bool isGenericValid = SingleModeDataFunction::isGenericRespawnPlayerPositionValid(this);
            actorInfo.initNoViewId(&playerInfo, rInfo);

            if (isGenericValid) {
                isGenericRespawn = true;
            } else {
                s32 islandId = -1;

                if (SingleModeDataFunction::getLastIslandCheckpointPass(this, &islandId)) {
                    const al::ActorInitInfo* startInfo =
                        mIslandKeeper->getIslandStartPos(islandId);

                    if (startInfo != nullptr) {
                        actorInfo = *startInfo;
                    }
                } else if (SingleModeDataFunction::isPhase4BossDefeated(this) &&
                           mUnlockedPhase == 8) {
                    al::PlacementInfo linkInfo;

                    if (al::tryGetLinksInfo(&linkInfo, playerInfo, "Phase4v2Pos")) {
                        al::PlacementInfo phase4Info;
                        phase4Info.set(linkInfo.getPlacementIter(), linkInfo.getZoneIter(),
                                       playerInfo._20, linkInfo._28);
                        actorInfo.initNoViewId(&phase4Info, rInfo);
                    }
                }
            }
        }
    } else {
        actorInfo.initNoViewId(&playerInfo, rInfo);

        if (!mIsLastBowserBattle) {
            isGigaBellRespawn = SingleModeDataFunction::isGigaBellPlayerRespawnPointValid(this);
        }
    }

placePlayers:
    s32 cutscenePosNum = al::calcLinkChildNum(actorInfo, "CutscenePos");

    if (!isGigaBellRespawn && cutscenePosNum != 0) {
        al::getLinksMatrix(&mDemoBaseMtx, actorInfo, "CutscenePos");
    } else {
        al::tryGetMatrixTR(&mDemoBaseMtx, actorInfo);
    }

    mIsNarrowPlace = false;
    al::tryGetArg(&mIsNarrowPlace, actorInfo, "IsNarrowPlace");
    mStartingFigure = -1;
    al::tryGetArg(&mStartingFigure, actorInfo, "StartingFigure");

    PlayerKoopaJr* koopaJr;
    {
        al::ActorInitInfo linkInfo;
        linkInfo.initNoViewId(&playerInfo, rInfo);

        if (al::calcLinkChildNum(playerInfo, "KoopaJR") >= 1) {
            koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

            if (koopaJr == nullptr) {
                koopaJr = new PlayerKoopaJr("KoopaJr");
                al::initLinksActor(koopaJr, linkInfo, "KoopaJR", 0);
                al::setSceneObj(this, koopaJr, SceneObjID_PlayerKoopaJr);
            }
        } else {
            koopaJr = nullptr;
        }
    }

    al::LiveActor* dirtPile = nullptr;
    {
        al::ActorInitInfo linkInfo;
        linkInfo.initNoViewId(&playerInfo, rInfo);

        if (al::calcLinkChildNum(playerInfo, "Phase0DirtPile") >= 1) {
            dirtPile = new al::FixMapParts("Phase0DirtPile");
            al::initLinksActor(dirtPile, linkInfo, "Phase0DirtPile", 0);
        }
    }

    s32 characterNum = rc::getPlayerCharacterNumMax();
    s32 playerNum = isIslandScene() ? characterNum : 1;

    if (mUnlockedPhase == 0 &&
        SingleModeDataFunction::isFirstPhase0(GameDataHolderAccessor(mGameDataHolder))) {
        mIsPhase0Start = true;
        SingleModeDataFunction::setIsFirstPhase0(GameDataHolderAccessor(mGameDataHolder));
    }

    for (s32 i = 0; i < playerNum; i++) {
        const sead::Matrix34f* viewMtx = &mLiveActorKit->getCameraDirector_RS()->mViewMtx;
        const char* archiveName = isBossScene() ? "SMBoss" : nullptr;
        const char* characterName = rc::getPlayerCharacterName(i);
        auto* player = new PlayerActor(viewMtx);
        PlayerActionGraphBuilder builder(false);
        player->initSpecial(actorInfo, 0, characterName, pSelector, pSelector, &builder, "プレイヤー",
                            31, 0, archiveName);
        rc::deactivatePlayer(player);

        if (isGigaBellRespawn) {
            sead::Vector3f trans;
            sead::Vector3f front;
            SingleModeDataFunction::tryGetGigaBellPlayerRespawnPoint(
                GameDataHolderAccessor(mGameDataHolder), &trans, &front);
            rc::setPlayerTrans(player, trans);
            rc::setPlayerFrontVec(player, front);
            mDemoBaseMtx.setTranslation(trans);
        } else if (isGenericRespawn) {
            sead::Vector3f trans;
            sead::Vector3f front;
            SingleModeDataFunction::tryGetGenericPlayerRespawnPosition(
                GameDataHolderAccessor(mGameDataHolder), &trans, &front);
            rc::setPlayerTrans(player, trans);
            rc::setPlayerFrontVec(player, front);
        }

        player->setSingleModeInput(true);
        alPlayerFunction::registerPlayer(player, player->getPadRumbleKeeper(), true);
        PlayerStockerFunction::registerPlacementPlayer(this, player, i);
        mPlayers.pushBack(player);
        player->setInvincibleBgmController(mInvincibleBgmController);
        player->setBigBgmController(mBigBgmController);
        player->setKoopaJr(koopaJr);
    }

    if (koopaJr != nullptr) {
        alPlayerFunction::registerPlayer(koopaJr, koopaJr->getPadRumbleKeeper(), false);
    }

    mPlayerCrown = new PlayerCrown(actorInfo, nullptr);
    PlayerEntryFunction::retirePlayer(GameDataHolderAccessor(mGameDataHolder), 1);
    PlayerEntryFunction::retirePlayer(GameDataHolderAccessor(mGameDataHolder), 2);
    PlayerEntryFunction::retirePlayer(GameDataHolderAccessor(mGameDataHolder), 3);

    if (mIsPhase0Start) {
        auto* phase0StartDemo = new StageStartPhase0Demo("Phase0Start", dirtPile);
        mPhase0StartDemo = phase0StartDemo;
        phase0StartDemo->init(rInfo);

        if (SingleModeDataFunction::isDemoWasCancelled(GameDataHolderAccessor(mGameDataHolder))) {
            phase0StartDemo->setSkipped();
            SingleModeDataFunction::setDemoWasCancelled(GameDataHolderAccessor(mGameDataHolder),
                                                        false);
        }
    }
}

/**
 * Places the goals (goal doors and poles) of every map stage.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementGoal(const al::ActorInitInfo& rInfo) {
    for (s32 i = 0; i < al::getStageInfoMapNum(this); i++) {
        al::PlacementInfo listInfo;
        s32 count = 0;
        al::getPlacementInfoAndCount(&listInfo, &count, al::getStageInfoMap(this, i), "GoalList");
        ProjectActorFactory factory;

        for (s32 j = 0; j < count; j++) {
            al::PlacementInfo placementInfo;
            al::getPlacementInfoByIndex(&placementInfo, listInfo, j);
            al::LiveActor* actor =
                al::createPlacementActorFromFactory(factory, rInfo, &placementInfo);

            if (actor == nullptr) {
                continue;
            }

            const char* objectName = nullptr;
            al::getObjectName(&objectName, placementInfo);

            if (al::isEqualString(objectName, "GoalDoor") ||
                al::isEqualString(objectName, "GoalPole") ||
                al::isEqualString(objectName, "GoalPoleSuper") ||
                al::isEqualString(objectName, "GoalPoleRunaway") ||
                al::isEqualString(objectName, "GoalPoleLast")) {
                continue;
            }
        }
    }
}

namespace {

/// Placement of a zone of the main stage, used to locate the island holders.
struct ZoneInfo {
    const char* mName;
    sead::Vector3f mTrans;
    s32 mZoneId;
};

}  // namespace

/**
 * Creates the island holders, from the island data when available or else from the zone list.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementZoneHolders(const al::ActorInitInfo& rInfo) {
    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);

    if (mIslandKeeper->createIslandHolders(stageInfo, rInfo)) {
        return;
    }

    ProjectActorFactory factory;
    al::PlacementInfo listInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&listInfo, &count, stageInfo, "ZoneList");
    al::LiveActor* holders[32] = {};
    count++;
    al::tryInitPlacementCategory(this, rInfo, 0, "ZoneHolderList", factory, holders, count);

    sead::FixedObjArray<ZoneInfo, 32> zones;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::tryGetPlacementInfoByIndex(&placementInfo, listInfo, i);
        const char* objectName = nullptr;

        if (!al::tryGetObjectName(&objectName, placementInfo)) {
            continue;
        }

        sead::Vector3f rotate = sead::Vector3f::ones;
        sead::Vector3f scale = sead::Vector3f::zero;
        sead::Vector3f trans = sead::Vector3f::zero;
        al::tryGetRotate(&rotate, placementInfo);
        al::tryGetScale(&scale, placementInfo);
        al::tryGetTrans(&trans, placementInfo);
        s32 zoneId = 0;
        placementInfo.getZoneIter().tryGetIntByKey(&zoneId, "ZoneID");
        placementInfo.getPlacementIter().tryGetIntByKey(&zoneId, "ZoneID");

        zones.birthInsert(i);
        zones.unsafeAt(i)->mName = objectName;
        zones.at(i)->mTrans = trans;
        zones.unsafeAt(i)->mZoneId = zoneId;
    }

    for (s32 i = 0; i < count; i++) {
        auto* holder = static_cast<IslandHolder*>(holders[i]);

        if (holder == nullptr) {
            break;
        }

        for (s32 j = 0; j < count; j++) {
            if (al::isEqualString(zones.unsafeAt(j)->mName, holder->getObjectName())) {
                holder->setAppearEffectPos(zones.unsafeAt(j)->mTrans);
                holder->setIslandId(zones.unsafeAt(j)->mZoneId);
                mIslandKeeper->push(holder);
                break;
            }
        }
    }

    mIslandKeeper->setIslandStartPos(mStageResourceKeeper->getMapStageInfo(), rInfo);
}

/**
 * Places the ocean of the main stage.
 * @param rInfo The actor init info, which receives the ocean.
 */
void SingleModeScene::initPlacementOceanWater(al::ActorInitInfo& rInfo) {
    initPlacementOceanWater(al::getStageInfoMap(this, 0), rInfo);
}

/**
 * Places the ocean of a stage, initializes it from the stage's ocean settings and registers it.
 * @param pStageInfo The stage holding the ocean.
 * @param rInfo The actor init info, which receives the ocean.
 */
void SingleModeScene::initPlacementOceanWater(const al::StageInfo* pStageInfo,
                                              al::ActorInitInfo& rInfo) {
    al::PlacementInfo listInfo;
    s32 count = 0;

    if (!al::tryGetPlacementInfoAndCount(&listInfo, &count, pStageInfo, "OceanList") ||
        count < 1) {
        return;
    }

    ProjectActorFactory factory;
    al::PlacementInfo placementInfo;
    al::getPlacementInfoByIndex(&placementInfo, listInfo, 0);
    al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &placementInfo);

    if (actor == nullptr) {
        return;
    }

    auto* ocean = static_cast<al::OceanWaveDirector*>(actor);
    al::ViewRenderer* viewRenderer = mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer();
    mOceanWaveDirector = ocean;
    const u8* byml = pStageInfo->getResource()->tryGetByml("Ocean");

    if (byml != nullptr) {
        ocean->initFromYaml(al::ByamlIter(byml), mStageName.cstr());
    } else {
        ocean->setStageName(mStageName.cstr());
    }

    rInfo.mOceanWaveDirector = ocean;
    al::setSceneObj(this, ocean, SceneObjID_OceanWater);

    if (ocean->getRenderType() == 1) {
        viewRenderer->enableSSR();
    }
}

/**
 * Creates Fury Bowser and registers the points he can spawn from.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementDisasterModeBowser(const al::ActorInitInfo& rInfo) {
    s32 mapNum = al::getStageInfoMapNum(this);
    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);
    ProjectActorFactory factory;
    al::PlacementInfo listInfo;
    s32 count = 0;

    if (!al::tryGetPlacementInfoAndCount(&listInfo, &count, stageInfo, "BowserSpawnList")) {
        return;
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (controller == nullptr) {
        return;
    }

    SuperBowser* bowser = nullptr;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, i);

        if (!isValidPlacement(placementInfo)) {
            continue;
        }

        const char* className = nullptr;
        al::getClassName(&className, placementInfo);

        if (al::isEqualString(className, "DisasterModeBowser")) {
            al::ActorInitInfo actorInfo;
            actorInfo.initViewIdSelf(&placementInfo, rInfo);
            bowser = controller->tryCreateSuperBowser(actorInfo);
            bowser->init(actorInfo);
            break;
        }
    }

    for (s32 i = 0; i < mapNum; i++) {
        const al::StageInfo* mapStageInfo = al::getStageInfoMap(this, i);

        if (!isValidPlacementParent(mapStageInfo->getPlacementInfo())) {
            continue;
        }

        al::PlacementInfo spawnListInfo;
        s32 spawnNum = 0;

        if (!al::tryGetPlacementInfoAndCount(&spawnListInfo, &spawnNum, mapStageInfo,
                                             "BowserSpawnList")) {
            continue;
        }

        for (s32 j = 0; j < spawnNum; j++) {
            al::PlacementInfo placementInfo;
            al::getPlacementInfoByIndex(&placementInfo, spawnListInfo, j);

            if (!isValidPlacement(placementInfo)) {
                continue;
            }

            const char* className = nullptr;
            al::getClassName(&className, placementInfo);

            if (al::isEqualString(className, "DisasterModeBowser")) {
                continue;
            }

            al::ActorInitInfo actorInfo;
            actorInfo.initViewIdSelf(&placementInfo, rInfo);
            s32 zoneId = -1;
            al::tryGetZoneID(&zoneId, actorInfo.getPlacementInfo());

            if (bowser != nullptr) {
                bowser->addSpawnPoint(actorInfo);
            }
        }
    }
}

/**
 * Places the lucky islands (none in this scene).
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementLuckyIsland(const al::ActorInitInfo& rInfo) {}

/**
 * Creates Plessie from the first spawn point of a stage and registers the others.
 * @param pStageInfo The stage to place from.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementRaidonSurf(const al::StageInfo* pStageInfo,
                                              const al::ActorInitInfo& rInfo) {
    if (!isValidPlacementParent(pStageInfo->getPlacementInfo())) {
        return;
    }

    al::PlacementInfo listInfo;
    s32 count = 0;

    if (!al::tryGetPlacementInfoAndCount(&listInfo, &count, pStageInfo, "RaidonSpawnList")) {
        return;
    }

    ProjectActorFactory factory;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, i);

        if (!isValidPlacement(placementInfo)) {
            continue;
        }

        if (mRaidonSurf == nullptr) {
            mRaidonSurf = static_cast<RaidonSurf*>(
                al::createPlacementActorFromFactory(factory, rInfo, &placementInfo));
        } else {
            al::ActorInitInfo actorInfo;
            actorInfo.initViewIdSelf(&placementInfo, rInfo);
            mRaidonSurf->addSpawnPoint(actorInfo);
        }
    }
}

/**
 * Creates the fly-over camera of the island intros.
 * @param rInfo The actor init info.
 */
void SingleModeScene::initPlacementIntroCameras(const al::ActorInitInfo& rInfo) {
    al::getStageInfoMapNum(this);
    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);
    ProjectActorFactory factory;
    al::PlacementInfo listInfo;
    s32 count = 0;

    if (!al::tryGetPlacementInfoAndCount(&listInfo, &count, stageInfo, "IntroCameraList")) {
        return;
    }

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, i);

        if (!isValidPlacement(placementInfo)) {
            continue;
        }

        const char* className = nullptr;
        al::getClassName(&className, placementInfo);

        if (al::isEqualString(className, "IntroFlyOverCamera")) {
            mIntroFlyOverCamera = static_cast<IntroFlyOverCamera*>(
                al::createPlacementActorFromFactory(factory, rInfo, &placementInfo));
            return;
        }
    }
}

/**
 * Places the objects of a stage.
 * @param pStageInfo The stage to place from.
 * @param rInfo The actor init info.
 * @param pListName The kind of the stage (unused).
 */
void SingleModeScene::initPlacementObject(const al::StageInfo* pStageInfo,
                                          const al::ActorInitInfo& rInfo,
                                          const char* pListName) {
    if (!isValidPlacementParent(pStageInfo->getPlacementInfo())) {
        return;
    }

    al::PlacementInfo listInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&listInfo, &count, pStageInfo, "ObjectList");
    ProjectActorFactory factory;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, i);

        if (!isValidPlacement(placementInfo)) {
            continue;
        }

        const char* objectName = nullptr;
        al::getObjectName(&objectName, placementInfo);

        if (al::isEqualString(objectName, "ProfilingWarpPoint")) {
            continue;
        }

        al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &placementInfo);

        if (actor != nullptr) {
            DemoActorGroupUtil::initDemoActorGroup(actor, rInfo, placementInfo);
        }
    }
}

/**
 * Kills every living player and ends the game with the time up.
 */
void SingleModeScene::forceKillPlayerAll() {
    al::PlayerHolder* playerHolder = mLiveActorKit->getPlayerHolder();

    for (s32 i = 0; i < al::getPlayerNumMax(playerHolder); i++) {
        al::LiveActor* player = al::getPlayerActor(playerHolder, i);

        if (!rc::isPlayerDead(player)) {
            rc::forceKillPlayer(player);
        }
    }

    al::setNerve(this, &NrvSingleModeScenePreTimeUp);
}

/**
 * Updates the scene while the players are frozen: everything but the players' movement keeps
 * running.
 * @param isFreeze Unused.
 */
void SingleModeScene::updateFreezeMode(bool isFreeze) {
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "OceanWaterRequest");
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "ライト管理");
    al::updateKitList(this, "Clipping");
    al::updateKitListPaused(this, "ModelUpdate");
    al::updateKitList(this, "デモ");
    al::updateKitList(this, "空");

    if (!mGameDataHolder->isUnknown62()) {
        al::updateKitList(this, "プレイヤー");
        al::updateKitList(this, "プレイヤー装飾");
        al::updateKitList(this, "プレイヤー装飾２");
    }

    al::updateKitList(this, "デモオブジェクト");
    al::updateKitList(this, "地形オブジェ");
    al::updateKitList(this, "地形オブジェ[Movement]");
    al::updateKitList(this, "コリジョン地形");
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "エフェクト（前処理）");
    al::updateKitList(this, "エフェクト（３Ｄ）");
    al::updateKitList(this, "エフェクト（プレイヤー）");
    al::updateKitList(this, "Effect (HitStop)");
    al::updateKitList(this, "エフェクト（カメラデモ）");
    al::updateKitList(this, "エフェクト（ベース２Ｄ）");
    al::updateKitList(this, "エフェクト（２Ｄ）");
    al::updateKitList(this, "エフェクト（後処理）");
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateKitList(this, "コインローテータ");
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "2D StopScene");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateKitList(this, "Water");
    mLiveActorKit->updateReducedBufferEffect();
    mLiveActorKit->getExecuteDirector()->finishExecute();
    mLiveActorKit->updateGraphics(false);
}

/**
 * Updates the actors that keep moving (paused) during a cutscene.
 */
void SingleModeScene::updateDemoCutsceneAddOn() {
    al::updateKitListPaused(this, "コリジョン地形");
    al::updateKitListPaused(this, "コリジョン地形[Movement]");
    al::updateKitListPaused(this, "コリジョン地形装飾");
    al::updateKitListPaused(this, "ＮＰＣ");

    if (!getDemoDirector()->isUnknownE1()) {
        al::updateKitListPaused(this, "アイテム");
    }

    if (getDemoDirector()->isUnknownE2()) {
        al::updateKitListPaused(this, "監視オブジェ");
    }
}

/**
 * Gets the name of the 2D draw kit of the sub screen.
 * @return The kit name.
 */
const char* SingleModeScene::getDraw2DKitSubName() const {
    return "２Ｄベース（サブ画面）";
}

/**
 * Checks whether the stage has no start event.
 * @return True when there is no start event.
 */
bool SingleModeScene::isNotExistStartDemo() const {
    return mStartEvent == nullptr;
}

/**
 * Checks the zone id of the placed checkpoints (always valid).
 * @return True.
 */
bool SingleModeScene::doZoneIDCheck() const {
    return true;
}

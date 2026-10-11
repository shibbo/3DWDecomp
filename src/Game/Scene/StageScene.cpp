#include "Scene/StageScene.hpp"

#include <nn/oe.h>
#include <prim/seadBitFlag.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Bgm/MicInputSePlayer.hpp"
#include "Bgm/PlayerBigBgmController.hpp"
#include "Bgm/PlayerInvincibleBgmController.hpp"
#include "Camera/CameraObserver.hpp"
#include "Demo/DemoKoopaW7.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Demo/StageStartEventTimer.hpp"
#include "Layout/CameraChangeLayout.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/PauseMenu.hpp"
#include "Layout/PlayerEntryMini.hpp"
#include "Layout/ResultTimerCount.hpp"
#include "Layout/StageSceneLayout.hpp"
#include "Layout/Switch/SnapshotLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/AudioDirector.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Base/HashCodeUtil.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Debug/CommandOption.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/System/SystemKit.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/SceneUtil.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Draw/GraphicsInitArg.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Nfp/NfpDirector.hpp"
#include "Library/Obj/FootPrintServer.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWait.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "MapObj/BlockAssistFunction.hpp"
#include "MapObj/BlockAssistWatcher.hpp"
#include "MapObj/BlockChoiceWatcher.hpp"
#include "MapObj/CheckpointFlag.hpp"
#include "MapObj/ChikaChikaBlockSynchronizer.hpp"
#include "MapObj/CoinRotater.hpp"
#include "MapObj/DokanWorldWarp.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcAssistDirectorList.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "MapObj/FairyHouseIllustItemWatcher.hpp"
#include "MapObj/GoalObjHolder.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/IllustItemKeeper.hpp"
#include "MapObj/GoalPole.hpp"
#include "MapObj/MysteryHouseChecker.hpp"
#include "MapObj/PlayerCrown.hpp"
#include "MapObj/ScoreHolder.hpp"
#include "MapObj/StampDirector.hpp"
#include "MapObj/SuperbViewArea.hpp"
#include "NPC/GhostPlayerDirector.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "Player/FurEnv.hpp"
#include "Player/Giga/PlayerActionGraphBuilder.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerGroupSceneObj.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "Player/PlayerActionGraphBuilderKinopioBrigade.hpp"
#include "Player/PlayerCooperation.hpp"
#include "Player/PlayerFireBallAppearWatcher.hpp"
#include "Player/PlayerProcess.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/PlayerStocker.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneEventMessageSender.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "Scene/SnapshotState.hpp"
#include "Scene/StageSceneStateGameOver.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Application.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "System/Data/StageListHolder.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/PlayLogFunction.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "System/StageTimer.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"

/**
 * Declares a StageScene nerve whose execute function has a different name than the nerve.
 * @param Action The nerve name.
 * @param Func The StageScene::exe* function the nerve runs.
 */
#define STAGE_SCENE_NERVE(Action, Func)                                                            \
    class StageSceneNrv##Action : public al::Nerve {                                               \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<StageScene>()->exe##Func();                                         \
        }                                                                                          \
    };

/**
 * Defines a StageScene nerve object.
 * @note Unlike NERVES_MAKE_NOSTRUCT, the nerves are not const: the compiler merges them into one
 *       object and addresses them from a common base, as in the game.
 * @param Class The scene class.
 * @param Action The nerve name.
 */
#define STAGE_SCENE_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action{};

/**
 * Defines the StageScene nerve objects, in the order of the game's data.
 * @param ... The nerve names.
 */
#define STAGE_SCENE_NERVES_MAKE(...) FOR_EACH(STAGE_SCENE_NERVE_MAKE, StageScene, __VA_ARGS__)

namespace {
NERVE_DECL(StageScene, CaptureMode)
NERVE_DECL(StageScene, StartBindDemo)
NERVE_DECL(StageScene, StartEvent)
NERVE_DECL(StageScene, Start)
NERVE_DECL(StageScene, GameOver)
NERVE_DECL(StageScene, Pause)
NERVE_DECL(StageScene, IllustItemResult)
STAGE_SCENE_NERVE(LoadGame, GameEnd)
NERVE_DECL(StageScene, Retire)
NERVE_DECL(StageScene, Goal)
NERVE_DECL(StageScene, Restart)
STAGE_SCENE_NERVE(RestartMiss, Restart)
NERVE_DECL(StageScene, ReenterStage)
STAGE_SCENE_NERVE(DeadGoldenExpress, Restart)
STAGE_SCENE_NERVE(RestartMysteryBox, Restart)
STAGE_SCENE_NERVE(TimeUpMysteryBox, MysteryBoxTimeUp)
NERVE_DECL(StageScene, GameEnd)
STAGE_SCENE_NERVE(GameChange, GameEnd)
NERVE_DECL(StageScene, WorldWarp)
NERVE_DECL(StageScene, Play)
NERVE_DECL(StageScene, GoalDemo)
NERVE_DECL(StageScene, DemoChangePlayer)
NERVE_DECL(StageScene, DemoCamera)
NERVE_DECL(StageScene, PreGoalDemo)
NERVE_DECL(StageScene, DemoScene)
NERVE_DECL(StageScene, PauseWindowMessage)
NERVE_DECL(StageScene, ResultTimerCount)
NERVE_DECL(StageScene, KoopaDemoW7)
NERVE_DECL(StageScene, PreTimeUp)
STAGE_SCENE_NERVES_MAKE(CaptureMode, StartBindDemo, StartEvent, Start, GameOver, Pause,
                        IllustItemResult, LoadGame, Retire, Goal, Restart, RestartMiss,
                        ReenterStage, DeadGoldenExpress, RestartMysteryBox, TimeUpMysteryBox,
                        GameEnd, GameChange, WorldWarp, Play, GoalDemo, DemoChangePlayer,
                        DemoCamera, PreGoalDemo, DemoScene, PauseWindowMessage, ResultTimerCount,
                        KoopaDemoW7, PreTimeUp)

/** Event type of the start events binding the players (StageStartBindDemo*). */
constexpr s32 cStartEventTypeBindDemo = 3;
/** Event type of the start event showing the stage timer. */
constexpr s32 cStartEventTypeTimer = 4;
/** Audio demo type of the demos played in the stage. */
constexpr alSeFunction::DemoType cAudioDemoType = static_cast<alSeFunction::DemoType>(0);
/** Player type of the players of a stage. */
constexpr u32 cPlayerType = 31;
/** Player type of the players of a stage when Double Mario (Double Cherry) can be used. */
constexpr u32 cPlayerTypeDoubleMario = 19;
/** Number of player entry items shown in the Captain Toad stages. */
constexpr s32 cPlayerEntryItemNumKinopioBrigade = 3;
/** Distance between two players placed side by side at the start of the stage. */
constexpr f32 cPlayerPlacementInterval = 150.0f;
/** Side offset of the players placed in a narrow place. */
constexpr f32 cPlayerNarrowPlacementOffset = 75.0f;

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
 * Gets the type of a stage start event.
 * @param pEvent The start event.
 * @return The event type.
 */
s32 getEventType(const StageStartEventBase* pEvent) {
    return pEvent->getEventType();
}

/**
 * Gets whether a course plays a sequence BGM that has to be stopped when the stage ends.
 * @param pHolder The game data holder.
 * @param courseId The course id.
 * @return true for the Toad houses, the fairy houses and the mystery box courses.
 */
bool isStageSequenceBgm(GameDataHolder* pHolder, s32 courseId) {
    if (courseId < 0) {
        return false;
    }

    GameDataHolderAccessor accessor(pHolder);
    return GameDataFunction::isStageKinopioHouse(accessor, courseId) ||
           GameDataFunction::isStageKinopioHouseHide(accessor, courseId) ||
           GameDataFunction::isStageFairyHouse(accessor, courseId) ||
           GameDataFunction::isStageContinuousMysteryBox(accessor, courseId);
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
 * Creates a player of a character.
 * @param rInfo The actor init info of the player placement.
 * @param pViewMtx The view matrix of the camera following the player.
 * @param pCharacterName The name of the player's character.
 * @param pSelector The retargetting selector of the players.
 * @param playerType The type of the player.
 * @return The created player.
 */
PlayerActor* createPlayer(const al::ActorInitInfo& rInfo, const sead::Matrix34f* pViewMtx,
                          const char* pCharacterName, PlayerRetargettingSelector* pSelector,
                          u32 playerType) {
    auto* player = new PlayerActor(pViewMtx);
    PlayerActionGraphBuilder builder(false);
    player->initSpecial(rInfo, 0, pCharacterName, pSelector, pSelector, &builder, "プレイヤー",
                        playerType, 0, nullptr);
    return player;
}

/**
 * Gets whether a pad is connected (also true when there is no pad port to check).
 * @return true if a pad is connected.
 */
bool isPadConnectedAny() {
    bool isConnected = true;

    for (s32 i = 0; i <= al::getMaxControllerPorts(); i++) {
        isConnected = al::isPadConnected(i);

        if (isConnected) {
            break;
        }
    }

    return isConnected;
}

/**
 * Gets the goal interface of a goal actor.
 * @param pActor The goal actor; every goal actor derives from al::LiveActor, then IGoalObj.
 * @return The goal interface.
 */
const IGoalObj* getGoalObj(al::LiveActor* pActor) {
    return static_cast<GoalPole*>(pActor);
}
}  // namespace

/**
 * Constructs the stage scene.
 * @param pStageWipeKeeper The wipes shared with the other scenes.
 */
StageScene::StageScene(StageWipeKeeper* pStageWipeKeeper)
    : InGameSceneBase("ステージシーン"), mStageWipeKeeper(pStageWipeKeeper) {
    mMainViewport.set(0.0f, 0.0f, 1280.0f, 720.0f);
    mSubViewport.set(0.0f, 0.0f, 854.0f, 480.0f);
    mStereoMainViewport.set(0.0f, 0.0f, 1280.0f, 720.0f);
    mStereoSubViewport.set(640.0f, 0.0f, 1280.0f, 720.0f);
}

/**
 * Destroys the stage scene and restores the settings changed by the stage.
 */
StageScene::~StageScene() {
    if (al::isNerve(this, &NrvStageSceneCaptureMode)) {
        mSnapshotState->kill();
    }

    nn::oe::SetAlbumImageOrientation(nn::album::ImageOrientation_None);
    rc::setMainPlayerActor(nullptr);
    mLiveActorKit->getEffectSystem()->endScene();
    getFrameworkAlive()->mIsClearRenderBuffer = true;

    if (mStampDirector != nullptr) {
        delete mStampDirector;
        mStampDirector = nullptr;
    }

    if (mNfpDirector != nullptr) {
        mNfpDirector->stop(true);
    }
}

/**
 * Initializes the scene: stage resources, kits, scene objects, layouts and placement.
 * @param rInfo The scene init info.
 */
void StageScene::init(const al::SceneInitInfo& rInfo) {
    const char* sceneName = rInfo.mSceneName.cstr();
    al::tryGetIntOptionValue(&mWorldId, sceneName, "WorldId");
    al::tryGetIntOptionValue(&mStageId, sceneName, "StageId");
    mStageName = rInfo.mStageName;
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mGameDataHolder->setSingleMode(false);
    rInfo.mGameSystemInfo->getGamePadSystem()->setMaxNpadNum(4);

    u32 seed = al::calcHashCode(mStageName.cstr());
    al::initRandomSeed(seed);
    al::initRandomSeedNonSync(seed);
    initAndLoadStageResource(mStageName.cstr(), 1);
    al::tryRequestPreLoadFile(this, rInfo, 1, nullptr);
    GameDataFunction::updateClearStarLevel(GameDataHolderWriter(mGameDataHolder));
    mNetworkSystem = nullptr;
    mNfpDirector = rInfo.mGameSystemInfo->getNfpDirector();
    mCourseId = mGameDataHolder->getStageList()->calcCourseId(mWorldId, mStageId);
    initSceneStopCtrl();
    initScreenCoverCtrl();
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    mGameDataHolder->setSceneObjHolder(mSceneObjHolder);
    al::createSceneObj(this, SceneObjID_ScoreHolder);
    al::createSceneObj(this, SceneObjID_GreenStarKeeper);
    al::createSceneObj(this, SceneObjID_IllustItemKeeper);
    al::createSceneObj(this, SceneObjID_ControllerEventWatcher);
    auto* fireBallWatcher = static_cast<PlayerFireBallAppearWatcher*>(
        al::createSceneObj(this, SceneObjID_PlayerFireBallAppearWatcher));
    mGoalObjHolder = new GoalObjHolder(4);
    al::setSceneObj(this, new BlockAssistWatcher(mCourseId, mGameDataHolder),
                    SceneObjID_BlockAssistWatcher);
    mDrcAssistDirectorList = new DrcAssistDirectorList(4);
    al::setSceneObj(this, mDrcAssistDirectorList, SceneObjID_DrcAssistDirectorList);

    for (s32 i = 0; i < 4; i++) {
        mDrcAssistDirectorList->addTouchAssist(al::getPlayerControllerPort(i), false);
    }

    al::setSceneObj(this, mGameDataHolder, SceneObjID_GameDataHolder);
    al::getSceneObj<ScoreHolder>(this, SceneObjID_ScoreHolder)
        ->setStageDataHolder(mGameDataHolder->getStageDataHolderPtr());
    PlayerStockerFunction::createPlayerStocker(this, true);
    mMainViewport = *al::getDisplayViewport();
    mSubViewport = *al::getSubDisplayViewport();
    initSceneAudio(rInfo, mStageName.cstr(), 180, 180, 1, false, "Scene", 20, 1.0f);

    al::GraphicsInitArg graphicsArg;
    graphicsArg.mViewRendererCreator = new al::ViewRendererCreator();
    graphicsArg.mFar = 100.0f;
    graphicsArg.mIsStereo = false;
    graphicsArg.mIsUsingViewRenderer = true;
    graphicsArg.setViewNum(2);
    initLiveActorKitWithGraphics(graphicsArg, rInfo, 7198, 64, 2, 1310, false, false);
    mLiveActorKit->initHitSensorDirector(1, false);

    auto* messageSender = new SceneEventMessageSender();
    messageSender->setActorGroup(mLiveActorKit->getActorGroup());
    al::setSceneObj(this, messageSender, SceneObjID_SceneEventMessageSender);

    if (fireBallWatcher != nullptr) {
        fireBallWatcher->setGraphicsSystemInfo(mLiveActorKit->getGraphicsSystemInfo());
    }

    al::LiveActorKit* kit = mLiveActorKit;
    kit->setDemoDirector(new ProjectDemoDirector(kit->getPlayerHolder(), -1));
    al::setSceneObj(this, new PlayerCooperation(64), SceneObjID_PlayerCooperation);
    al::setSceneObj(this, new FurEnv(mLiveActorKit->getGraphicsSystemInfo()), SceneObjID_FurEnv);
    mLiveActorKit->getCameraDirector()->setStageName(mStageName.cstr());
    mLiveActorKit->getCameraDirector()->setCameraAspect(&mMainViewport, &mSubViewport);
    mIsKinopioBrigade = GameDataFunction::isStageKinopioBrigade(
        GameDataHolderAccessor(mGameDataHolder), mCourseId);
    mDrcAssistDirectorList->setKinopioBrigadeFlag(mIsKinopioBrigade);
    mDrcAssistDirectorList->setPlayerHolder(mLiveActorKit->getPlayerHolder());
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    sead::LookAtCamera* camera = cameraDirector->getLookAtCamera();
    initSceneAudio3D(rInfo, &camera->getPos(), &camera->getMatrix(),
                     cameraDirector->getProjection(), &camera->getAt(), "ターゲット寄り中間",
                     mLiveActorKit->getAreaObjDirector(), false);
    initAudioKeeper(nullptr);
    mAudioDirector->setPlayerHolder(mLiveActorKit->getPlayerHolder());
    mMicInputSePlayer = new MicInputSePlayer(mAudioDirector);
    mInvincibleBgmController = new PlayerInvincibleBgmController(mAudioDirector);
    mBigBgmController = new PlayerBigBgmController(mAudioDirector);
    mGhostPlayerDirector =
        new GhostPlayerDirector(this, rInfo, mLiveActorKit->getExecuteDirector(),
                                mLiveActorKit->getPlayerHolder(), mGameDataHolder);
    al::initItemDirector(this, new ProjectItemDirector(layoutInfo, mGameDataHolder,
                                                       mLiveActorKit->getPlayerHolder(),
                                                       mLiveActorKit->getAreaObjDirector()));

    mCameraObserver = new CameraObserver(mLiveActorKit->getPlayerHolder(),
                                         mLiveActorKit->getCameraDirector()->_108,
                                         &mLiveActorKit->getCameraDirector()->_120,
                                         &mLiveActorKit->getCameraDirector()->_158,
                                         mLiveActorKit->getCameraDirector()->_110,
                                         mLiveActorKit->getCameraDirector()->_118, layoutInfo);
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());

    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    al::setSceneObj(this, new CoinRotater(mLiveActorKit->getExecuteDirector()),
                    SceneObjID_CoinRotater);
    ScoreHolderUtil::initScoreHolder(this);
    auto* itemDirector = static_cast<ProjectItemDirector*>(mLiveActorKit->getItemDirector());
    itemDirector->createItemHolder(actorInfo, false);
    al::setSceneObj(this, new al::FootPrintServer(actorInfo, "FootPrint", 32),
                    SceneObjID_FootPrintServer);

    s32 stampCourseId = -1;

    if (mGameDataHolder->getStageDataHolder() != nullptr) {
        stampCourseId = mGameDataHolder->getStageDataHolder()->getCourseId();
    }

    mStampDirector = new rc::StampDirector(
        actorInfo, "Stamp",
        mDrcAssistDirectorList->getDrcAssist(al::getMainControllerPort())->getTouchAssistInfo(),
        30, stampCourseId);
    mDrcAssistDirectorList->setStampDirector(mStampDirector);
    al::setSceneObj(this, mStampDirector, SceneObjID_StampDirector);
    mLiveActorKit->getGraphicsSystemInfo()->initStageResource(
        al::tryGetStageResourceDesign(this, 0), mStageName.cstr(), mLiveActorKit, false, 0);
    initPlacement(actorInfo);
    initSceneAudioAfterInitPlacement(rInfo);
    mScreenCaptureExecutor = rInfo.mScreenCaptureExecutor;

    if (GameDataFunction::isCheckpointPass(this)) {
        mLiveActorKit->getCameraDirector()->_123 = true;
    }

    mLiveActorKit->getCameraDirector()->init(mLiveActorKit->getPlayerHolder());
    mLiveActorKit->getCameraDirector()->initAudioKeeper(actorInfo);

    if (mIsKinopioBrigade) {
        mLiveActorKit->getCameraDirector()->setKinopioBrigadeReverseHorizontal(
            GameDataFunction::getKinopioBrigadeCameraReverseHorizontal(
                GameDataHolderWriter(this)));
        mLiveActorKit->getCameraDirector()->setKinopioBrigadeReverseVertical(
            GameDataFunction::getKinopioBrigadeCameraReverseVertical(GameDataHolderWriter(this)));
    } else {
        mLiveActorKit->getCameraDirector()->setReverseHorizontal(
            GameDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
        mLiveActorKit->getCameraDirector()->setReverseVertical(
            GameDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
    }

    if (al::isExistSceneObj(this, SceneObjID_ChikaChikaBlockSynchronizer)) {
        al::getSceneObj<ChikaChikaBlockSynchronizer>(this, SceneObjID_ChikaChikaBlockSynchronizer)
            ->initAudioKeeper(actorInfo);
    }

    BlockAssistFunction::setCheckpointFlag(this, mCheckpointFlag);
    EchoEmitterHolder* emitterHolder = rc::tryGetEmitterHolder(this);

    if (emitterHolder != nullptr) {
        mLiveActorKit->getGraphicsSystemInfo()->setViewIndexedUboArray(
            "EchoBlockEmitterUbo", emitterHolder->getUboArray());
        al::registerExecutorUser(emitterHolder, mLiveActorKit->getExecuteDirector(),
                                 emitterHolder->getSceneObjName());
    }

    bool isNoAmiibo = true;

    if (al::getStageInfoMap(this, 0)->getResource()->tryGetByml("NoAmiibo") == nullptr) {
        isNoAmiibo = mIsKinopioBrigade;
    }

    mPlayerAliveWatcher = new PlayerAliveWatcher(actorInfo, mLiveActorKit->getPlayerHolder(),
                                                 false, mIsKinopioBrigade, isNoAmiibo);
    al::setSceneObj(this, mPlayerAliveWatcher, SceneObjID_PlayerAliveWatcher);
    rc::setAliveWatcherToAudio(mPlayerAliveWatcher, mLiveActorKit->getPlayerHolder());
    mDrcAssistDirectorList->setPlayerAliveWatcher(mPlayerAliveWatcher);
    mStageSceneLayout = new StageSceneLayout(
        layoutInfo, mGameDataHolder, mStageName.cstr(),
        al::getSceneObj<GreenStarKeeper>(this, SceneObjID_GreenStarKeeper),
        al::getSceneObj<IllustItemKeeper>(this, SceneObjID_IllustItemKeeper),
        mLiveActorKit->getPlayerHolder(), mPlayerAliveWatcher, itemDirector);
    mSnapshotLayout = new SnapshotLayout(layoutInfo, mStampDirector);
    mLiveActorKit->getCameraDirector()->setSnapShotAudioKeeper(mSnapshotLayout);

    if (mIsKinopioBrigade) {
        mStageSceneLayout->disableItemStock();
    }

    al::setSceneObj(this, new GuideGameWindow(layoutInfo, false), SceneObjID_GuideGameWindow);

    if (mIsKinopioBrigade) {
        mStageSceneLayout->setStageKinopioBrigade();
    }

    mStageTimer = mStageSceneLayout->getStageTimer();

    if (mGoalPole != nullptr) {
        mGoalPole->setStageTimer(mStageTimer);
    }

    if (mStartEvent != nullptr && getEventType(mStartEvent) == cStartEventTypeTimer) {
        static_cast<StageStartEventTimer*>(mStartEvent)->setStageTimer(mStageTimer);
    }

    auto* guideGameWindow = al::getSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
    al::setSceneObj(this,
                    new CameraChangeLayout(layoutInfo, mLiveActorKit->getCameraDirector(),
                                           guideGameWindow, mIsKinopioBrigade),
                    SceneObjID_CameraChangeLayout);
    mCameraChangeLayout =
        al::getSceneObj<CameraChangeLayout>(this, SceneObjID_CameraChangeLayout);
    bool isKinopioBrigade = mIsKinopioBrigade;
    mPlayerEntryMini =
        new PlayerEntryMini(layoutInfo, mGameDataHolder, mLiveActorKit->getPlayerHolder(),
                            isKinopioBrigade ? cPlayerEntryItemNumKinopioBrigade : 0);
    mResultTimerCount = new ResultTimerCount(
        mStageTimer, mGameDataHolder->getStageDataHolderPtr(), mAudioDirector);
    mPauseMenu = new PauseMenu(layoutInfo, mGameDataHolder, mPlayerAliveWatcher,
                               mLiveActorKit->getCameraDirector(),
                               mLiveActorKit->getPlayerHolder(),
                               rInfo.mGameSystemInfo->getGamePadSystem(), mScreenCaptureExecutor);
    mMissLayout = new al::SimpleLayoutAppearWait("ミス表示", "HeadMiss", layoutInfo, nullptr);
    mMissWipe = new al::WipeSimple("ミスワイプ", "WipeMiss", layoutInfo, nullptr);
    mStateGameOver = new StageSceneStateGameOver(this, layoutInfo, actorInfo, mGameDataHolder,
                                                 mAudioDirector, mMissWipe, mMissLayout,
                                                 mStageTimer->getTimeUpLayout());
    mStateGameOver->entryKillLayout(mStageSceneLayout);
    mStateGameOver->entryKillLayout(mStageTimer->getTimeUpLayout());
    endInit(actorInfo, nullptr);

    cameraDirector = mLiveActorKit->getCameraDirector();
    alTempSeadUtility::calcStereoCamera(&mStereoProjectionLeft, &mStereoCameraLeft,
                                        &mStereoProjectionRight, &mStereoCameraRight,
                                        *cameraDirector->getProjection(),
                                        *cameraDirector->getLookAtCamera(), 800.0f, 0.4f, false);

    if (mStartEvent == nullptr || GameDataFunction::isCheckpointPass(this)) {
        initNerve(&NrvStageSceneStart, 2);
    } else if (getEventType(mStartEvent) == cStartEventTypeBindDemo) {
        initNerve(&NrvStageSceneStartBindDemo, 2);
    } else if (GameDataFunction::isRestartStage(this)) {
        initNerve(&NrvStageSceneStart, 2);
    } else {
        initNerve(&NrvStageSceneStartEvent, 2);
    }

    al::initNerveState(this, mStateGameOver, &NrvStageSceneGameOver, "GameOver");
    mSnapshotState = new SnapshotState(mSnapshotLayout, this, mDrcAssistDirectorList,
                                       mStampDirector, mPlayerAliveWatcher);
    al::initNerveState(this, mSnapshotState, &NrvStageSceneCaptureMode, "SnapshotState");

    if (mMysteryHouseChecker != nullptr) {
        mGameDataHolder->getStageDataHolderPtr()->setContinuousMysteryBox();
    }

    if (mIsTimerDisabled) {
        mStageTimer->deactivate();
    }

    if (GameDataFunction::isCheckpointPass(this)) {
        mCheckpointFlag->setStateAfter();
        mStageTimer->setTimerCheckpoint();
    }
}

/**
 * Places the cameras, area objects, checkpoint, players, goals, objects and start events.
 * @param rInfo The actor init info.
 */
void StageScene::initPlacement(const al::ActorInitInfo& rInfo) {
    s32 mapNum = al::getStageInfoMapNum(this);
    mLiveActorKit->getCameraDirector()->initCameraCreator(mapNum, al::isStageOneResource(this));

    for (s32 i = 0; i < mapNum; i++) {
        mLiveActorKit->getCameraDirector()->setCameraResource(al::getStageResourceMap(this, i), i);
    }

    al::initPlacementAreaObj(this, rInfo, nullptr);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
    initPlacementCheckpoint(rInfo);
    al::createSceneObj(this, SceneObjID_SuperbViewAreaHolder);
    al::createSceneObj(this, 55);  // Unknown scene object.
    auto* selector = static_cast<PlayerRetargettingSelectorSceneObj*>(
        al::createSceneObj(this, SceneObjID_PlayerRetargettingSelector));
    initPlacementPlayer(al::getStageInfoMap(this, 0), rInfo, selector);
    PlayerStockerFunction::tryCreateDoubleMario(this, rInfo, selector, mInvincibleBgmController,
                                                false);
    initPlacementGoal(rInfo);

    ProjectActorFactory skyFactory;
    al::initPlacementByStageInfo(al::getStageInfoMap(this, 0), "SkyList", skyFactory, rInfo);

    for (s32 i = 0; i < mapNum; i++) {
        initPlacementObject(al::getStageInfoMap(this, i), rInfo, "ObjectList");
    }

    for (s32 i = 0; i < al::getStageInfoDesignNum(this); i++) {
        initPlacementObject(al::getStageInfoDesign(this, i), rInfo, "ObjectList");
    }

    for (s32 i = 0; i < al::getStageInfoSoundNum(this); i++) {
        initPlacementObject(al::getStageInfoSound(this, i), rInfo, "ObjectList");
    }

    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);
    al::PlacementInfo placementInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&placementInfo, &count, stageInfo, "DemoObjList");
    ProjectActorFactory factory;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo info;
        al::getPlacementInfoByIndex(&info, placementInfo, i);
        const char* objectName = nullptr;
        al::getObjectName(&objectName, info);
        al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &info);

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

        if (al::isEqualString(objectName, "DemoKoopaW7") &&
            !CourseInfoFunction::isClear(GameDataHolderAccessor(mGameDataHolder), mCourseId)) {
            mDemoKoopaW7 = static_cast<DemoKoopaW7*>(actor);
            mDemoKoopaW7->setGoalPole(mGoalPole);
        }
    }

    if ((GameDataFunction::isStageNormal(GameDataHolderAccessor(mGameDataHolder), mCourseId) ||
         GameDataFunction::isStageGateKeeper(GameDataHolderAccessor(mGameDataHolder),
                                             mCourseId)) &&
        GameDataFunction::getInitStageTimer(GameDataHolderAccessor(mGameDataHolder)) <= 100) {
        auto* timerEvent = new StageStartEventTimer("ステージ開始デモタイマー");
        mStartEvent = timerEvent;
        timerEvent->init(rInfo);
    }
}

/**
 * Makes the scene appear: places the players and starts the stage.
 */
void StageScene::appear() {
    decidePlayerPlacement();
    GameDataFunction::onStageStart(GameDataHolderWriter(mGameDataHolder));
    al::Scene::appear();
    getFrameworkAlive()->mIsClearRenderBuffer = false;
}

/**
 * Places the players of the active control users side by side and makes them appear.
 */
void StageScene::decidePlayerPlacement() {
    if (!mIsKinopioBrigade) {
        for (s32 i = 0; i < rc::getPlayerCharacterNumMax(); i++) {
            s32 userId = rc::tryCalcControlUserIdByCharacterType(this, i, false);

            if (userId == -1) {
                continue;
            }

            PlayerActor* player = mPlayers[i];
            GameDataHolderAccessor accessor(mGameDataHolder);
            al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
            bool isNarrowPlace = mIsNarrowPlace;
            bool isMainPort =
                rc::getControlUserPortNumber(accessor, userId) == al::getMainControllerPort();
            player->setViewMtx(isMainPort ? &cameraDirector->mMainViewMtx :
                                            &cameraDirector->mSubViewMtx);
            player->replaceInputPort(rc::getControlUserPortNumber(accessor, userId));
            rc::initPlayerFigureType(player, rc::getControlUserFigureType(accessor, userId), false);

            const sead::Vector3f& front = player->getProperty()->getFront();
            sead::Vector3f side(front.z, 0.0f, -front.x);
            side.normalize();
            s32 activeNum = rc::getActiveControlUserNum(accessor);
            s32 order = rc::calcControlUserDisplayOrder(accessor, userId);
            sead::Vector3f pos;

            if (isNarrowPlace && activeNum == 4) {
                sead::Vector3f offset;

                if (order == 3) {
                    offset = side * -cPlayerNarrowPlacementOffset -
                             front * cPlayerPlacementInterval;
                } else if (order == 2) {
                    offset = side * cPlayerNarrowPlacementOffset -
                             front * cPlayerPlacementInterval;
                } else if (order == 1) {
                    offset = side * -cPlayerNarrowPlacementOffset;
                } else {
                    offset = side * cPlayerNarrowPlacementOffset;
                }

                pos = offset + player->getProperty()->getTrans();
            } else {
                const sead::Vector3f& trans = player->getProperty()->getTrans();
                f32 center = (activeNum - 1) * 0.5f * cPlayerPlacementInterval;
                f32 offset = -order * cPlayerPlacementInterval;
                pos = trans + side * center + side * offset;
            }

            player->getProperty()->mTrans = pos;
            al::setTrans(player, pos);
            player->updatePosture();
            player->appear();
            player->getModelHolder()->appear();

            if (mPlayerEntryMini != nullptr) {
                mPlayerEntryMini->setItemActive(userId);
            }

            if (mGameDataHolder->tryGetLastStageBestScoreUserID() == userId) {
                mPlayerCrown->changeHost(mPlayers[i]);
                mPlayerCrown->appear();
            }

            if (rc::calcControlUserDisplayOrder(GameDataHolderAccessor(mGameDataHolder), userId) ==
                0) {
                rc::setMainPlayerActor(player);
            }
        }

        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->startPlayerEntry(false);
        }

        mPlayerAliveWatcher->appear();
    } else {
        for (s32 i = 0; i < rc::getPlayerCharacterNumMax(); i++) {
            s32 userId =
                rc::tryCalcControlUserIdByCharacterType(this, i, false);

            if (userId == -1) {
                continue;
            }

            PlayerActor* player = mPlayers[userId];
            GameDataHolderAccessor accessor(mGameDataHolder);
            al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
            bool isMainPort =
                rc::getControlUserPortNumber(accessor, userId) == al::getMainControllerPort();
            player->setViewMtx(isMainPort ? &cameraDirector->mMainViewMtx :
                                            &cameraDirector->mSubViewMtx);
            player->replaceInputPort(rc::getControlUserPortNumber(accessor, userId));
            player->updatePosture();
            player->appear();
            player->getModelHolder()->appear();

            if (mPlayerEntryMini != nullptr) {
                mPlayerEntryMini->setItemActive(userId);
            }

            if (userId == 0) {
                rc::setMainPlayerActor(player);
            }
        }

        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->appear();
        }
    }
}

/**
 * Kills the scene.
 */
void StageScene::kill() {
    al::Scene::kill();
}

/**
 * Updates the cameras, the player cooperation and the fur of the scene once per frame.
 */
void StageScene::control() {
    mIsControlled = true;
    mLiveActorKit->preDrawGraphics();
    mCameraObserver->update();
    mCameraChangeLayout->update();
    al::getSceneObj<PlayerCooperation>(this, SceneObjID_PlayerCooperation)->update();
    al::getSceneObj<FurEnv>(this, SceneObjID_FurEnv)->update();
}

/**
 * Draws the main screen: the 3D view, the stamps and the 2D layouts.
 */
void StageScene::drawMain_() const {
    al::GameFrameworkNx* framework = getFramework();
    agl::RenderBuffer* renderBuffer = framework->getCurrentRenderBuffer();
    mMainViewport.setByFrameBuffer(*renderBuffer);
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFrameworkAlive()->mIsDocked);
    mLayoutKit->setFrameBuffer(renderBuffer, &mMainViewport);

    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    sead::LookAtCamera* camera = cameraDirector->getLookAtCamera();
    const sead::Matrix44f& projMtx = cameraDirector->getProjection()->getProjectionMatrix();
    alSystemKitFunction::applyViewportTop(mMainViewport);
    EchoEmitterHolder* emitterHolder = rc::tryGetEmitterHolder(this);

    if (emitterHolder != nullptr) {
        emitterHolder->setToUbo(0, camera->getMatrix(), projMtx);
    }

    if (isDraw3D() && mIsControlled) {
        cameraDirector = mLiveActorKit->getCameraDirector();
        cameraDirector->getProjection()->getNear();
        mLiveActorKit->getCameraDirector()->getProjection()->getFar();

        al::LiveActorKit* kit = mLiveActorKit;
        al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
        const al::SceneCameraInfo* cameraInfo = getSceneCameraInfo();
        bool isFadeScreen = false;

        if (al::isNerve(this, &NrvStageSceneGameOver) || al::isNerve(this, &NrvStageSceneRestart) ||
            al::isNerve(this, &NrvStageSceneRestartMiss)) {
            isFadeScreen = !mPlayerAliveWatcher->mIsAbyss;
        }

        viewRenderer->drawView(0, 0, kit, cameraInfo, renderBuffer, mMainViewport, true,
                               isFadeScreen, static_cast<agl::ShaderMode>(4));
        mStampDirector->draw2D(renderBuffer);
    }

    const char* kitName = getDraw2DKitMainName();
    al::drawKit(this, kitName);

    if (mScreenCaptureExecutor->isDraw(1)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     renderBuffer, 1);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           renderBuffer, 1);
    }

    al::drawKit(this, "2DDrawAboveBlur1");

    if (mScreenCaptureExecutor->isDraw(2)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     renderBuffer, 2);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           renderBuffer, 2);
    }

    al::drawKit(this, "2DDrawAboveBlur2");
    mLiveActorKit->getCameraDirector()->getProjection()->setOffset(sead::Vector2f::zero);
}

/**
 * Gets whether the 3D view is drawn.
 * @return false while the pause menu hides the stage, in the illust item result and while the
 *         scene ends.
 */
bool StageScene::isDraw3D() const {
    if (al::isNerve(this, &NrvStageScenePause) && !mPauseMenu->isForceExit()) {
        return false;
    }

    if (al::isNerve(this, &NrvStageSceneIllustItemResult) && al::isGreaterEqualStep(this, 2)) {
        return false;
    }

    if (al::isNerve(this, &NrvStageSceneLoadGame)) {
        return false;
    }

    return !al::isNerve(this, &NrvStageSceneRetire);
}

/**
 * Gets the name of the layout kit drawn on the main screen.
 * @return The miss layouts while the players miss, the base layouts otherwise.
 */
const char* StageScene::getDraw2DKitMainName() const {
    if (isGameOver() || isRestartStage() || al::isNerve(this, &NrvStageSceneCaptureMode)) {
        return "２Ｄミス（メイン画面）";
    }

    return "２Ｄベース（メイン画面）";
}

/**
 * Draws the sub screen (nothing is drawn).
 */
void StageScene::drawSub_() const {}

/**
 * Gets whether the players lost their last life.
 * @return true in the game over.
 */
bool StageScene::isGameOver() const {
    return al::isNerve(this, &NrvStageSceneGameOver);
}

/**
 * Gets whether the stage restarts after a miss.
 * @return true if the stage restarts.
 */
bool StageScene::isRestartStage() const {
    return al::isNerve(this, &NrvStageSceneRestart) || al::isNerve(this, &NrvStageSceneRestartMiss);
}

/**
 * Gets the name of the layout kit drawn on the sub screen.
 * @return The miss layouts while the players miss, the base layouts otherwise.
 */
const char* StageScene::getDraw2DKitSubName() const {
    if (isGameOver() || isRestartStage()) {
        return "２Ｄミス（サブ画面）";
    }

    return "２Ｄベース（サブ画面）";
}

/**
 * Gets whether the stage has no start event.
 * @return true if there is no start event.
 */
bool StageScene::isNotExistStartDemo() const {
    return mStartEvent == nullptr;
}

/**
 * Gets whether the stage is cleared.
 * @return true after the goal.
 */
bool StageScene::isGoal() const {
    return al::isNerve(this, &NrvStageSceneGoal);
}

/**
 * Gets whether the stage is left from the pause menu or the goal.
 * @return true if the stage is retired.
 */
bool StageScene::isRetire() const {
    return al::isNerve(this, &NrvStageSceneRetire);
}

/**
 * Gets whether the stage is entered again from the pause menu.
 * @return true if the stage is reentered.
 */
bool StageScene::isReenterStage() const {
    return al::isNerve(this, &NrvStageSceneReenterStage);
}

/**
 * Gets whether the players missed in a golden express course.
 * @return true after a miss in a golden express course.
 */
bool StageScene::isDeadGoldenExpress() const {
    return al::isNerve(this, &NrvStageSceneDeadGoldenExpress);
}

/**
 * Gets whether a mystery box course restarts after a miss.
 * @return true if the mystery box course restarts.
 */
bool StageScene::isRestartMysteryBox() const {
    return al::isNerve(this, &NrvStageSceneRestartMysteryBox);
}

/**
 * Gets whether the time of a mystery box course is up.
 * @return true if the mystery box time is up.
 */
bool StageScene::isTimeUpMysteryBox() const {
    return al::isNerve(this, &NrvStageSceneTimeUpMysteryBox);
}

/**
 * Gets whether the game ends after a game over.
 * @return true if the game ends.
 */
bool StageScene::isGameEnd() const {
    return al::isNerve(this, &NrvStageSceneGameEnd);
}

/**
 * Gets whether the game is changed from the pause menu.
 * @return true if the game is changed.
 */
bool StageScene::isGameChange() const {
    return al::isNerve(this, &NrvStageSceneGameChange);
}

/**
 * Gets whether a save file is loaded from the pause menu.
 * @return true if a save file is loaded.
 */
bool StageScene::isLoadGame() const {
    return al::isNerve(this, &NrvStageSceneLoadGame);
}

/**
 * Gets whether the players warp to another world.
 * @return true after a world warp.
 */
bool StageScene::isWorldWarp() const {
    return al::isNerve(this, &NrvStageSceneWorldWarp);
}

/**
 * Gets whether the result of the goal is shown.
 * @return true if the goal shows the result.
 */
bool StageScene::isUseResult() const {
    return mGoalObjHolder->isUseResult();
}

/**
 * Gets whether the stage restarts from a checkpoint.
 * @return true if the stage restarts or is reentered.
 */
bool StageScene::isRestartCheck() const {
    if (al::isNerve(this, &NrvStageSceneRestart) || al::isNerve(this, &NrvStageSceneRestartMiss) ||
        al::isNerve(this, &NrvStageSceneRestartMysteryBox) ||
        al::isNerve(this, &NrvStageSceneTimeUpMysteryBox)) {
        return true;
    }

    return al::isNerve(this, &NrvStageSceneReenterStage);
}

/**
 * Records the end of the stage in the save data before the scene is destroyed.
 */
void StageScene::prepareDestroy() {
    if (mIsPrepareDestroyed) {
        return;
    }

    mIsPrepareDestroyed = true;
    GameDataFunction::onStageEnd(GameDataHolderWriter(mGameDataHolder));
    al::changeBgmSituation(this, "ChangeHurryToNormal");
    al::changeBgmSituation(this, "OutWater");

    if (al::isNerve(this, &NrvStageSceneReenterStage)) {
        GameDataFunction::reenterStage(GameDataHolderWriter(mGameDataHolder));
        GameDataFunction::incMenuRestartCount(GameDataHolderWriter(mGameDataHolder));
    } else if (isRestartStage()) {
        if (mIsKinopioBrigade) {
            GameDataFunction::restartKinopioBrigade(GameDataHolderWriter(mGameDataHolder));
        } else {
            GameDataFunction::restartStage(GameDataHolderWriter(mGameDataHolder));
        }
    } else if (al::isNerve(this, &NrvStageSceneRestartMysteryBox)) {
        GameDataFunction::restartMysteryBox(GameDataHolderWriter(mGameDataHolder));
    } else if (al::isNerve(this, &NrvStageSceneTimeUpMysteryBox)) {
        GameDataFunction::restartTimeupMysteryBox(GameDataHolderWriter(mGameDataHolder));
    } else if (isWorldWarp()) {
        GameDataFunction::clearStageWorldWarp(GameDataHolderWriter(mGameDataHolder));

        if (mPlayerCrown != nullptr) {
            s32 userId = -1;

            if (mPlayerCrown->isAttach() && mPlayerCrown->getHost() != nullptr) {
                userId = rc::findControlUserId(mPlayerCrown->getHost());
            }

            GameDataFunction::setBestScoreUserId(GameDataHolderWriter(mGameDataHolder), userId);
        }
    } else if (isGoal()) {
        if (mMysteryHouseChecker != nullptr && mGoalObjHolder->isRetireGoal()) {
            GameDataFunction::retireStage(GameDataHolderWriter(mGameDataHolder));
        } else if (mBlockChoiceWatcher != nullptr && !mBlockChoiceWatcher->isReleasedItem()) {
            GameDataFunction::retireStageExitDoor(GameDataHolderWriter(mGameDataHolder));
        } else if (GameDataFunction::isStageFairyHouse(GameDataHolderAccessor(mGameDataHolder),
                                                       mCourseId) &&
                   !GameDataFunction::isAcquireIllustItem(
                       GameDataHolderAccessor(mGameDataHolder))) {
            GameDataFunction::retireStageExitDoor(GameDataHolderWriter(mGameDataHolder));
        } else {
            GameDataFunction::clearStage(GameDataHolderWriter(mGameDataHolder));
            GameDataFunction::playReportStageEvent(GameDataHolderWriter(mGameDataHolder), 0);

            if (mIsKinopioBrigade) {
                return;
            }

            if (mGoalObjHolder->isUseResult()) {
                GameDataFunction::updateBestScoreUser(GameDataHolderWriter(mGameDataHolder));
            } else if (mPlayerCrown != nullptr) {
                s32 userId = -1;

                if (mPlayerCrown->isAttach() && mPlayerCrown->getHost() != nullptr) {
                    userId = rc::findControlUserId(mPlayerCrown->getHost());
                }

                GameDataFunction::setBestScoreUserId(GameDataHolderWriter(mGameDataHolder),
                                                     userId);
            }
        }
    } else if (al::isNerve(this, &NrvStageSceneRetire)) {
        GameDataFunction::retireStage(GameDataHolderWriter(mGameDataHolder));
        GameDataFunction::playReportStageEvent(GameDataHolderWriter(mGameDataHolder), 2);
    } else if (al::isNerve(this, &NrvStageSceneGameOver)) {
        GameDataFunction::playReportStageEvent(GameDataHolderWriter(mGameDataHolder), 1);

        if (GameDataFunction::isStageGoldenExpress(GameDataHolderAccessor(mGameDataHolder),
                                                   mCourseId)) {
            GameDataFunction::recoverGameOverFromGoldenExpress(
                GameDataHolderWriter(mGameDataHolder));
        } else {
            GameDataFunction::recoverGameOver(GameDataHolderWriter(mGameDataHolder));
        }
    } else if (al::isNerve(this, &NrvStageSceneDeadGoldenExpress)) {
        GameDataFunction::clearStage(GameDataHolderWriter(mGameDataHolder));
        GameDataFunction::setBestScoreUserId(GameDataHolderWriter(mGameDataHolder), -1);
        GameDataFunction::playReportStageEvent(GameDataHolderWriter(mGameDataHolder), 0);
    } else if (al::isNerve(this, &NrvStageSceneGameEnd)) {
    }
}

/**
 * Records the figure, the survival and the goal state of every control user.
 */
void StageScene::recordClearData() {
    StageDataHolder* stageDataHolder = mGameDataHolder->getStageDataHolderPtr();
    al::LiveActor* player = al::getPlayerActor(mLiveActorKit->getPlayerHolder(), 0);

    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        const al::LiveActor* alivePlayer = rc::tryFindAlivePlayerActorFirstByUserId(player, i);

        if (alivePlayer != nullptr) {
            stageDataHolder->setPlayerFigureType(i, rc::getPlayerFigureType(alivePlayer));
            stageDataHolder->setAlive(i, true);
        } else {
            bool isWaitBubble = false;

            if (rc::isActiveControlUser(GameDataHolderAccessor(mGameDataHolder), i)) {
                s32 characterType =
                    rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder), i);
                isWaitBubble = mPlayerAliveWatcher->isWaitBubbleForRevive(characterType);
            }

            stageDataHolder->setPlayerFigureType(i, rc::getPlayerFigureTypeDefault());
            stageDataHolder->setAlive(i, isWaitBubble);
        }

        if (mGoalPole == nullptr) {
            if (alivePlayer != nullptr) {
                stageDataHolder->setGoalState(i, true, 0.0f, false);
            } else {
                stageDataHolder->setGoalState(i, false, 0.0f, false);
            }
        } else if (mGoalPole->isCatchSuccess(i)) {
            f32 rate = mGoalPole->calcCatchHeightRate(i);
            stageDataHolder->setGoalState(i, true, rate, i == mGoalPole->calcHighestUserId());
        } else {
            stageDataHolder->setGoalState(i, false, 0.0f, false);
        }
    }

    if (mMysteryHouseChecker != nullptr && mMysteryHouseChecker->isGoal()) {
        s32 userId = mMysteryHouseChecker->tryCalcLastAcquirerUserId();

        if (userId >= 0) {
            stageDataHolder->setGoalState(userId, true, 1.0f, true);
        }
    }
}

/**
 * Ignores the input of every player for the next frames.
 */
void StageScene::invalidatePlayerInput() {
    s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());

    for (s32 i = 0; i < playerNum; i++) {
        static_cast<PlayerActor*>(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i))
            ->getInput()
            ->invalidateFrame(2);
    }
}

/**
 * Gets whether the start wipe can open.
 * @return true if there is no start event, if the stage restarts or if the start event allows it.
 */
bool StageScene::isEnableOpenStartWipe() const {
    if (mStartEvent == nullptr) {
        return true;
    }

    if (getEventType(mStartEvent) != cStartEventTypeBindDemo &&
        GameDataFunction::isRestartStage(GameDataHolderAccessor(mGameDataHolder))) {
        return true;
    }

    return mStartEvent->isEnableOpenStartWipe();
}

/**
 * Plays the start event of the stage.
 */
void StageScene::exeStartEvent() {
    if (al::isFirstStep(this)) {
        rc::disableGuideGameWindowPriority(this);
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);
        mStartEvent->startDemo();
        alSeFunction::changeListenerPoserDemo(mAudioDirector);

        if (getEventType(mStartEvent) == cStartEventTypeTimer) {
            mStageSceneLayout->startDemo(false, false);
        } else {
            mStageSceneLayout->startDemo(true, true);
        }

        rc::disappearCameraChangeLayout(this);
        al::deactivateAudioEventController(this);
    }

    updateStartEvent();

    if (mStartEvent->isEndDemo()) {
        if (getEventType(mStartEvent) == cStartEventTypeTimer) {
            mStageSceneLayout->endDemo(false, false);
        } else {
            mStageSceneLayout->endDemo(true, true);
        }

        alSeFunction::changeListenerPoserLast(mAudioDirector);
        al::setNerve(this, &NrvStageSceneStart);
    }
}

/**
 * Updates the scene while the start event plays.
 */
void StageScene::updateStartEvent() {
    mLiveActorKit->getCameraDirector()->update(false);

    if (mStartEvent->isEnableMovement()) {
        al::updateKit(this);
        return;
    }

    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "Clipping");
    al::updateKitList(this, "デモプレイヤーロケーター");
    al::updateKitList(this, "デモプレイヤー前処理");

    if (getEventType(mStartEvent) == cStartEventTypeTimer) {
        al::updateKitList(this, "プレイヤー[Movement]");
    }

    al::updateKitList(this, "プレイヤー");
    al::updateKitList(this, "プレイヤー装飾");
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

/**
 * Plays the start demo binding the players (Captain Toad, Toad houses...).
 */
void StageScene::exeStartBindDemo() {
    if (al::isFirstStep(this)) {
        mStartEvent->startDemo();
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ開始デモ", 0,
                                                    false);
        updatePlay();
        mDrcAssistDirectorList->disappearTouchPointer();
        rc::disappearCameraChangeLayout(this);
        mStageSceneLayout->startDemo(false, true);

        if (mPlayerEntryMini != nullptr &&
            !GameDataFunction::isStageKinopioBrigade(GameDataHolderAccessor(mGameDataHolder),
                                                     mCourseId)) {
            mPlayerEntryMini->startDemo();
        }

        mPlayerAliveWatcher->startDemo();
        al::deactivateAudioEventController(this);

        if (mCourseId >= 0 && GameDataFunction::isStageKinopioBrigade(
                                  GameDataHolderAccessor(mGameDataHolder), mCourseId)) {
            al::activateSePlayEvent(this);
            al::activateAudioEffectChangeEvent(this);
            al::changeAudioEffectWithAreaCheck(this);
        }

        return;
    }

    updatePlay();

    if (mStartEvent->isEndDemo()) {
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 60, false);
        mStageSceneLayout->endDemo(true, true);

        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->endDemo();
        }

        mPlayerAliveWatcher->endDemo();
        alSeFunction::changeListenerPoserLast(mAudioDirector);
        al::setNerve(this, &NrvStageSceneStart);
    }
}

/**
 * Updates the scene while the players play.
 */
void StageScene::updatePlay() {
    if (al::isStopScene(this)) {
        al::updateKitList(this, "プレイヤー[Movement]");
        al::updateEffectSystem(this);
        return;
    }

    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    cameraDirector->update(false);
    cameraDirector->_154 = mPlayerAliveWatcher->isEnableGyroCamera();

    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        s32 port = rc::getPadPortByUserId(i);

        if (rc::isActiveControlUser(GameDataHolderAccessor(mGameDataHolder), i)) {
            cameraDirector->validUserCameraControlByPort(port);
        } else {
            cameraDirector->invalidUserCameraControlByPort(port);
        }
    }

    if (al::isNerve(this, &NrvStageScenePlay)) {
        rc::updateDrcAssistDirector(this);
    }

    bool isPlay = al::isNerve(this, &NrvStageScenePlay);
    bool isGoalDemo = al::isNerve(this, &NrvStageSceneGoalDemo) && mGoalPole != nullptr &&
                      !mGoalPole->isGoalDemoPlaying();

    if ((isPlay || isGoalDemo) &&
        al::isExistSceneObj(this, SceneObjID_ChikaChikaBlockSynchronizer)) {
        rc::updateChikaChikaSynchronizer(this);
    }

    al::updateKit(this);
    mMicInputSePlayer->update();
    mPlayerAliveWatcher->update();
}

/**
 * Starts the stage: ghosts, BGM and save data.
 */
void StageScene::exeStart() {
    if (al::isFirstStep(this)) {
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);
        rc::setControllerConnectDisabled(false);

        if (!GameDataFunction::isCheckpointPass(this) &&
            rc::getActiveControlUserNum(this) == 1) {
            mGhostPlayerDirector->tryStartRecord();
        }

        mGhostPlayerDirector->tryStartPlay();
        GameDataHolderAccessor accessor(mGameDataHolder);
        s32 courseId = mCourseId;
        mIsGhostRecording = true;
        al::activateAudioEventController(this);
        al::changeAudioEffectWithAreaCheck(this);

        if (courseId >= 0 && (GameDataFunction::isStageKinopioHouse(accessor, courseId) ||
                              GameDataFunction::isStageKinopioHouseHide(accessor, courseId) ||
                              GameDataFunction::isStageContinuousMysteryBox(accessor, courseId))) {
        } else if (courseId >= 0 && GameDataFunction::isStageFairyHouse(accessor, courseId)) {
            al::startSequenceBgm(this, "FairyHouse", -1, 0);
        } else {
            al::startBgmWithAreaCheck(
                this, GameDataFunction::isCheckpointPass(this), -1, 0, -1);
        }

        rc::appearCameraChangeLayout(this);
        GameDataFunction::setSkipStartDemo(GameDataHolderWriter(this), false);
        SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, false);
        rc::enableGuideGameWindowPriority(this);
    }

    invalidatePlayerInput();
    updatePlay();
    al::setNerve(this, &NrvStageScenePlay);
}

/**
 * Plays the stage until the goal, a miss, a demo or the pause.
 */
void StageScene::exePlay() {
    if (mInvalidateInputFrame > 0) {
        invalidatePlayerInput();
        mInvalidateInputFrame--;
    }

    updatePlay();
    mInvincibleBgmController->update();
    mBigBgmController->update();

    if (mGoalObjHolder != nullptr && mGoalObjHolder->isGoal()) {
        mStageTimer->clearStage();

        if (mGoalObjHolder->isEndGoalDemo()) {
            al::setNerve(this, &NrvStageSceneGoal);
        } else {
            al::setNerve(this, &NrvStageSceneGoalDemo);
        }

        return;
    }

    if (mPlayerAliveWatcher->isGameOver()) {
        mDrcAssistDirectorList->disappearTouchPointerImmediately();
        mLiveActorKit->getGraphicsSystemInfo()->mAreaTarget = 2;

        if (GameDataFunction::isGameOver(GameDataHolderAccessor(mGameDataHolder))) {
            al::setNerve(this, &NrvStageSceneGameOver);
        } else if (mMysteryHouseChecker != nullptr) {
            al::setNerve(this, &NrvStageSceneRestartMysteryBox);
        } else if (GameDataFunction::isStageGoldenExpress(GameDataHolderAccessor(mGameDataHolder),
                                                          mCourseId)) {
            al::setNerve(this, &NrvStageSceneDeadGoldenExpress);
        } else {
            al::setNerve(this, &NrvStageSceneRestart);
        }

        return;
    }

    if (mMysteryHouseChecker != nullptr && mMysteryHouseChecker->isMiss()) {
        GameDataFunction::addMissCount(GameDataHolderWriter(mGameDataHolder));
        al::setNerve(this, &NrvStageSceneTimeUpMysteryBox);
        return;
    }

    if (mStageTimer->isTimeUp()) {
        mLiveActorKit->getGraphicsSystemInfo()->mAreaTarget = 2;

        if (mGoalPole != nullptr) {
            mGoalPole->invalidateBind();
        }

        forceKillPlayerAll();
        return;
    }

    if (rc::isPlayerChangeDemoAny(mLiveActorKit->getPlayerHolder())) {
        al::setNerve(this, &NrvStageSceneDemoChangePlayer);
        return;
    }

    if (rc::isActiveDemoCamera(this)) {
        al::setNerve(this, &NrvStageSceneDemoCamera);
        return;
    }

    if (rc::isActiveDemoPlayerCutscene(this)) {
        al::setNerve(this, &NrvStageScenePreGoalDemo);
        return;
    }

    if (rc::isActiveDemoPlayer(this) || rc::isActiveDemoBinding(this)) {
        al::setNerve(this, &NrvStageSceneDemoScene);
        return;
    }

    if (rc::isGuideGameWindowWaitConfirm(this)) {
        al::setNerve(this, &NrvStageScenePauseWindowMessage);
        return;
    }

    if (isTriggerPause(&mPausePort)) {
        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->hide();
        }

        auto* superbViewAreaHolder =
            al::getSceneObj<SuperbViewAreaHolder>(this, SceneObjID_SuperbViewAreaHolder);

        if (superbViewAreaHolder != nullptr) {
            superbViewAreaHolder->startPause();
        }

        mPlayerAliveWatcher->startPause();
        mScreenCaptureExecutor->requestCapture(false, 1, true);
        al::setNerve(this, &NrvStageScenePause);
        return;
    }

    if (mIllustItemWatcher != nullptr && mIllustItemWatcher->isShowLayout()) {
        mScreenCaptureExecutor->requestCapture(false, 1, false);
        al::setNerve(this, &NrvStageSceneIllustItemResult);
        return;
    }

    if (PlayLogFunction::isUseDrcOneUser(GameDataHolderAccessor(mGameDataHolder))) {
        s32 port = al::getMainControllerPort();

        if (al::isPadHoldUp(port) || al::isPadHoldDown(port) || al::isPadHoldLeft(port) ||
            al::isPadHoldRight(port)) {
            PlayLogFunction::useCrossKey(GameDataHolderWriter(mGameDataHolder));
        }
    }

    if (mLiveActorKit->getCameraDirector()->_162) {
        PlayLogFunction::useCameraRotate(GameDataHolderWriter(mGameDataHolder));
    }

    if (al::isPadTriggerDown(al::getMainControllerPort()) &&
        rc::findActiveUserIdList(nullptr, this) <= 1) {
        al::setNerve(this, &NrvStageSceneCaptureMode);
    }
}

/**
 * Kills every alive player when the time is up.
 */
void StageScene::forceKillPlayerAll() {
    al::PlayerHolder* playerHolder = mLiveActorKit->getPlayerHolder();

    for (s32 i = 0; i < al::getPlayerNumMax(playerHolder); i++) {
        al::LiveActor* player = al::getPlayerActor(playerHolder, i);

        if (!rc::isPlayerDead(player)) {
            rc::forceKillPlayer(player);
        }
    }

    al::setNerve(this, &NrvStageScenePreTimeUp);
}

/**
 * Plays the demo of a player changing character.
 */
void StageScene::exeDemoChangePlayer() {
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
    al::setNerve(this, &NrvStageScenePlay);
}

/**
 * Updates the players while a player changes character.
 */
void StageScene::updateDemoChangePlayer() {
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "プレイヤー前処理");
    al::updateKitList(this, "プレイヤー[Movement]");
    al::updateKitList(this, "プレイヤー後処理");
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateEffectPlayer(this);
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    mLiveActorKit->updateGraphics(false);
}

/**
 * Waits after the player change demo before playing again.
 */
void StageScene::exeDemoChangePlayerAfter() {
    updateDemoChangePlayerAfter();

    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvStageScenePlay);
    }
}

/**
 * Updates the player effects and the layouts after the player change demo.
 */
void StageScene::updateDemoChangePlayerAfter() {
    al::updateEffectPlayer(this);
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
}

/**
 * Plays a camera demo.
 */
void StageScene::exeDemoCamera() {
    if (al::isFirstStep(this)) {
        alAudioSystemFunction::startDemo(mAudioDirector, cAudioDemoType);
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mStageTimer->startDemo();
        mDrcAssistDirectorList->disappearTouchPointerEffect();
    }

    updateDemoCamera();

    if (al::isActiveDemo(this)) {
        return;
    }

    mStageTimer->endDemo();
    alAudioSystemFunction::endDemo(mAudioDirector, cAudioDemoType);
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    al::setNerve(this, &NrvStageScenePlay);
}

/**
 * Updates the scene while a camera demo plays.
 */
void StageScene::updateDemoCamera() {
    mLiveActorKit->getCameraDirector()->update(false);
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "ライト管理");
    al::updateKitList(this, "Clipping");
    al::updateDemoActor(this);
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectDemo(this);
    mLiveActorKit->updateGraphics(false);
    mAudioDirector->update();
}

/**
 * Plays a demo of the players or a binding demo.
 */
void StageScene::exeDemoScene() {
    if (al::isFirstStep(this)) {
        alAudioSystemFunction::startDemo(mAudioDirector, cAudioDemoType);
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointer();
        mStageSceneLayout->startDemo(false, true);

        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->startDemo();
        }

        mPlayerAliveWatcher->startDemo();
    }

    updatePlay();

    if (al::isActiveDemo(this)) {
        return;
    }

    alAudioSystemFunction::endDemo(mAudioDirector, cAudioDemoType);
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    mStageSceneLayout->endDemo(true, true);

    if (mPlayerEntryMini != nullptr) {
        mPlayerEntryMini->endDemo();
    }

    mPlayerAliveWatcher->endDemo();
    al::setNerve(this, &NrvStageScenePlay);
}

/**
 * Plays a cutscene of the players before the goal.
 */
void StageScene::exePreGoalDemo() {
    if (al::isFirstStep(this)) {
        mDrcAssistDirectorList->disappearTouchPointerEffect();
    }

    updateDemoPlayerOnly();

    if (!al::isActiveDemo(this)) {
        al::setNerve(this, &NrvStageScenePlay);
    }
}

/**
 * Updates the scene while only the demo actors and the players move.
 */
void StageScene::updateDemoPlayerOnly() {
    mLiveActorKit->getCameraDirector()->update(false);
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    al::updateKitList(this, "エフェクトオブジェ");
    al::updateKitList(this, "ライト管理");
    al::updateKitList(this, "Clipping");
    al::updateKitList(this, "デモ");
    al::updateKitList(this, "空");
    al::updateKitList(this, "敵");
    al::updateKitList(this, "地形オブジェ");
    al::updateKitList(this, "地形オブジェ[Movement]");
    al::updateKitList(this, "乗り物");
    al::updateKitList(this, "プレイヤー[Movement]");
    al::updateKitList(this, "プレイヤー");
    al::updateKitList(this, "プレイヤー装飾");
    al::updateDemoActorWithEffects(this);
    al::updateKitList(this, "シャドウマスク");
    al::updateKitList(this, "グラフィックス要求者");
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffect(this);
    mLiveActorKit->updateGraphics(false);
    mAudioDirector->update();
}

/**
 * Plays the goal demo, then shows the result or leaves the stage.
 */
void StageScene::exeGoalDemo() {
    updatePlay();

    if (al::isFirstStep(this)) {
        mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer()->setForceFilterAA(true);
        mResultTimerCount->appear();
        static_cast<ProjectItemDirector*>(mLiveActorKit->getItemDirector())->setDemoMode();

        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->startDemo();
        }

        mPlayerAliveWatcher->startDemo();
        mDrcAssistDirectorList->disappearTouchPointer();

        if (mIsKinopioBrigade) {
            rc::disappearGyroIconInKinopioBrigade(this);
        }

        rc::disappearCameraChangeLayout(this);
        mGhostPlayerDirector->tryEndPlay();
    }

    if (al::isStep(this, 120)) {
        mStageSceneLayout->courseClear();
    }

    if (mIsGhostRecording && mGhostPlayerDirector->isEndRecord()) {
        mIsGhostRecording = false;
        mGhostPlayerDirector->trySaveRecord();
        rc::tryRequestDownloadGhostData(
            mNetworkSystem, mGameDataHolder, mStageName.cstr(),
            GameDataFunction::getFileId(GameDataHolderAccessor(mGameDataHolder)),
            mDokanWorldWarp != nullptr && mDokanWorldWarp->isGoal());
    }

    if (!mGoalObjHolder->isEndGoalDemo()) {
        return;
    }

    alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ終了", 30,
                                                false);
    alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "ステージ終了", 30,
                                                false);

    if (mGoalObjHolder->isUseResult()) {
        if (mIsTimerDisabled) {
            al::setNerve(this, &NrvStageSceneGoal);
        } else {
            al::setNerve(this, &NrvStageSceneResultTimerCount);
        }

        return;
    }

    if ((GameDataFunction::isStageKinopioHouse(GameDataHolderAccessor(mGameDataHolder),
                                               mCourseId) ||
         GameDataFunction::isStageKinopioHouseHide(GameDataHolderAccessor(mGameDataHolder),
                                                   mCourseId)) &&
        mBlockChoiceWatcher != nullptr && !mBlockChoiceWatcher->isReleasedItem()) {
        al::setNerve(this, &NrvStageSceneRetire);
        return;
    }

    if (GameDataFunction::isStageFairyHouse(GameDataHolderAccessor(mGameDataHolder), mCourseId) &&
        !GameDataFunction::isAcquireIllustItem(GameDataHolderAccessor(mGameDataHolder))) {
        al::setNerve(this, &NrvStageSceneRetire);
        return;
    }

    if (mDokanWorldWarp != nullptr) {
        mIsWaitEffectBeforeKill = true;
        al::setNerve(this, &NrvStageSceneWorldWarp);
        return;
    }

    al::setNerve(this, &NrvStageSceneGoal);
}

/**
 * Shows the message of a guide window.
 */
void StageScene::exePauseWindowMessage() {
    if (al::isFirstStep(this)) {
        alAudioSystemFunction::startDemo(mAudioDirector, cAudioDemoType);
        alSeFunction::changeListenerPoserDemo(mAudioDirector);
        mDrcAssistDirectorList->disappearTouchPointer();
        mStageSceneLayout->startDemo(false, true);

        if (mPlayerEntryMini != nullptr) {
            mPlayerEntryMini->startDemo();
        }

        mPlayerAliveWatcher->startDemo();
    }

    invalidatePlayerInput();
    updatePlay();

    if (rc::isGuideGameWindowWaitConfirm(this)) {
        return;
    }

    alAudioSystemFunction::endDemo(mAudioDirector, cAudioDemoType);
    alSeFunction::changeListenerPoserLast(mAudioDirector);
    mStageSceneLayout->endDemo(true, true);

    if (mPlayerEntryMini != nullptr) {
        mPlayerEntryMini->endDemo();
    }

    mPlayerAliveWatcher->endDemo();
    al::setNerve(this, &NrvStageScenePlay);
}

/**
 * Counts the remaining time into the score after the goal.
 */
void StageScene::exeResultTimerCount() {
    if (al::isFirstStep(this)) {
        mResultTimerCount->start();
        return;
    }

    updateLayout();
    mResultTimerCount->update();

    if (mResultTimerCount->isWait()) {
        if (mDemoKoopaW7 != nullptr) {
            al::setNerve(this, &NrvStageSceneKoopaDemoW7);
        } else {
            al::setNerve(this, &NrvStageSceneGoal);
        }
    }
}

/**
 * Updates the layouts.
 */
void StageScene::updateLayout() {
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Plays the Bowser demo of world 7 after the goal.
 */
void StageScene::exeKoopaDemoW7() {
    if (al::isFirstStep(this)) {
        mDemoKoopaW7->startDemo();
    }

    updatePlay();

    if (mDemoKoopaW7->isEndDemo()) {
        al::setNerve(this, &NrvStageSceneGoal);
    }
}

/**
 * Clears the stage and ends the scene.
 */
void StageScene::exeGoal() {
    if (al::isFirstStep(this) && !mIsKinopioBrigade) {
        recordClearData();
    }

    if (isStageSequenceBgm(mGameDataHolder, mCourseId)) {
        al::stopAllSequenceBgm(this, 60);
    }

    kill();
}

/**
 * Gets whether a connected player pressed the pause button.
 * @param pPort Set to the port of the player who pressed the button.
 * @return true if the pause button is pressed.
 */
bool StageScene::isTriggerPause(s32* pPort) const {
    if (!InGameSceneBase::isTriggerPause(pPort)) {
        return false;
    }

    if (mStageWipeKeeper != nullptr &&
        (mStageWipeKeeper->isActiveStartWipe() || mStageWipeKeeper->isActiveRetryWipe())) {
        return false;
    }

    sead::BitFlag16 activePorts =
        rc::getActiveInputPortList(GameDataHolderAccessor(mGameDataHolder));
    s32 mainPort = al::getMainControllerPort();

    if (activePorts.isOnBit(mainPort)) {
        al::isPadTriggerPlus(rc::tryGetConnectCheckedPort(mainPort));
    }

    for (s32 port = 1; port <= al::getMaxControllerPorts(); port++) {
        bool isConnected = false;

        if (activePorts.isOnBit(port)) {
            isConnected = al::isPadConnected(port);
        }

        s32 userId = rc::tryCalcControlUserIdFromPortNum(this, port);

        if (userId < 0) {
            continue;
        }

        bool isDead = rc::isDeadControlUserInStage(this, userId);

        if (isConnected & !isDead) {
            if ((al::isPadTypeJoyLeft(port) && al::isPadTriggerMinus(port)) ||
                al::isPadTriggerPlus(port)) {
                if (pPort != nullptr) {
                    *pPort = port;
                }

                return true;
            }
        }
    }

    return false;
}

/**
 * Shows the pause menu and applies its decision.
 */
void StageScene::exePause() {
    s32 port;

    if (al::isFirstStep(this)) {
        alAudioSystemFunction::pauseSystem(mAudioDirector, this, true, 0);
        mPauseMenu->appear(mPausePort);
        updateLayout();
        al::pausePadRumble(this);
        rc::pauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder(), false);
    }

    updatePauseLayout();

    if (!al::isPadConnected(mPausePort)) {
        if (isPadConnectedAny() || rc::getActiveControlUserNum(this) >= 2) {
            mPauseMenu->forceExit();

            auto* guideGameWindow =
                al::getSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);

            if (guideGameWindow != nullptr) {
                guideGameWindow->endHide(this);
            }

            endLayoutPause();
            updateLayout();

            if (mPauseMenu->isAlive()) {
                return;
            }

            mScreenCaptureExecutor->offDraw(1);
            al::endPausePadRumble(this);
            rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
            al::setNerve(this, &NrvStageScenePlay);
            alAudioSystemFunction::pauseSystem(mAudioDirector, this, false, 0);
            return;
        }

        rc::forceControllerApplet();
    }

    if (isTriggerPause(&port) && port == mPausePort) {
        if (mPauseMenu->isWait()) {
            mPauseMenu->decideBack();
        }
    } else if (mPauseMenu->isDecideGameChange()) {
        if (GameDataFunction::isStageContinuousMysteryBox(GameDataHolderAccessor(mGameDataHolder),
                                                          mCourseId)) {
            al::stopAllSequenceBgm(this, 60);
        }

        mScreenCaptureExecutor->offDraw(1);
        endLayoutPause();
        al::setNerve(this, &NrvStageSceneGameChange);
        return;
    } else if (mPauseMenu->isEndLoad()) {
        if (GameDataFunction::isStageContinuousMysteryBox(GameDataHolderAccessor(mGameDataHolder),
                                                          mCourseId)) {
            al::stopAllSequenceBgm(this, 60);
        }

        al::setNerve(this, &NrvStageSceneLoadGame);
        return;
    } else if (mPauseMenu->isDecideMap()) {
        if (GameDataFunction::isStageContinuousMysteryBox(GameDataHolderAccessor(mGameDataHolder),
                                                          mCourseId)) {
            al::stopAllSequenceBgm(this, 60);
        }

        mIsWaitEffectBeforeKill = true;
        al::setNerve(this, &NrvStageSceneRetire);
        return;
    } else if (mPauseMenu->isDecideReenterStage()) {
        GameDataFunction::resetCheckpointPass(GameDataHolderWriter(this));

        if (GameDataFunction::isStageContinuousMysteryBox(GameDataHolderAccessor(mGameDataHolder),
                                                          mCourseId)) {
            al::stopAllSequenceBgm(this, 60);
        }

        mGameDataHolder->getStageDataHolderPtr()->resetStockItems();
        s32 bestScoreUserId = GameDataFunction::tryGetLastStageBestScoreUserID(
            GameDataHolderAccessor(mGameDataHolder));
        GameDataFunction::resetStageScore(GameDataHolderWriter(mGameDataHolder));

        if (bestScoreUserId != -1) {
            GameDataFunction::setBestScoreUserId(GameDataHolderWriter(mGameDataHolder),
                                                 bestScoreUserId);
        }

        if (mStartEvent != nullptr &&
            al::isEqualString(mStartEvent->getName(), "キノピオ探検隊スタートデモ")) {
            GameDataFunction::setSkipStartDemo(GameDataHolderWriter(this), true);
        }

        mIsWaitEffectBeforeKill = true;
        al::setNerve(this, &NrvStageSceneReenterStage);
        return;
    }

    if (!mPauseMenu->isEnd()) {
        return;
    }

    auto* guideGameWindow = al::getSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);

    if (guideGameWindow != nullptr) {
        guideGameWindow->endHide(this);
    }

    endLayoutPause();
    mScreenCaptureExecutor->offDraw(1);
    mPauseMenu->kill();
    updateLayout();
    al::endPausePadRumble(this);
    rc::endPauseAllPlayerAmiiboDirector(mLiveActorKit->getPlayerHolder());
    mPauseMenu->isDecideBack();
    al::setNerve(this, &NrvStageScenePlay);
    alAudioSystemFunction::pauseSystem(mAudioDirector, this, false, 0);
}

/**
 * Starts the layouts of the pause (nothing to do).
 */
void StageScene::startLayoutPause() {}

/**
 * Updates the pad rumble and the layouts while the scene is paused.
 */
void StageScene::updatePauseLayout() {
    mLiveActorKit->updatePadRumble();
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Ends the pause of the layouts and of the players.
 */
void StageScene::endLayoutPause() {
    mStageSceneLayout->endPause();

    if (!mIsKinopioBrigade) {
        mPlayerAliveWatcher->endPause();
    }

    auto* superbViewAreaHolder =
        al::getSceneObj<SuperbViewAreaHolder>(this, SceneObjID_SuperbViewAreaHolder);

    if (superbViewAreaHolder != nullptr) {
        superbViewAreaHolder->endPause();
    }
}

/**
 * Leaves the stage once the effects ended.
 */
void StageScene::exeRetire() {
    if (mIsWaitEffectBeforeKill && !al::isGreaterStep(this, 20)) {
        al::updateEffectSystem(this);
        return;
    }

    kill();
}

/**
 * Enters the stage again once the effects ended.
 */
void StageScene::exeReenterStage() {
    if (mIsWaitEffectBeforeKill && !al::isGreaterStep(this, 20)) {
        al::updateEffectSystem(this);
        return;
    }

    kill();
}

/**
 * Plays the miss demo, then restarts the stage.
 */
void StageScene::exeRestart() {
    updateMissDemo(true, false);

    if (al::isFirstStep(this)) {
        alSeFunction::stopAllSeWithExceptList(mAudioDirector, "ミス", 0);
        alSeFunction::setIsStateAfterGoal(mAudioDirector, true);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ終了", 30,
                                                    false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "ステージ終了", 30,
                                                    false);
        mPlayerAliveWatcher->onGameOver();
        mStageSceneLayout->kill();

        if (!al::isNerve(this, &NrvStageSceneRestartMiss)) {
            mMissLayout->appear();
        }

        al::stopAllBgm(this, -1);
        al::disableBgmStart(this);
        al::changeAudioEffect(this, nullptr);
        s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());

        for (s32 i = 0; i < playerNum; i++) {
            al::setCameraCalcTargetFlag(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i),
                                        false);
        }
    }

    if (al::isStep(this, 28)) {
        al::startSequenceBgm(this, "Miss", -1, 0);
    }

    if (al::isStep(this, 119)) {
        mMissWipe->startClose(-1);
    }

    if (al::isStep(this, 190)) {
        kill();
    }
}

/**
 * Updates the scene while the miss demo plays.
 * @param isUpdateGraphics Whether the graphics are updated.
 * @param isSkipVehicle Whether the vehicles are not updated.
 */
void StageScene::updateMissDemo(bool isUpdateGraphics, bool isSkipVehicle) {
    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();
    mLiveActorKit->getCameraDirector()->update(false);

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

    if (al::isLessEqualStep(this, 30)) {
        mPlayerAliveWatcher->update();
        al::updateKitList(this, "２Ｄ");
    } else {
        s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());

        for (s32 i = 0; i < playerNum; i++) {
            al::setCameraCalcTargetFlag(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i),
                                        false);
        }
    }

    al::updateKitList(this, "２Ｄ（ポーズ無視）");

    if (isUpdateGraphics) {
        mLiveActorKit->updateGraphics(false);
    }
}

/**
 * Plays the game over.
 */
void StageScene::exeGameOver() {
    if (al::isFirstStep(this)) {
        alSeFunction::setIsStateAfterGoal(mAudioDirector, true);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "ステージ終了", 30,
                                                    false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "ステージ終了", 30,
                                                    false);
        mStageSceneLayout->kill();
        mPlayerAliveWatcher->onGameOver();
        mPlayerAliveWatcher->startDemo();
        mCameraChangeLayout->disappearCameraChangeLayout();
        mGhostPlayerDirector->tryEndPlay();
        s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());

        for (s32 i = 0; i < playerNum; i++) {
            al::setCameraCalcTargetFlag(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i),
                                        false);
        }
    }

    updateMissDemo(false, mStateGameOver->isDemo());
    bool isEnd = al::updateNerveState(this);
    mLiveActorKit->updateGraphics(false);

    if (isEnd) {
        if (!mStateGameOver->isContinue()) {
            al::setNerve(this, &NrvStageSceneGameEnd);
        }

        kill();
    }
}

/**
 * Waits for the scene to end after the game over (nothing to do).
 */
void StageScene::exeGameEnd() {}

/**
 * Plays the miss demo of the players killed when the time is up.
 */
void StageScene::exePreTimeUp() {
    updateMissDemo(true, false);
    mDrcAssistDirectorList->disappearTouchPointerImmediately();

    if (GameDataFunction::isGameOver(GameDataHolderAccessor(mGameDataHolder))) {
        al::setNerve(this, &NrvStageSceneGameOver);
    } else {
        al::setNerve(this, &NrvStageSceneRestartMiss);
    }
}

/**
 * Restarts the stage after the time is up.
 */
void StageScene::exeTimeUp() {
    exeRestart();
}

/**
 * Ends the scene when the time of a mystery box course is up.
 */
void StageScene::exeMysteryBoxTimeUp() {
    kill();
}

/**
 * Warps the players to another world once the effects ended.
 */
void StageScene::exeWorldWarp() {
    if (al::isFirstStep(this)) {
        if (!mIsKinopioBrigade) {
            recordClearData();
        }

        if (isStageSequenceBgm(mGameDataHolder, mCourseId)) {
            al::stopAllSequenceBgm(this, 60);
        }
    }

    if (!al::isStep(this, 31) && mIsWaitEffectBeforeKill) {
        al::updateEffectSystem(this);
        return;
    }

    kill();
}

/**
 * Shows the illust item result of a fairy house.
 */
void StageScene::exeIllustItemResult() {
    if (al::isFirstStep(this)) {
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "土管入り", 60,
                                                    false);
    }

    al::updateKitList(this, "監視オブジェ");
    al::updateKitList(this, "２Ｄ");
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectDemo(this);
    mAudioDirector->update();

    if (al::isDead(mIllustItemWatcher)) {
        mScreenCaptureExecutor->offDraw(1);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 60, false);
        al::setNerve(this, &NrvStageScenePlay);
    }
}

/**
 * Runs the snapshot mode and restores the camera mode afterwards.
 */
void StageScene::exeCaptureMode() {
    if (al::isFirstStep(this)) {
        mCameraModeBeforeCapture = mLiveActorKit->getCameraDirector()->getCameraMode();
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvStageScenePlay);
        mLiveActorKit->getCameraDirector()->setCameraMode(mCameraModeBeforeCapture);
    }
}

/**
 * Places the checkpoint flag of the stage.
 * @param rInfo The actor init info.
 */
void StageScene::initPlacementCheckpoint(const al::ActorInitInfo& rInfo) {
    ProjectActorFactory factory;
    al::LiveActor* checkpointFlag =
        al::tryInitPlacementSingleObject(this, rInfo, 0, "CheckPointList", factory);

    if (checkpointFlag != nullptr) {
        mCheckpointFlag = static_cast<CheckpointFlag*>(checkpointFlag);
    }
}

/**
 * Creates the players of every character for each player placement of the stage.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 * @param pSelector The retargetting selector of the players.
 */
void StageScene::initPlacementPlayer(const al::StageInfo* pStageInfo,
                                     const al::ActorInitInfo& rInfo,
                                     PlayerRetargettingSelector* pSelector) {
    al::PlacementInfo placementInfo;
    s32 count = 0;
    al::tryGetPlacementInfoAndCount(&placementInfo, &count, pStageInfo, "PlayerList");
    static_cast<PlayerGroupSceneObj*>(al::createSceneObj(this, SceneObjID_PlayerGroup))
        ->initGroup(128);
    al::setSceneObj(this, new PlayerProcess(mLiveActorKit->getPlayerHolder()),
                    SceneObjID_PlayerProcess);

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo info;
        al::getPlacementInfoByIndex(&info, placementInfo, i);
        al::ActorInitInfo actorInfo;

        if (GameDataFunction::isCheckpointPass(this) &&
            mCheckpointFlag != nullptr) {
            actorInfo = *mCheckpointFlag->getPlayerInfo();
        } else {
            actorInfo.initNoViewId(&info, rInfo);
        }

        mIsNarrowPlace = false;
        al::tryGetArg(&mIsNarrowPlace, actorInfo, "IsNarrowPlace");

        if (mIsKinopioBrigade) {
            mLiveActorKit->getCameraDirector()->setSingleJoyconCameraValid(true);

            for (s32 j = 0; j < 4; j++) {
                const sead::Matrix34f* viewMtx = &mLiveActorKit->getCameraDirector()->mMainViewMtx;
                auto* player = new PlayerActor(viewMtx);
                PlayerActionGraphBuilderKinopioBrigade builder;
                player->initSpecial(actorInfo, 0, "KinopioBrigade", pSelector, pSelector, &builder,
                                    "プレイヤー", cPlayerType, j, nullptr);
                rc::initPlayerFigureType(player, 0, false);
                rc::deactivatePlayer(player);
                rc::setMainPlayerActor(player);
                alPlayerFunction::registerPlayer(player, player->getPadRumbleKeeper(), true);
                PlayerStockerFunction::registerPlacementPlayer(this, player, j);
                player->setEnableSingleJoyCamera(true);
                mPlayers.pushBack(player);
                player->setInvincibleBgmController(mInvincibleBgmController);
                player->setBigBgmController(mBigBgmController);
            }
        } else {
            s32 doubleMarioNum =
                GameDataFunction::getDoubleMarioNumMax(GameDataHolderAccessor(mGameDataHolder));
            mLiveActorKit->getCameraDirector()->setSingleJoyconCameraValid(false);

            for (s32 j = 0; j < rc::getPlayerCharacterNumMax(); j++) {
                al::ActorInitInfo playerInfo;
                playerInfo.initNoViewId(&info, rInfo);
                PlayerActor* player =
                    createPlayer(actorInfo, &mLiveActorKit->getCameraDirector()->mMainViewMtx,
                                 rc::getPlayerCharacterName(j), pSelector,
                                 doubleMarioNum > 0 ? cPlayerTypeDoubleMario : cPlayerType);
                rc::deactivatePlayer(player);
                player->setEnableSingleJoyCamera(false);
                alPlayerFunction::registerPlayer(player, player->getPadRumbleKeeper(), true);
                PlayerStockerFunction::registerPlacementPlayer(this, player, j);
                mPlayers.pushBack(player);
                player->setInvincibleBgmController(mInvincibleBgmController);
                player->setBigBgmController(mBigBgmController);
                player->setKoopaJr(nullptr);
            }

            mPlayerCrown = new PlayerCrown(actorInfo, nullptr);
        }
    }
}

/**
 * Places the goals of the stage listed in its GoalList.
 * @param rInfo The actor init info.
 */
void StageScene::initPlacementGoal(const al::ActorInitInfo& rInfo) {
    for (s32 i = 0; i < al::getStageInfoMapNum(this); i++) {
        al::PlacementInfo placementInfo;
        s32 count = 0;
        al::getPlacementInfoAndCount(&placementInfo, &count, al::getStageInfoMap(this, i),
                                     "GoalList");
        ProjectActorFactory factory;

        for (s32 j = 0; j < count; j++) {
            al::PlacementInfo info;
            al::getPlacementInfoByIndex(&info, placementInfo, j);
            al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &info);

            if (actor == nullptr) {
                continue;
            }

            const char* objectName = nullptr;
            al::getObjectName(&objectName, info);

            if (al::isEqualString(objectName, "KinopioBrigadeChecker") ||
                al::isEqualString(objectName, "GoalDoor")) {
                mGoalObjHolder->pushBack(getGoalObj(actor));
            } else if (al::isEqualString(objectName, "GoalDoorForFairyHouse")) {
                mGoalObjHolder->pushBack(getGoalObj(actor));
                mIsTimerDisabled = true;
            } else if (al::isEqualString(objectName, "GateKeeperChecker")) {
                mGoalObjHolder->pushBack(getGoalObj(actor));
            } else if (al::isEqualString(objectName, "GoalBonusGameBlockSlot")) {
                mGoalObjHolder->pushBack(getGoalObj(actor));
                mIsTimerDisabled = true;
            } else if (al::isEqualString(objectName, "MysteryHouseChecker")) {
                mMysteryHouseChecker = static_cast<MysteryHouseChecker*>(actor);
                mGoalObjHolder->pushBack(getGoalObj(actor));
                mIsTimerDisabled = true;
            } else if (al::isEqualString(objectName, "DokanWorldWarp")) {
                mDokanWorldWarp = static_cast<DokanWorldWarp*>(actor);
                mGoalObjHolder->pushBack(getGoalObj(actor));
            } else if (al::isEqualString(objectName, "GoalPole") ||
                       al::isEqualString(objectName, "GoalPoleSuper") ||
                       al::isEqualString(objectName, "GoalPoleRunaway") ||
                       al::isEqualString(objectName, "GoalPoleLast")) {
                mGoalPole = static_cast<GoalPole*>(actor);
                mGoalObjHolder->pushBack(mGoalPole);
                mLiveActorKit->getCameraDirector()->setGoalPosPtr(al::getTransPtr(actor));
            } else if (al::isEqualString(objectName, "BlockChoiceWatcher")) {
                mBlockChoiceWatcher = static_cast<BlockChoiceWatcher*>(actor);
                mIsTimerDisabled = true;
            } else if (al::isEqualString(objectName, "FairyHouseIllustItemWatcher")) {
                mIllustItemWatcher = static_cast<FairyHouseIllustItemWatcher*>(actor);
                mIllustItemWatcher->setStampDirector(mStampDirector);
            }
        }
    }
}

/**
 * Creates the actors listed in a placement list of a stage.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 * @param pListName The placement list name (unused, ObjectList is always used).
 */
void StageScene::initPlacementObject(const al::StageInfo* pStageInfo,
                                     const al::ActorInitInfo& rInfo, const char* pListName) {
    al::PlacementInfo placementInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&placementInfo, &count, pStageInfo, "ObjectList");
    ProjectActorFactory factory;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo info;
        al::getPlacementInfoByIndex(&info, placementInfo, i);
        const char* objectName = nullptr;
        al::getObjectName(&objectName, info);
        al::createPlacementActorFromFactory(factory, rInfo, &info);
    }
}

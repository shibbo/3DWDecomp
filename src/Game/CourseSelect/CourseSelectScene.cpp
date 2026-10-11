#include "CourseSelect/CourseSelectScene.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include <math/seadMatrix.h>
#include <nerd/nerdMath.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "AreaObj/WorldClipArea.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectFairy.hpp"
#include "CourseSelect/CourseSelectFunction.hpp"
#include "CourseSelect/CourseSelectPlayerActor.hpp"
#include "CourseSelect/CourseSelectRoad.hpp"
#include "CourseSelect/CourseSelectStateAfterEndingEvent.hpp"
#include "CourseSelect/CourseSelectStateClearDemo.hpp"
#include "CourseSelect/CourseSelectStateRevivePlayer.hpp"
#include "CourseSelect/CourseSelectStateStageEnter.hpp"
#include "CourseSelect/CourseSelectStateStageExit.hpp"
#include "CourseSelect/CourseSelectStateWorldStartDemo.hpp"
#include "CourseSelect/CourseSelectWindowHolder.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Layout/CounterGreenStarAppend.hpp"
#include "Layout/CourseSelectLayout.hpp"
#include "Layout/CourseSelectSceneLayout.hpp"
#include "Layout/ListClearStar.hpp"
#include "Layout/ListStamp.hpp"
#include "Layout/ListStampResult.hpp"
#include "Layout/MapMenu.hpp"
#include "Layout/PauseMenuMap.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/Switch/CourseSelectMenuHeader.hpp"
#include "Layout/Switch/SnapshotLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/AudioDirector.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Project/Camera/Info/SceneCameraControlInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Library/Clipping/ClippingDirectorBase.hpp"
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
#include "Library/Obj/FootPrintServer.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "MapObj/CoinRotater.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcAssistDirectorList.hpp"
#include "MapObj/ScoreHolder.hpp"
#include "MapObj/StampDirector.hpp"
#include "Player/FurEnv.hpp"
#include "Player/Giga/PlayerActionGraphBuilderCourseSelect.hpp"
#include "Player/IUsePlayerPanicControl.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGroupSceneObj.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Clipping/ClippingFarAreaObserver.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/DemoOpeningState.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/ProjectActorFactoryTypes.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "Scene/SnapshotState.hpp"
#include "Sequence/ProductStageStartParam.hpp"
#include "System/Application.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace nn::hid {
void StartLrAssignmentMode();
void StopLrAssignmentMode();
}  // namespace nn::hid

/**
 * Declares a CourseSelectScene nerve whose execute function has a different name than the nerve.
 * @param Action The nerve name.
 * @param Func The CourseSelectScene::exe* function the nerve runs.
 */
#define COURSE_SELECT_SCENE_NERVE(Action, Func)                                                    \
    class CourseSelectSceneNrv##Action : public al::Nerve {                                        \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<CourseSelectScene>()->exe##Func();                                  \
        }                                                                                          \
    };

namespace {
NERVE_DECL(CourseSelectScene, Snapshot)
NERVE_DECL(CourseSelectScene, Opening)
NERVE_DECL(CourseSelectScene, AfterEndingEvent)
NERVE_DECL(CourseSelectScene, StageExit)
NERVE_DECL(CourseSelectScene, ClearDemo)
NERVE_DECL(CourseSelectScene, Play)
NERVE_DECL(CourseSelectScene, StageEnter)
NERVE_DECL(CourseSelectScene, WorldStartDemo)
NERVE_DECL(CourseSelectScene, RevivePlayer)
NERVE_DECL(CourseSelectScene, Save)
COURSE_SELECT_SCENE_NERVE(SaveGotoTitle, Save)
COURSE_SELECT_SCENE_NERVE(SaveNoWindow, Save)
NERVE_DECL(CourseSelectScene, MapPauseEnd)
NERVE_DECL(CourseSelectScene, GotoTitle)
NERVE_DECL(CourseSelectScene, LoadGame)
NERVE_DECL(CourseSelectScene, GreenStarInfo)
NERVE_DECL(CourseSelectScene, StampInfo)
NERVE_DECL(CourseSelectScene, Pause)
NERVE_DECL(CourseSelectScene, MapPause)
COURSE_SELECT_SCENE_NERVE(MapPauseReturn, MapPause)
NERVE_DECL(CourseSelectScene, StampList)
NERVE_DECL(CourseSelectScene, StarList)
NERVE_DECL(CourseSelectScene, EventGateKeeper)
NERVE_DECL(CourseSelectScene, MapPauseStart)

/**
 * Panic control keeping the players panicking for the whole course select (before the first
 * course was cleared).
 */
class PanicControlAlways : public IUsePlayerPanicControl {
public:
    /**
     * Gets whether the players panic.
     * @return Always true.
     */
    bool isPanic() override { return true; }
};

/**
 * Panic control keeping the players panicking until they leave the world of Bowser's castle
 * (right after it was cleared for the first time).
 */
class PanicControlKoopaCastleClear : public IUsePlayerPanicControl {
public:
    /**
     * Constructs the panic control.
     * @param pDirector The course select director.
     */
    explicit PanicControlKoopaCastleClear(CourseSelectDirector* pDirector)
        : mDirector(pDirector) {}

    /**
     * Gets whether the players panic. Turns the panic off for good once the main player is in
     * another world.
     * @return true while the players panic.
     */
    bool isPanic() override {
        if (!mIsPanic) {
            return false;
        }

        s32 worldId = mDirector->getActiveWorldId();
        if (worldId != -1 && worldId != cKoopaCastleWorldId) {
            mIsPanic = false;
            return false;
        }

        return true;
    }

private:
    /** World of the Bowser's castle whose first clear makes the players panic. */
    static constexpr s32 cKoopaCastleWorldId = 7;

    CourseSelectDirector* mDirector;
    bool mIsPanic = true;
};

NERVE_DECL(CourseSelectScene, PauseStart)
NERVES_MAKE_NOSTRUCT(CourseSelectScene, Snapshot, Opening, AfterEndingEvent, StageExit, ClearDemo,
                     Play, StageEnter, WorldStartDemo, RevivePlayer, Save, SaveGotoTitle,
                     SaveNoWindow, MapPauseEnd, GotoTitle, LoadGame, GreenStarInfo, StampInfo,
                     Pause, MapPause, MapPauseReturn, StampList, StarList, EventGateKeeper,
                     MapPauseStart, PauseStart)

/** Number of the worlds shown on the course select map (the special worlds come after). */
constexpr s32 cNormalWorldNum = 8;
/** Frames a disconnected control user may idle before leaving the game. */
constexpr s32 cLeaveIdleFrame = 60;
/** Height under which the main player is put back to the start of the world. */
constexpr f32 cFallLimitY = -1500.0f;
/** Distance between two players placed side by side. */
constexpr f32 cPlayerPlacementInterval = 150.0f;

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
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
 * Gets the nerve following the stage exit demo and the result windows.
 * @param pScene The course select scene.
 * @return The next nerve.
 */
const al::Nerve* getNextNerveAfterStageExit(CourseSelectScene* pScene) {
    GameDataHolder* holder = pScene->getGameDataHolder();

    if (al::isNerve(pScene, &NrvCourseSelectSceneStageExit) &&
        GameDataFunction::getLastTotalAcquireGreenStarNum(GameDataHolderAccessor(holder)) !=
            GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor(holder))) {
        return &NrvCourseSelectSceneGreenStarInfo;
    }

    if ((al::isNerve(pScene, &NrvCourseSelectSceneStageExit) ||
         al::isNerve(pScene, &NrvCourseSelectSceneGreenStarInfo)) &&
        GameDataFunction::isLastPlayCourseClearWorldWarp(GameDataHolderAccessor(holder))) {
        return &NrvCourseSelectSceneStampInfo;
    }

    alSeFunction::setRequestKeeperVolumeSetting(pScene->mAudioDirector, "メイン", "通常", 60,
                                                false);
    s32 courseId = GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(holder));
    if (pScene->getCourseSelectDirector()->findMiniatureObj(courseId)->isNeedClearDemo()) {
        return &NrvCourseSelectSceneClearDemo;
    }

    return &NrvCourseSelectSceneRevivePlayer;
}
}  // namespace

/**
 * Constructs the course select scene.
 * @param isAfterEndingEvent Whether the scene shows the event played after the ending.
 */
CourseSelectScene::CourseSelectScene(bool isAfterEndingEvent)
    : al::Scene("コース選択シーン"), mIsAfterEndingEvent(isAfterEndingEvent) {
    for (s32 i = 0; i < cControlUserNum; i++) {
        mIdleFrames[i] = 0;
    }
}

/**
 * Destroys the course select scene.
 */
CourseSelectScene::~CourseSelectScene() {
    if (al::isNerve(this, &NrvCourseSelectSceneSnapshot)) {
        mSnapshotState->kill();
    }

    rc::setMainPlayerActor(nullptr);
    mLiveActorKit->getEffectSystem()->endScene();
    getFramework()->mIsClearRenderBuffer = true;

    if (mStampDirector != nullptr) {
        delete mStampDirector;
    }

    mStampDirector = nullptr;
}

/**
 * Initializes the scene: save data, kits, layouts, placement, states and the first nerve.
 * @param rInfo The scene init info.
 */
void CourseSelectScene::init(const al::SceneInitInfo& rInfo) {
    rInfo.mGameSystemInfo->getGamePadSystem()->changeMultiPlayMode(4, 1);
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    GameDataFunction::updateClearStarLevel(GameDataHolderAccessor(mGameDataHolder));
    bool isAfterEnding =
        GameDataFlagFunction::isAfterEnding(GameDataHolderAccessor(mGameDataHolder));
    mStageName = rInfo.mStageName;
    initSceneStopCtrl();
    initScreenCoverCtrl();
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    al::createSceneObj(this, SceneObjID_ScoreHolder);
    al::getSceneObj<ScoreHolder>(this, SceneObjID_ScoreHolder)
        ->setStageDataHolder(mGameDataHolder->getStageDataHolderPtr());
    al::setSceneObj(this, mGameDataHolder, SceneObjID_GameDataHolder);

    mDirector = new CourseSelectDirector(this);
    al::setSceneObj(this, mDirector, SceneObjID_CourseSelectDirector);
    al::createSceneObj(this, SceneObjID_ControllerEventWatcher);

    mDrcAssistDirectorList = new DrcAssistDirectorList(4);
    al::setSceneObj(this, mDrcAssistDirectorList, SceneObjID_DrcAssistDirectorList);
    mDrcAssistDirectorList->addTouchAssist(al::getPlayerControllerPort(0), false);
    mDrcAssistDirectorList->addTouchAssist(al::getPlayerControllerPort(1), false);
    mDrcAssistDirectorList->addTouchAssist(al::getPlayerControllerPort(2), false);
    mDrcAssistDirectorList->addTouchAssist(al::getPlayerControllerPort(3), false);
    mDrcAssistDirectorList->setEnable(false);

    mMainViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(6));
    mSubViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(9));
    initSceneAudio(rInfo, mStageName.cstr(), 60, 30, 1, false, "Scene", 20, 1.0f);

    {
        al::GraphicsInitArg graphicsArg;
        graphicsArg.mViewRendererCreator = new al::ViewRendererCreator();
        graphicsArg.setViewNum(2);
        graphicsArg.mIsUsingViewRenderer = true;
        initLiveActorKitWithGraphics(graphicsArg, rInfo, 5120, rc::getControlUserNumMax() * 2, 2,
                                     0, false, false);
    }

    mLiveActorKit->initHitSensorDirector(1, false);

    al::LiveActorKit* kit = mLiveActorKit;
    kit->mDemoDirector = new ProjectDemoDirector(kit->getPlayerHolder(), -1);
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    mWindowHolder = new CourseSelectWindowHolder(this);
    mWindowHolder->init(layoutInfo);
    al::initItemDirector(this, new ProjectItemDirector(layoutInfo, mGameDataHolder,
                                                       mLiveActorKit->getPlayerHolder(),
                                                       mLiveActorKit->getAreaObjDirector()));
    mLiveActorKit->getCameraDirector()->setCameraAspect(mMainViewport, mSubViewport);
    mLiveActorKit->getCameraDirector()->setStageName(mStageName.cstr());
    mLiveActorKit->getCameraDirector()->setReverseHorizontal(
        GameDataFunction::getCameraReverseHorizontal(GameDataHolderAccessor(this)));
    mLiveActorKit->getCameraDirector()->setReverseVertical(
        GameDataFunction::getCameraReverseVertical(GameDataHolderAccessor(this)));

    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    sead::LookAtCamera* camera = cameraDirector->getLookAtCamera();
    initSceneAudio3D(rInfo, &camera->getPos(), &camera->getMatrix(),
                     cameraDirector->getProjection(), &camera->getAt(), "コースセレクト用",
                     mLiveActorKit->getAreaObjDirector(), false);
    mAudioDirector->setPlayerHolder(mLiveActorKit->getPlayerHolder());
    mAudioDirector->setDefaultBgmPlayName(nullptr);
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());

    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    initAudioKeeper("CourseSelectScene");
    static_cast<ProjectItemDirector*>(mLiveActorKit->getItemDirector())
        ->createItemHolder(actorInfo, true);
    al::setSceneObj(this, new CoinRotater(mLiveActorKit->getExecuteDirector()), 1);
    al::setSceneObj(this, new al::FootPrintServer(actorInfo, "FootPrint", 32), 7);
    initPlacement(actorInfo);
    initSceneAudioAfterInitPlacement(rInfo);
    mDirector->createLayout(layoutInfo, mWindowHolder, rInfo.mGameSystemInfo->getNetworkSystem(),
                            nullptr, nullptr);
    mDirector->setWorldId(mWorldId);
    mScreenCaptureExecutor = rInfo.mScreenCaptureExecutor;
    mLiveActorKit->getCameraDirector()->init(mLiveActorKit->getPlayerHolder());
    mLiveActorKit->getCameraDirector()->initAudioKeeper(actorInfo);

    if (mMainPlayer != nullptr) {
        al::invalidUserCameraControl(mMainPlayer);
    }

    mDirector->initAfterPlacement();

    if (!mGameDataHolder->isCourseSelectVisited()) {
        mGameDataHolder->setCourseSelectVisited(true);
    }

    if (mGameDataHolder->isFirstPlay()) {
        initNerve(&NrvCourseSelectSceneOpening, 11);

        s32 courseId =
            GameDataFunction::calcCourseId(GameDataHolderAccessor(mGameDataHolder), 1, 1);
        const sead::Vector3f& trans = al::getTrans(mDirector->tryFindMiniatureObj(courseId));
        // Default camera matrices of the opening demo (high above the map) and the matrix of
        // the first course.
        sead::Matrix34f demoMtx[3] = {
            {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 10000.0f, 0.0f, 0.0f, 1.0f, 0.0f},
            {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 10000.0f, 0.0f, 0.0f, 1.0f, 0.0f},
            {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        };
        demoMtx[2].setTranslation(trans);

        DemoOpeningSwitch* openingSwitch = mDirector->getDemoOpeningSwitch();
        if (openingSwitch != nullptr) {
            sead::Matrix34f baseMtx = *openingSwitch->getBaseMtx();
            demoMtx[0] = baseMtx;
            demoMtx[1] = baseMtx;
        }

        mOpeningState = new DemoOpeningState(this, actorInfo,
                                             mLiveActorKit->getGraphicsSystemInfo(), demoMtx);
        mDirector->setAfterOpeningDemo();
        mDirector->initPlayerPositionOpening("");
        al::initNerveState(this, mOpeningState, &NrvCourseSelectSceneOpening, "OpeningDemo");
        mDirector->startDemo(true);
    } else if (mIsAfterEndingEvent) {
        initNerve(&NrvCourseSelectSceneAfterEndingEvent, 10);
    } else {
        s32 courseId =
            GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(mGameDataHolder));
        if (!GameDataFunction::isInvalidCourseId(courseId) && !mGameDataHolder->isSaveDataRead()) {
            initNerve(&NrvCourseSelectSceneStageExit, 10);
        } else if (GameDataFlagFunction::isNewShowAfterGameOverInfo(
                       GameDataHolderAccessor(mGameDataHolder))) {
            initNerve(&NrvCourseSelectSceneClearDemo, 10);
        } else {
            initNerve(&NrvCourseSelectScenePlay, 10);
        }
    }

    mGameDataHolder->setSaveDataRead(false);

    al::SceneCameraInfo* cameraInfo = mLiveActorKit->getCameraDirector()->getSceneCameraInfo();
    mAfterEndingEventState = new CourseSelectStateAfterEndingEvent(this, mDirector);
    mStageEnterState = new CourseSelectStateStageEnter(
        this, layoutInfo, mLiveActorKit->getCameraDirector()->getSceneCameraInfo(),
        rInfo.mGameSystemInfo);
    mStageExitState = new CourseSelectStateStageExit(
        this, mLiveActorKit->getCameraDirector()->getSceneCameraInfo());
    mClearDemoState = new CourseSelectStateClearDemo(
        this, mLiveActorKit->getPlayerHolder(),
        mLiveActorKit->getCameraDirector()->getSceneCameraInfo(), layoutInfo);
    mWorldStartDemoState = new CourseSelectStateWorldStartDemo(
        this, actorInfo, mLiveActorKit->getCameraDirector()->getSceneCameraInfo());
    mRevivePlayerState = new CourseSelectStateRevivePlayer(this, isAfterEnding);
    al::initNerveState(this, mAfterEndingEventState, &NrvCourseSelectSceneAfterEndingEvent,
                       "AfterEndingEvent");
    al::initNerveState(this, mStageEnterState, &NrvCourseSelectSceneStageEnter, "StageEnter");
    al::initNerveState(this, mStageExitState, &NrvCourseSelectSceneStageExit, "StageExit");
    al::initNerveState(this, mClearDemoState, &NrvCourseSelectSceneClearDemo, "ClearDemo");
    al::initNerveState(this, mWorldStartDemoState, &NrvCourseSelectSceneWorldStartDemo,
                       "WorldStartDemo");
    al::initNerveState(this, mRevivePlayerState, &NrvCourseSelectSceneRevivePlayer,
                       "RevivePlayer");

    mPauseMenuMap = new PauseMenuMap(layoutInfo, mGameDataHolder, mDirector,
                                     mLiveActorKit->getCameraDirector(),
                                     mLiveActorKit->getPlayerHolder(), mControlGuideBar,
                                     mScreenCaptureExecutor);
    mMapMenu = new MapMenu(layoutInfo, mGameDataHolder,
                           mLiveActorKit->getCameraDirector()->getSceneCameraInfo(), mDirector,
                           mLiveActorKit->getGraphicsSystemInfo(), mControlGuideBar);
    mListClearStar = new ListClearStar(layoutInfo, mGameDataHolder);
    mListStamp = new ListStamp(layoutInfo, mGameDataHolder);
    mMenuHeader = new CourseSelectMenuHeader(layoutInfo);
    mControlGuideBar = new RCSControlGuideBar(layoutInfo);
    mControlGuideBar->hide();
    mMapMenu->setControlGuideBar(mControlGuideBar);
    mPauseMenuMap->setControlGuideBar(mControlGuideBar);
    mCounterGreenStar = new CounterGreenStarAppend(layoutInfo);
    mListStampResult = new ListStampResult(layoutInfo, mGameDataHolder);

    mStampDirector = new rc::StampDirector(
        actorInfo, "Stamp",
        mDrcAssistDirectorList->getDrcAssist(al::getMainControllerPort())->getTouchAssistInfo(), 30,
        -1);
    mStampDirector->setUseNoCodeWallFilter(true);
    mDrcAssistDirectorList->setStampDirector(mStampDirector);
    al::setSceneObj(this, mStampDirector, SceneObjID_StampDirector);
    mSnapshotLayout = new SnapshotLayout(layoutInfo, mStampDirector);
    mLiveActorKit->getCameraDirector()->setSnapShotAudioKeeper(mSnapshotLayout);
    mSnapshotState = new SnapshotState(mSnapshotLayout, this, mDrcAssistDirectorList,
                                       mStampDirector, nullptr);
    al::initNerveState(this, mSnapshotState, &NrvCourseSelectSceneSnapshot, "SnapshotState");
    endInit(actorInfo, nullptr);
    mDirector->startDemo(false);

    if (al::isFirstStep(this)) {
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);
    }

    alAudioSystemFunction::pauseSystem(mAudioDirector, this, false, 0);
}

/**
 * Loads the stage and places the area objects, cameras, sky, players, objects and demos.
 * @param rInfo The actor init info.
 */
void CourseSelectScene::initPlacement(const al::ActorInitInfo& rInfo) {
    initAndLoadStageResource(mStageName.cstr(), 1);

    al::GraphicsSystemInfo* graphicsInfo = mLiveActorKit->getGraphicsSystemInfo();
    const al::Resource* designResource = al::tryGetStageResourceDesign(this, 0);
    graphicsInfo->initStageResource(designResource, mStageName.cstr(), mLiveActorKit, false, 0);
    al::StringTmp<256> mapStageName("%sMap", mStageName.cstr());
    mDirector->init(rInfo);
    al::initPlacementAreaObj(this, rInfo, nullptr);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
    mLiveActorKit->getCameraDirector()->initCameraCreator(al::getStageInfoMapNum(this),
                                                          al::isStageOneResource(this));

    for (s32 i = 0; i < al::getStageInfoMapNum(this); i++) {
        mLiveActorKit->getCameraDirector()->setCameraResource(
            al::getStageInfoMap(this, i)->getResource(), i);
    }

    ProjectActorFactory factory;
    al::initPlacementByStageInfo(al::getStageInfoMap(this, 0), "SkyList", factory, rInfo);
    initPlacementPlayer(al::getStageInfoMap(this, 0), rInfo);

    for (s32 i = 0; i < al::getStageInfoMapNum(this); i++) {
        initPlacementObject(al::getStageInfoMap(this, i), rInfo, "ObjectList");
    }

    for (s32 i = 0; i < al::getStageInfoDesignNum(this); i++) {
        initPlacementObject(al::getStageInfoDesign(this, i), rInfo, "ObjectList");
    }

    for (s32 i = 0; i < al::getStageInfoSoundNum(this); i++) {
        initPlacementObject(al::getStageInfoSound(this, i), rInfo, "ObjectList");
    }

    const al::StageInfo* stageInfo = al::getStageInfoMap(this, 0);
    al::PlacementInfo demoListInfo;
    s32 demoNum = 0;
    al::getPlacementInfoAndCount(&demoListInfo, &demoNum, stageInfo, "DemoObjList");

    for (s32 i = 0; i < demoNum; i++) {
        al::PlacementInfo demoInfo;
        al::getPlacementInfoByIndex(&demoInfo, demoListInfo, i);
        al::createPlacementActorFromFactory(factory, rInfo, &demoInfo);

        const char* objectName = nullptr;
        al::getObjectName(&objectName, demoInfo);
        // The result of this check is unused in the original code as well.
        al::isEqualString(objectName, "DemoOpeningSwitch");
    }

    mDirector->connectNodeLink();
    initPlacementNodeItem(rInfo);

    s32 courseId = GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(mGameDataHolder));
    if (mDirector->tryFindMiniatureObj(courseId) == nullptr) {
        return;
    }

    s32 worldId;
    s32 stageId;
    GameDataFunction::calcWorldAndStageId(GameDataHolderAccessor(mGameDataHolder), &worldId,
                                          &stageId, courseId);
    mWorldId = worldId;

    if (GameDataFlagFunction::isAfterEnding(GameDataHolderAccessor(mGameDataHolder))) {
        if (!mIsAfterEndingEvent) {
            GameDataFlagFunction::resetAfterEnding(GameDataHolderAccessor(mGameDataHolder));
        }

        mDirector->initPlayerPositionAfterEnding();
    } else {
        mDirector->initPlayerPositionStage(courseId);
    }
}

/**
 * Makes the scene appear.
 */
void CourseSelectScene::appear() {
    CourseSelectRoad::setRoadOpenSuperSpeed(false);
    al::Scene::appear();
    mDirector->appearLayout();
    getFramework()->mIsClearRenderBuffer = false;
}

/**
 * Does nothing: the scene is updated by its nerves.
 */
void CourseSelectScene::control() {}

/**
 * Updates the scene: play report, cameras, main player and control users, kits and layouts.
 */
void CourseSelectScene::update() {
    mIsUpdated = true;

    if (mGameDataHolder->isNeedCourseSelectPlayReport()) {
        mGameDataHolder->setNeedCourseSelectPlayReport(false);

        if (!SingleModeDataFunction::beginPlayReport(GameDataHolderAccessor(mGameDataHolder),
                                                     nullptr, preport::KeyEventType(2), 1, 0)) {
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      preport::Key(45), 0);
            SingleModeDataFunction::endPlayReport(GameDataHolderAccessor(mGameDataHolder));
        }

        if (!mGameDataHolder->beginPlayReport(preport::KeyEventType(0), 6, 0)) {
            GameDataHolder* holder = mGameDataHolder;
            SingleModeData* singleFile = holder->getSingleFile();
            GameDataFile* file = holder->getGameDataFile(holder->getLastPlayingFileId());
            mGameDataHolder->addSessionId();
            SingleModeDataFunction::setPlayReportData(
                GameDataHolderAccessor(mGameDataHolder), preport::Key(9),
                mGameDataHolder->getLastSingleModePlayingFileID());
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      preport::Key(10), file->getName());
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      preport::Key(11),
                                                      file->getTotalPlayTimePR(true));
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      preport::Key(12),
                                                      singleFile->getTotalPlayTimePR(false));
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      preport::Key(3),
                                                      GameDataFile::getCameraReverseHorizontal());
            SingleModeDataFunction::setPlayReportData(GameDataHolderAccessor(mGameDataHolder),
                                                      preport::Key(4),
                                                      GameDataFile::getCameraReverseVertical());
            mGameDataHolder->endPlayReport();
        }
    }

    mLiveActorKit->preDrawGraphics();
    al::getSceneObj<FurEnv>(this, SceneObjID_FurEnv)->update();

    if (al::isStopScene(this)) {
        return;
    }

    if (!isPause() && !isPauseStart()) {
        if (!al::isNerve(this, &NrvCourseSelectScenePlay) || mDirector->isPlayPuppeterDemoAll()) {
            if (mMainPlayer != nullptr) {
                al::invalidUserCameraControl(mMainPlayer);
                al::invalidZoomCameraControl(mMainPlayer);
            }
        } else if (mMainPlayer != nullptr) {
            al::validUserCameraControl(mMainPlayer);
            al::validZoomCameraControl(mMainPlayer);
        }

        s32 port = rc::getPlayerInputPort(mMainPlayer);
        mLiveActorKit->getCameraDirector()->validUserCameraControlByPortOnly(port);
        al::validZoomCameraControlByPort(mMainPlayer, port);
    }

    mLiveActorKit->getCameraDirector()->setMainPlayerIndex(
        alPlayerFunction::findPlayerHolderIndex(mMainPlayer));

    if (!isPause()) {
        mLiveActorKit->getCameraDirector()->update(false);
        mDirector->prepareFrame();

        s32 userIds[cControlUserNum];
        s32 userNum = rc::findActiveUserIdList(userIds, GameDataHolderAccessor(mGameDataHolder));
        s32 mainUserId = rc::tryFindControlUserId(mMainPlayer);
        if (mainUserId == -1) {
            mainUserId = rc::calcControlUserIdByPortNum(mMainPlayer->getInputPort());
        }

        bool isPrevJoySingle = mIsJoySingle;
        bool isJoySingle = al::isPadTypeJoySingle(rc::getPadPortByUserId(mainUserId));
        if (isPrevJoySingle) {
            mIsJoySingle = isJoySingle;
        } else if (isJoySingle) {
            mIsJoySingle = true;
            getSceneCameraInfo()->mControlInfo->mIsRequestResetUserControl = true;
        }

        if (mDirector->isPlayLeaveDemo(mainUserId) ||
            rc::isDeadControlUserInStage(GameDataHolderAccessor(mGameDataHolder), mainUserId)) {
            // The result of this check is unused in the original code as well.
            al::isPadConnected(rc::getPadPortByUserId(mainUserId));
        }

        s32 entryUserId = -1;
        s32 aliveUserNum = 0;
        for (s32 i = 0; i < userNum; i++) {
            if (entryUserId < 0 && al::isPadConnected(rc::getPadPortByUserId(i))) {
                entryUserId = i;
            }

            if (!mDirector->isPlayLeaveDemo(userIds[i]) &&
                !rc::isDeadControlUserInStage(GameDataHolderAccessor(mGameDataHolder),
                                              userIds[i])) {
                aliveUserNum++;
            }
        }

        if (entryUserId >= 0 && aliveUserNum == 0) {
            PlayerEntryFunction::entryPlayer(
                GameDataHolderAccessor(mGameDataHolder), entryUserId,
                rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder),
                                                entryUserId));
            mDirector->activateUser(entryUserId, true);
        } else {
            for (s32 i = 0; i < userNum; i++) {
                if (!mDirector->isPlayLeaveDemo(userIds[i]) &&
                    !rc::isDeadControlUserInStage(GameDataHolderAccessor(mGameDataHolder),
                                                  userIds[i])) {
                    mMainPlayer = static_cast<PlayerActor*>(
                        rc::findPlayerActorFirstByUserId(mMainPlayer, userIds[i]));
                    break;
                }
            }
        }

        mDirector->setMainPlayer(mMainPlayer);

        for (s32 i = 0; i < al::getPlayerNumMax(mLiveActorKit->getPlayerHolder()); i++) {
            al::offAreaTarget(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i));
        }

        al::onAreaTarget(mMainPlayer);
    }

    if (isPause()) {
        if (al::isFirstStep(this)) {
            al::updateKitList(this, "２Ｄ");
        }

        al::updateKitList(this, "２Ｄ（ポーズ無視）");
        al::updateEffectLayout(this);
    } else if (isPauseStart()) {
        al::updateKitList(this, "２Ｄ");
        al::updateKitList(this, "２Ｄ（ポーズ無視）");
        al::updateEffectLayout(this);
    } else if (al::isNerve(this, &NrvCourseSelectSceneOpening) ||
               al::isNerve(this, &NrvCourseSelectSceneSave) ||
               al::isNerve(this, &NrvCourseSelectSceneSaveGotoTitle) ||
               al::isNerve(this, &NrvCourseSelectSceneSaveNoWindow)) {
        al::updateKit(this);
        updateWorldClipArea();
    } else {
        al::updateKit(this);
        updateWorldClipArea();

        if (!al::isNerve(this, &NrvCourseSelectSceneMapPauseEnd)) {
            mDirector->update();
        }
    }

    if (rc::isControllerAssignmentChanged() && mDirector != nullptr) {
        mDirector->getLayout()->getSceneLayout()->updateButtonIcons();
    }
}

/**
 * Gets whether the user camera control is invalid (while the scene pauses).
 * @return true if the camera control is invalid.
 */
bool CourseSelectScene::isInvalidCameraCtrl() const {
    return isPause() || isPauseStart();
}

/**
 * Gets whether the scene pauses (pause menu, map menu or lists open).
 * @return true if the scene pauses.
 */
bool CourseSelectScene::isPause() const {
    if (al::isNerve(this, &NrvCourseSelectScenePause) ||
        al::isNerve(this, &NrvCourseSelectSceneMapPause) ||
        al::isNerve(this, &NrvCourseSelectSceneMapPauseReturn) ||
        al::isNerve(this, &NrvCourseSelectSceneStarList) ||
        al::isNerve(this, &NrvCourseSelectSceneStampList)) {
        return true;
    }

    if (al::isNerve(this, &NrvCourseSelectSceneMapPauseEnd) && al::isLessEqualStep(this, 58)) {
        return true;
    }

    return al::isNerve(this, &NrvCourseSelectSceneLoadGame);
}

/**
 * Gets the pad port of the main player.
 * @return The pad port.
 */
s32 CourseSelectScene::getMainPlayerInputPort() const {
    return mMainPlayer->getInputPort();
}

/**
 * Gets the player holder of the scene.
 * @return The player holder.
 */
al::PlayerHolder* CourseSelectScene::getPlayerHolder() const {
    return mLiveActorKit->getPlayerHolder();
}

/**
 * Updates the layouts updated while the scene pauses.
 */
void CourseSelectScene::updatePauseLayout() {
    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Gets whether a pause (pause menu or map menu) is starting.
 * @return true if a pause is starting.
 */
bool CourseSelectScene::isPauseStart() const {
    return al::isNerve(this, &NrvCourseSelectScenePauseStart) ||
           al::isNerve(this, &NrvCourseSelectSceneMapPauseStart);
}

/**
 * Updates the worlds active around the main player from the world clip area it is in.
 */
void CourseSelectScene::updateWorldClipArea() {
    const sead::Vector3f& pos =
        static_cast<s32>(mLiveActorKit->getGraphicsSystemInfo()->getAreaTarget()) ==
                al::GraphicsAreaTarget::Player ?
            al::getTrans(mMainPlayer) :
            al::getCameraLookAt(mMainPlayer);
    auto* area =
        static_cast<WorldClipArea*>(al::tryFindAreaObj(mMainPlayer, "WorldClipArea", pos));
    if (area != nullptr) {
        mDirector->updateActiveWorld(area->getWorldFlag());
    }
}

/**
 * Draws the main screen: 3D view, 2D layouts and screen captures.
 */
void CourseSelectScene::drawMain_() const {
    agl::RenderBuffer* renderBuffer = getFramework()->getCurrentRenderBuffer();
    mMainViewport->setByFrameBuffer(*renderBuffer);
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFramework()->mIsDocked);
    mLayoutKit->setFrameBuffer(renderBuffer, mMainViewport);
    alSystemKitFunction::applyViewportTop(*mMainViewport);

    if (!isPause() && mIsUpdated) {
        mLiveActorKit->getCameraDirector()->getProjection()->getNear();
        mLiveActorKit->getCameraDirector()->getProjection()->getFar();

        al::ViewRenderer* viewRenderer = mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer();
        agl::ShaderMode shaderMode = static_cast<agl::ShaderMode>(4);
        al::tryChangeShaderMode(al::GameFrameworkNx::getAglDrawContext(), shaderMode);
        viewRenderer->drawView(0, 0, mLiveActorKit, getSceneCameraInfo(), renderBuffer,
                               *mMainViewport, true, false, shaderMode);
    }

    if (al::isNerve(this, &NrvCourseSelectSceneSnapshot)) {
        al::drawKit(this, "２Ｄミス（メイン画面）");
    } else {
        al::drawKit(this, "２Ｄベース（メイン画面）");
    }

    if (mScreenCaptureExecutor->isDraw(1)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(), renderBuffer, 1);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(), renderBuffer,
                                           1);
    }

    al::drawKit(this, "2DDrawAboveBlur1");

    if (mScreenCaptureExecutor->isDraw(2)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(), renderBuffer, 2);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(), renderBuffer,
                                           2);
    }

    al::drawKit(this, "2DDrawAboveBlur2");
    mLiveActorKit->getCameraDirector()->getProjection()->setOffset(sead::Vector2f::zero);
}

/**
 * Gets whether the 3D view is drawn while the scene pauses.
 * @return true if the scene pauses.
 */
bool CourseSelectScene::isPauseDraw3D() const {
    return isPause();
}

/**
 * Draws nothing to the sub screen.
 */
void CourseSelectScene::drawSub_() const {}

/**
 * Gets the game data holder.
 * @return The game data holder.
 */
GameDataHolder* CourseSelectScene::getGameDataHolder() const {
    return mGameDataHolder;
}

/**
 * Writes the world and stage of the course the player entered.
 * @param pParam The stage start parameters to write to.
 */
void CourseSelectScene::getSelectedCourseInfo(ProductStageStartParam* pParam) const {
    pParam->setWorldId(mSelectedWorldId);
    pParam->setStageId(mSelectedStageId);
}

/**
 * Gets the course select director.
 * @return The course select director.
 */
CourseSelectDirector* CourseSelectScene::getCourseSelectDirector() const {
    return al::getSceneObj<CourseSelectDirector>(this, SceneObjID_CourseSelectDirector);
}

/**
 * Gets the id of the course the player entered.
 * @return The course id.
 */
s32 CourseSelectScene::getSelectedCourseId() const {
    return mSelectedMiniature->getCourseId();
}

/**
 * Gets whether the scene goes back to the title.
 * @return true if the scene goes back to the title.
 */
bool CourseSelectScene::isGotoTitle() const {
    return al::isNerve(this, &NrvCourseSelectSceneGotoTitle);
}

/**
 * Gets whether a save file is loaded from the pause menu.
 * @return true if a save file is loaded.
 */
bool CourseSelectScene::isLoadGame() const {
    return al::isNerve(this, &NrvCourseSelectSceneLoadGame);
}

/**
 * Gets whether the controller change was chosen in the pause menu.
 * @return true if the controller change was chosen.
 */
bool CourseSelectScene::isControllerChange() const {
    return mPauseMenuMap->isControllerChange();
}

/**
 * Gets whether the map menu jumps to another world.
 * @return true if the map menu jumps to another world.
 */
bool CourseSelectScene::isMapPauseWorldJump() const {
    return al::isNerve(this, &NrvCourseSelectSceneMapPauseEnd);
}

/**
 * Enters the course of a miniature.
 * @param pMiniature The miniature of the course.
 */
void CourseSelectScene::enterStage(CourseSelectMiniature* pMiniature) {
    if (!al::isNerve(this, &NrvCourseSelectScenePlay)) {
        return;
    }

    setNameplatesVisible(false);
    mSelectedMiniature = pMiniature;
    mSelectedWorldId = mSelectedMiniature->getWorldId();
    mSelectedStageId = mSelectedMiniature->getStageId();
    al::setNerve(this, &NrvCourseSelectSceneStageEnter);
}

/**
 * Shows or hides the nameplates of the alive players.
 * @param isVisible Whether the nameplates are shown.
 */
void CourseSelectScene::setNameplatesVisible(bool isVisible) {
    s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());
    for (s32 i = 0; i < playerNum; i++) {
        auto* player =
            static_cast<PlayerActor*>(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i));
        if (al::isAlive(player)) {
            player->setNameplateVisible(isVisible, false);
        }
    }
}

/**
 * Gets whether the main player is controlled with the main controller.
 * @return true if the main player uses the main controller.
 */
bool CourseSelectScene::isMainPlayerInputPortDrc() const {
    if (mMainPlayer == nullptr) {
        return false;
    }

    return mMainPlayer->getInputPort() == al::getMainControllerPort();
}

/**
 * Ignores the input of every player for some frames.
 * @param frame The number of frames.
 */
void CourseSelectScene::invalidatePlayerInput(s32 frame) {
    s32 playerNum = al::getPlayerNumMax(mLiveActorKit->getPlayerHolder());
    for (s32 i = 0; i < playerNum; i++) {
        auto* player =
            static_cast<PlayerActor*>(al::getPlayerActor(mLiveActorKit->getPlayerHolder(), i));
        player->getInput()->invalidateFrame(frame);
    }
}

/**
 * Plays the transition back from a course.
 */
void CourseSelectScene::exeStageExit() {
    invalidatePlayerInput(2);
    update();

    if (al::isFirstStep(this)) {
        mDirector->startDemo(true);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "コースセレクトデモ",
                                                    0, false);
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, getNextNerveAfterStageExit(this));
    }
}

/**
 * Shows the green stars collected in the last course.
 */
void CourseSelectScene::exeGreenStarInfo() {
    invalidatePlayerInput(2);
    update();

    if (al::isFirstStep(this)) {
        if (GameDataFunction::getLastTotalAcquireGreenStarNum(
                GameDataHolderAccessor(mGameDataHolder)) ==
            GameDataFunction::calcTotalAcquireGreenStarNum(
                GameDataHolderAccessor(mGameDataHolder))) {
            al::setNerve(this, getNextNerveAfterStageExit(this));
        } else {
            mIsNeedSave = true;
        }
    } else if (al::isStep(this, 30)) {
        s32 lastNum = GameDataFunction::getLastTotalAcquireGreenStarNum(
            GameDataHolderAccessor(mGameDataHolder));
        s32 totalNum =
            GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor(mGameDataHolder));
        mCounterGreenStar->show(lastNum, totalNum);
    } else if (al::isGreaterStep(this, 30) && mCounterGreenStar->isEnd()) {
        al::setNerve(this, getNextNerveAfterStageExit(this));
    }
}

/**
 * Shows the stamp collected in the last course.
 */
void CourseSelectScene::exeStampInfo() {
    invalidatePlayerInput(2);
    update();

    if (al::isFirstStep(this)) {
        if (!GameDataFunction::isLastPlayCourseFirstAcquireIllustItem(
                GameDataHolderAccessor(mGameDataHolder))) {
            al::setNerve(this, getNextNerveAfterStageExit(this));
        }
    } else if (al::isStep(this, 60)) {
        s32 courseId =
            GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(mGameDataHolder));
        mListStampResult->startAppear(courseId);
    } else if (al::isGreaterStep(this, 60) && mListStampResult->isEnd()) {
        al::setNerve(this, getNextNerveAfterStageExit(this));
    }
}

/**
 * Plays the opening demo of a new game.
 */
void CourseSelectScene::exeOpening() {
    if (al::isFirstStep(this)) {
        CourseSelectFairy* fairy = mDirector->findKoopaCastle(1)->getFairy();
        if (fairy != nullptr) {
            fairy->startWorldStartDemo();
        }
    }

    update();

    if (!al::updateNerveState(this)) {
        return;
    }

    mDirector->endDemo(true);
    mGameDataHolder->startOpening();
    GameDataFlagFunction::setShowWorldStartDemo(GameDataHolderAccessor(mGameDataHolder), 1);
    GameDataFunction::playWorldStartDemo(GameDataHolderAccessor(mGameDataHolder), mWorldId);

    CourseSelectFairy* fairy = mDirector->findKoopaCastle(1)->getFairy();
    if (fairy != nullptr) {
        fairy->endWorldStartDemo();
    }

    al::setNerve(this, &NrvCourseSelectScenePlay);
    SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, false);
}

/**
 * Plays the event shown when returning to the map after the ending.
 */
void CourseSelectScene::exeAfterEndingEvent() {
    invalidatePlayerInput(2);
    update();

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvCourseSelectSceneSave);
    }
}

/**
 * Starts the course select with the clear demo or the free play.
 */
void CourseSelectScene::exeStart() {
    update();

    if (GameDataFunction::isLastPlayCourseFirstClear(GameDataHolderAccessor(mGameDataHolder))) {
        al::setNerve(this, &NrvCourseSelectSceneClearDemo);
    } else {
        al::setNerve(this, &NrvCourseSelectScenePlay);
    }
}

/**
 * Plays the demos shown after a course was cleared (roads and courses opening).
 */
void CourseSelectScene::exeClearDemo() {
    if (al::isPadTriggerA(al::getPlayerControllerPort(0))) {
        CourseSelectRoad::setRoadOpenSuperSpeed(true);
    }

    invalidatePlayerInput(2);

    if (al::isFirstStep(this)) {
        mDirector->startDemo(false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "コースセレクトデモ",
                                                    60, false);
    }

    update();

    if (al::updateNerveState(this)) {
        CourseSelectRoad::setRoadOpenSuperSpeed(false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 60, false);
        GameDataFlagFunction::resetAfterGameOver(GameDataHolderAccessor(mGameDataHolder));
        al::setNerve(this, &NrvCourseSelectSceneRevivePlayer);
    }
}

/**
 * Plays the demo shown when a world is entered for the first time.
 */
void CourseSelectScene::exeWorldStartDemo() {
    invalidatePlayerInput(2);

    if (al::isFirstStep(this)) {
        mDirector->startDemo(false);
    }

    update();

    if (al::updateNerveState(this)) {
        GameDataFlagFunction::setShowWorldStartDemo(GameDataHolderAccessor(mGameDataHolder),
                                                    mWorldId);
        GameDataFunction::playWorldStartDemo(GameDataHolderAccessor(mGameDataHolder), mWorldId);
        al::disableBgmChangeArea(this);
        al::setNerve(this, &NrvCourseSelectSceneSaveNoWindow);
    }
}

/**
 * Revives the players that died in the last course, then saves if needed.
 */
void CourseSelectScene::exeRevivePlayer() {
    invalidatePlayerInput(2);
    update();

    if (!al::updateNerveState(this)) {
        return;
    }

    bool isSave =
        (GameDataFunction::isNeedSave(GameDataHolderAccessor(mGameDataHolder)) &&
         !GameDataFlagFunction::isAfterGameOver(GameDataHolderAccessor(mGameDataHolder))) ||
        mIsNeedSave;

    if (GameDataFunction::isLastPlayCourseClearWorldWarp(GameDataHolderAccessor(mGameDataHolder))) {
        GameDataFunction::onWorldWarpDokanDemo(GameDataHolderAccessor(mGameDataHolder));
    }

    if (isSave) {
        al::setNerve(this, &NrvCourseSelectSceneSave);
    } else {
        al::setNerve(this, &NrvCourseSelectScenePlay);
    }
}

/**
 * Runs the snapshot mode.
 */
void CourseSelectScene::exeSnapshot() {
    mLiveActorKit->preDrawGraphics();
    al::getSceneObj<FurEnv>(this, SceneObjID_FurEnv)->update();

    if (al::isFirstStep(this)) {
        mCameraMode = mLiveActorKit->getCameraDirector()->getCameraMode();
        mLiveActorKit->getClippingDirector()->mFarAreaObserver->mFarClipRate = 2.0f;
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvCourseSelectScenePlay);
        mLiveActorKit->getCameraDirector()->setCameraMode(mCameraMode);
        mLiveActorKit->getClippingDirector()->mFarAreaObserver->mFarClipRate = 1.0f;
    }
}

/**
 * Saves the game.
 */
void CourseSelectScene::exeSave() {
    invalidatePlayerInput(2);
    update();

    if (al::isFirstStep(this)) {
        mIsNeedSave = false;
        GameDataFunction::onSaveStartInCourseSelect(GameDataHolderAccessor(mGameDataHolder));

        if (al::isNerve(this, &NrvCourseSelectSceneSaveGotoTitle)) {
            SaveDataAccessFunction::startSaveDataWriteNoMessage(mGameDataHolder, false);
        } else if (al::isNerve(this, &NrvCourseSelectSceneSaveNoWindow)) {
            SaveDataAccessFunction::startSaveDataWriteNoWindow(mGameDataHolder, false, false);
        } else {
            bool isShowMessage = !mIsAfterEndingEvent;
            SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder, isShowMessage, -1, false);
        }

        al::changeBgmSituation(this, "CourseSelectExitStage");
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "コースセレクトデモ",
                                                    60, false);
    }

    if (!SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        return;
    }

    if (mIsAfterEndingEvent) {
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 60, false);
        kill();
        return;
    }

    if (al::isNerve(this, &NrvCourseSelectSceneSaveGotoTitle)) {
        al::setNerve(this, &NrvCourseSelectSceneGotoTitle);
        return;
    }

    alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 60, false);
    al::setNerve(this, &NrvCourseSelectScenePlay);
}

/**
 * Waits for the scene to be switched to the save file loading.
 */
void CourseSelectScene::exeLoadGame() {}

/**
 * Gets whether a connected control user pressed the pause button.
 * @param pPort The pad port that pressed the button is written here (can be nullptr).
 * @return true if the pause button was pressed.
 */
bool CourseSelectScene::isTriggerPause(s32* pPort) const {
    sead::BitFlag16 portList(rc::getActiveInputPortList(GameDataHolderAccessor(mGameDataHolder)));

    for (s32 i = 0; i <= al::getMaxControllerPorts(); i++) {
        if (!portList.isOnBit(i) || mDirector->isPlayEntryDemoAny() ||
            !al::isPadConnected(i)) {
            continue;
        }

        bool isTrigger;
        if (al::isPadTypeJoySingle(i)) {
            isTrigger = al::isPadTriggerPlus(i) || al::isPadTriggerMinus(i);
        } else {
            isTrigger = rc::isPadTriggerStart(i);
        }

        if (isTrigger) {
            if (pPort != nullptr) {
                *pPort = i;
            }

            return true;
        }
    }

    return false;
}

/**
 * Gets whether a connected control user pressed the map menu button.
 * @param pPort The pad port that pressed the button is written here (can be nullptr).
 * @return true if the map menu button was pressed.
 */
bool CourseSelectScene::isTriggerMapMenu(s32* pPort) const {
    sead::BitFlag16 portList(rc::getActiveInputPortList(GameDataHolderAccessor(mGameDataHolder)));

    for (s32 i = 0; i <= al::getMaxControllerPorts(); i++) {
        if (!portList.isOnBit(i)) {
            continue;
        }

        bool isTrigger;
        if (al::isPadTypeJoySingle(i)) {
            isTrigger = al::isPadTriggerX(i);
        } else {
            isTrigger = al::isPadTriggerMinus(i);
        }

        if (isTrigger && al::isPadConnected(i) && !mDirector->isPlayEntryDemoAny()) {
            if (pPort != nullptr) {
                *pPort = i;
            }

            return true;
        }
    }

    return false;
}

/**
 * Waits before opening the pause menu.
 */
void CourseSelectScene::exePauseStart() {
    update();

    if (al::isStep(this, 10)) {
        al::setNerve(this, &NrvCourseSelectScenePause);
    }
}

/**
 * Runs the pause menu.
 */
void CourseSelectScene::exePause() {
    if (al::isFirstStep(this)) {
        al::pausePadRumble(this);

        if (!checkEnableOpenMenu()) {
            return;
        }

        mPauseMenuMap->appear(mMenuPort, mWorldId);
        alAudioSystemFunction::pauseSystem(mAudioDirector, this, true, 0);
        return;
    }

    if (al::isStep(this, 2)) {
        mDirector->getLayout()->startPause();

        if (static_cast<u32>(mWorldId - 1) < cNormalWorldNum) {
            CourseSelectFairy* fairy = mDirector->findKoopaCastle(mWorldId)->getFairy();
            if (fairy != nullptr) {
                fairy->startPause();
            }
        }
    }

    al::updateKitList(this, "２Ｄ");
    update();
    mMenuPort = mPauseMenuMap->getPort();

    bool isForceExit = false;
    if (!al::isPadConnected(mMenuPort)) {
        if (isPadConnectedAny()) {
            isForceExit = true;
            mPauseMenuMap->forceExit();
        } else if (rc::getActiveControlUserNum(GameDataHolderAccessor(this)) < 2) {
            rc::forceControllerApplet();
        } else {
            mPauseMenuMap->forceExit();
        }
    }

    s32 port;
    if (isTriggerPause(&port) && port == mMenuPort && mPauseMenuMap->isWait()) {
        mPauseMenuMap->decideBack();
    }

    bool isClose = (mPauseMenuMap->isDecideBack() || mPauseMenuMap->isDecideLeaveGame() ||
                    (mPauseMenuMap->isEnd() && isForceExit)) &&
                   mPauseMenuMap->isEnd();

    if (!isClose) {
        if (mPauseMenuMap->isDecideGoToTitle()) {
            al::stopAllBgm(this, 0);
            al::stopAllSequenceBgm(this, 0);

            if (mPauseMenuMap->isEndGoToTitle()) {
                alSeFunction::deactivateRequestKeeper(mAudioDirector, "メイン");
                alSeFunction::deactivateRequestKeeper(mAudioDirector, "常時");
                mDirector->startDemo(false);
                GameDataFunction::onGotoTitle(GameDataHolderAccessor(mGameDataHolder), mWorldId);
                al::updateKitList(this, "２Ｄ");
                al::setNerve(this, &NrvCourseSelectSceneGotoTitle);
                al::changeBgmSituation(this, "CourseSelectExitStage");
                alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン",
                                                            "コースセレクトデモ", 60, false);
            }

            return;
        }

        if (mPauseMenuMap->isDecideLoad()) {
            al::setNerve(this, &NrvCourseSelectSceneLoadGame);
        }

        if (!mPauseMenuMap->isEnd()) {
            return;
        }
    }

    mDirector->getLayout()->endPause();

    if (static_cast<u32>(mWorldId - 1) < cNormalWorldNum) {
        CourseSelectFairy* fairy = mDirector->findKoopaCastle(mWorldId)->getFairy();
        if (fairy != nullptr) {
            fairy->endPause();
        }
    }

    mPauseMenuMap->kill();
    al::updateKitList(this, "２Ｄ");
    al::endPausePadRumble(this);
    al::setNerve(this, &NrvCourseSelectScenePlay);
    mScreenCaptureExecutor->offDraw(1);

    if (mScreenCaptureExecutor->isDraw(2)) {
        mScreenCaptureExecutor->offDraw(2);
    }

    alAudioSystemFunction::pauseSystem(mAudioDirector, this, false, 0);
}

/**
 * Checks whether the user that opened a menu can use it, otherwise goes back to the free play.
 * @return true if the menu can be opened.
 */
bool CourseSelectScene::checkEnableOpenMenu() {
    s32 userId =
        rc::tryCalcControlUserIdFromPortNum(GameDataHolderAccessor(mGameDataHolder), mMenuPort);
    if (userId < 0 || mDirector->isPlayLeaveDemo(userId)) {
        update();
        mScreenCaptureExecutor->offDraw(1);
        al::setNerve(this, &NrvCourseSelectScenePlay);
        return false;
    }

    return true;
}

/**
 * Captures the screen before opening the map menu.
 */
void CourseSelectScene::exeMapPauseStart() {
    invalidatePlayerInput(2);
    update();

    if (al::isStep(this, 10)) {
        mScreenCaptureExecutor->requestCapture(false, 1, true);
        al::setNerve(this, &NrvCourseSelectSceneMapPause);
    }
}

/**
 * Runs the map menu.
 */
void CourseSelectScene::exeMapPause() {
    invalidatePlayerInput(2);

    if (rc::isControllerAssignmentChanged()) {
        mMenuHeader->setButtonType(al::isPadTypeJoySingle(mMenuPort));
    }

    if (al::isFirstStep(this)) {
        al::pausePadRumble(this);

        if (!checkEnableOpenMenu()) {
            return;
        }

        if (al::isNerve(this, &NrvCourseSelectSceneMapPauseReturn)) {
            mMenuHeader->setHeader("Map", true);
        } else {
            mMenuHeader->setHeader("Map", false);
            mControlGuideBar->appearWithMessage(RCSControlGuideBar::GuideBarMsgType(3), mMenuPort);
            GameDataHolderAccessor accessor(mGameDataHolder);
            mControlGuideBar->setCharacter(
                rc::getPlayerCharacterName(rc::getControlUserCharacterType(
                    accessor, rc::calcControlUserIdFromPortNum(accessor, mMenuPort))));
            mMenuHeader->appear();
            mMapMenu->startAppear(mMenuPort, mWorldId);
            alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, true, 0);
            al::changeBgmSituation(this, "ShowMap");
        }
    }

    update();

    if (al::isLessEqualStep(this, 10)) {
        return;
    }

    if (!al::isPadConnected(mMenuPort)) {
        if (isPadConnectedAny() ||
            rc::getActiveControlUserNum(GameDataHolderAccessor(this)) >= 2) {
            if (!mMapMenu->isStartEnd()) {
                mMapMenu->forceEnd();
                mMenuHeader->end();
                return;
            }
        } else {
            rc::forceControllerApplet();
        }
    }

    if (mMapMenu->isStartEnd()) {
        mMenuHeader->end();

        if (mMapMenu->isEnd()) {
            mScreenCaptureExecutor->offDraw(1);
            al::endPausePadRumble(this);
            al::setNerve(this, &NrvCourseSelectScenePlay);
            alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
            al::changeBgmSituation(this, "HideMap");
            al::updateKitList(this, "２Ｄ");
        }

        return;
    }

    if (!mMapMenu->isDecideAny()) {
        if (al::isPadTriggerL(mMenuPort)) {
            mMapMenu->transitionOut(false);
            mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType(2), mMenuPort, true);
            al::setNerve(this, &NrvCourseSelectSceneStampList);
            mListStamp->startAppear(mMenuPort, true, false);
        } else if (al::isPadTriggerR(mMenuPort)) {
            mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType(1), mMenuPort, true);
            al::setNerve(this, &NrvCourseSelectSceneStarList);
            mListClearStar->startAppear(mMenuPort, mMapMenu->getWorldId(), true, true);
            mMapMenu->transitionOut(true);
            return;
        }
    }

    if (mMapMenu->isWorldJumpStart()) {
        mMenuHeader->end();
        mControlGuideBar->end();
        al::setNerve(this, &NrvCourseSelectSceneMapPauseEnd);
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
    }
}

/**
 * Waits for the world jump of the map menu to finish.
 */
void CourseSelectScene::exeMapPauseEnd() {
    invalidatePlayerInput(2);
    update();

    if (mMapMenu->isWorldJumpFinish()) {
        mScreenCaptureExecutor->offDraw(1);
        al::endPausePadRumble(this);
        al::setNerve(this, &NrvCourseSelectScenePlay);
    }
}

/**
 * Runs the green star list of the map menu.
 */
void CourseSelectScene::exeStarList() {
    if (al::isFirstStep(this)) {
        mMenuHeader->setHeader("Progress", true);
    }

    update();

    if (rc::isControllerAssignmentChanged()) {
        mMenuHeader->setButtonType(al::isPadTypeJoySingle(mMenuPort));
    }

    if (al::isLessEqualStep(this, 10)) {
        return;
    }

    if (mMapMenu->isStartEnd()) {
        mMenuHeader->end();

        if (mMapMenu->isEnd()) {
            mScreenCaptureExecutor->offDraw(1);
            al::setNerve(this, &NrvCourseSelectScenePlay);
            alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
            al::changeBgmSituation(this, "HideMap");
            al::updateKitList(this, "２Ｄ");
        }

        return;
    }

    if (mListClearStar->isInTransition()) {
        return;
    }

    s32 port = mMenuPort;
    if (rc::isPadTriggerUiCancelByPort(port) ||
        (al::isPadTypeJoySingle(port) ? al::isPadTriggerX(port) : al::isPadTriggerMinus(port))) {
        mMapMenu->forceEnd();
        mListClearStar->startEnd(false, false);
        mMenuHeader->end();
        return;
    }

    if (!al::isPadConnected(mMenuPort)) {
        if (isPadConnectedAny() ||
            rc::getActiveControlUserNum(GameDataHolderAccessor(this)) >= 2) {
            if (!mMapMenu->isStartEnd()) {
                mMapMenu->forceEnd();
                mListClearStar->startEnd(false, false);
                mMenuHeader->end();
                return;
            }
        } else {
            rc::forceControllerApplet();
        }
    }

    if (al::isPadTriggerL(mMenuPort)) {
        mListClearStar->startEnd(true, false);
        mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType(3), mMenuPort, true);
        mMapMenu->transitionIn(false);
        al::setNerve(this, &NrvCourseSelectSceneMapPauseReturn);
    } else if (al::isPadTriggerR(mMenuPort)) {
        mListClearStar->startEnd(true, true);
        al::setNerve(this, &NrvCourseSelectSceneStampList);
        mListStamp->startAppear(mMenuPort, true, true);
        return;
    }

    if (mListClearStar->isEnd()) {
        mScreenCaptureExecutor->offDraw(1);
        al::setNerve(this, &NrvCourseSelectScenePlay);
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
        al::changeBgmSituation(this, "HideMap");
        al::updateKitList(this, "２Ｄ");
    }
}

/**
 * Runs the stamp list of the map menu.
 */
void CourseSelectScene::exeStampList() {
    if (al::isFirstStep(this)) {
        mMenuHeader->setHeader("Stamps", true);
    }

    update();

    if (rc::isControllerAssignmentChanged()) {
        mMenuHeader->setButtonType(al::isPadTypeJoySingle(mMenuPort));
    }

    if (al::isLessEqualStep(this, 10)) {
        return;
    }

    if (mMapMenu->isStartEnd()) {
        mMenuHeader->end();

        if (mMapMenu->isEnd()) {
            mScreenCaptureExecutor->offDraw(1);
            al::setNerve(this, &NrvCourseSelectScenePlay);
            alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
            al::changeBgmSituation(this, "HideMap");
            al::updateKitList(this, "２Ｄ");
        }

        return;
    }

    if (mListStamp->isInTransition()) {
        return;
    }

    s32 port = mMenuPort;
    if (rc::isPadTriggerUiCancelByPort(port) ||
        (al::isPadTypeJoySingle(port) ? al::isPadTriggerX(port) : al::isPadTriggerMinus(port))) {
        mMapMenu->forceEnd();
        mMenuHeader->end();
        mListStamp->startEnd(false, false);
        return;
    }

    if (!al::isPadConnected(mMenuPort)) {
        if (isPadConnectedAny() ||
            rc::getActiveControlUserNum(GameDataHolderAccessor(this)) >= 2) {
            if (!mMapMenu->isStartEnd()) {
                mMapMenu->forceEnd();
                mListStamp->startEnd(false, false);
                mMenuHeader->end();
                return;
            }
        } else {
            rc::forceControllerApplet();
        }
    }

    if (al::isPadTriggerL(mMenuPort)) {
        mListStamp->startEnd(true, false);
        al::setNerve(this, &NrvCourseSelectSceneStarList);
        mListClearStar->startAppear(mMenuPort, mMapMenu->getWorldId(), true, false);
    } else if (al::isPadTriggerR(mMenuPort)) {
        mListStamp->startEnd(true, true);
        mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType(3), mMenuPort, true);
        mMapMenu->transitionIn(true);
        al::setNerve(this, &NrvCourseSelectSceneMapPauseReturn);
        return;
    }

    if (mListStamp->isEnd()) {
        mScreenCaptureExecutor->offDraw(1);
        al::setNerve(this, &NrvCourseSelectScenePlay);
        alAudioSystemFunction::pauseSystem(mAudioDirector, nullptr, false, 0);
        al::changeBgmSituation(this, "HideMap");
        al::updateKitList(this, "２Ｄ");
    }
}

/**
 * Runs the free play: idle control users, demos, world start demos, bgm and the menus.
 */
void CourseSelectScene::exePlay() {
    if (al::isFirstStep(this)) {
        if (mDirector->isDemo() && !mDirector->isPlayPuppeterDemoAll()) {
            mDirector->endDemo(!SaveDataAccessFunction::isWindowProcessingActive(mGameDataHolder));
        }

        mMenuPort = -1;
        GameDataFlagFunction::resetAfterGameOver(GameDataHolderAccessor(mGameDataHolder));
        al::enableBgmChangeArea(this);
    }

    for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
        if (!rc::isActiveControlUser(GameDataHolderAccessor(mGameDataHolder), i) ||
            al::isPadConnected(rc::getPadPortByUserId(i)) ||
            rc::getActiveControlUserNum(GameDataHolderAccessor(mGameDataHolder)) < 2 ||
            mDirector->isPlayPuppeterDemoAll()) {
            mIdleFrames[i] = 0;
            continue;
        }

        if (!mIsFirstPlayFrame && mIdleFrames[i]++ <= cLeaveIdleFrame) {
            continue;
        }

        if (mDirector->isInCourseSelectBubbleAny()) {
            continue;
        }

        mDirector->leaveUser(i);
        mIdleFrames[i] = 0;
    }

    mIsFirstPlayFrame = false;
    update();

    if (mDirector->isEventGateKeeper()) {
        al::setNerve(this, &NrvCourseSelectSceneEventGateKeeper);
        return;
    }

    if (mDirector->isPlayPuppeterDemoAll()) {
        mDirector->startDemo(false);
    } else if (mDirector->isDemo()) {
        mDirector->endDemo(!SaveDataAccessFunction::isWindowProcessingActive(mGameDataHolder));
    } else if (mDirector->getLayout()->isDemo() &&
               !SaveDataAccessFunction::isWindowProcessingActive(mGameDataHolder)) {
        mDirector->getLayout()->endDemo(true);
    }

    bool isNoWorldStartDemo = false;
    s32 worldId = rc::tryFindPlacementWorldId(mMainPlayer, &isNoWorldStartDemo);
    if (worldId != -1) {
        mWorldId = worldId;
        mDirector->setWorldId(worldId);
    }

    if (!rc::isPlayerBinded(mMainPlayer) &&
        !GameDataFlagFunction::isShowWorldStartDemo(GameDataHolderAccessor(mGameDataHolder),
                                                    mWorldId) &&
        !isNoWorldStartDemo) {
        if (mWorldId <= cNormalWorldNum) {
            al::setNerve(this, &NrvCourseSelectSceneWorldStartDemo);
        } else {
            GameDataFlagFunction::setShowWorldStartDemo(GameDataHolderAccessor(mGameDataHolder),
                                                        mWorldId);
        }

        return;
    }

    if (al::isFirstStep(this)) {
        GameDataHolderAccessor accessor(mGameDataHolder);
        if (CourseInfoFunction::isClear(accessor, GameDataFunction::calcCourseId(accessor, 1, 1))) {
            al::activateAudioEventController(this);
            al::startSequenceBgmWithAreaCheck(this, false, -1, 0, -1);
            al::changeBgmSituation(this, "CourseSelectPlay");
        } else {
            al::startSequenceBgm(this, "FirstStage", -1, 0);
            al::changeAudioEffectWithAreaCheck(this);
        }
    }

    if (al::getTrans(mMainPlayer).y < cFallLimitY) {
        al::getTrans(mMainPlayer);
        mDirector->setPlayerPositionWorldStart(mWorldId);
    }

    if (!al::isNerve(this, &NrvCourseSelectScenePlay) || al::isLessStep(this, 15) ||
        mDirector->isInCourseSelectBubbleAny() || mDirector->isPlayPuppeterDemoAny()) {
        return;
    }

    if (isTriggerPause(&mMenuPort)) {
        mScreenCaptureExecutor->requestCapture(false, 1, true);
        al::setNerve(this, &NrvCourseSelectScenePause);
    } else if (isTriggerMapMenu(&mMenuPort)) {
        mMenuHeader->setButtonType(al::isPadTypeJoySingle(mMenuPort));
        al::setNerve(this, &NrvCourseSelectSceneMapPauseStart);
    } else if (al::isPadTriggerDown(al::getMainControllerPort()) &&
               rc::findActiveUserIdList(nullptr, GameDataHolderAccessor(this)) <= 1) {
        al::setNerve(this, &NrvCourseSelectSceneSnapshot);
    }
}

/**
 * Plays the transition into a course, or goes back to the free play if it was canceled.
 */
void CourseSelectScene::exeStageEnter() {
    if (al::isFirstStep(this)) {
        mDirector->getLayout()->getSceneLayout()->startDemo(false);
        nn::hid::StopLrAssignmentMode();
    }

    update();

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStageEnterState->isCancel()) {
        setNameplatesVisible(true);
        nn::hid::StartLrAssignmentMode();
        al::setNerve(this, &NrvCourseSelectScenePlay);
        mDirector->cancelEnter();
        return;
    }

    if (al::getCurPlayingBgmPlayName(this) != nullptr) {
        al::stopAllSequenceBgm(this, 60);
    }

    kill();
}

/**
 * Waits while the event gate keeper talks to the player.
 */
void CourseSelectScene::exeEventGateKeeper() {
    if (al::isFirstStep(this)) {
        mDirector->startDemo(false);
    }

    invalidatePlayerInput(2);
    update();

    if (mDirector->isEventGateKeeper()) {
        return;
    }

    mDirector->endDemo(true);
    al::setNerve(this, &NrvCourseSelectScenePlay);
}

/**
 * Waits for the scene to be switched to the title.
 */
void CourseSelectScene::exeGotoTitle() {
    update();
}

/**
 * Creates the players of every character on the placement of the main player.
 * @param pStageInfo The stage info holding the player placement.
 * @param rInfo The actor init info.
 */
void CourseSelectScene::initPlacementPlayer(const al::StageInfo* pStageInfo,
                                            const al::ActorInitInfo& rInfo) {
    al::createSceneObj(this, SceneObjID_PlayerGroup);
    al::getSceneObj<PlayerGroupSceneObj>(this, SceneObjID_PlayerGroup)->initGroup(128);
    al::setSceneObj(this, new FurEnv(mLiveActorKit->getGraphicsSystemInfo()), SceneObjID_FurEnv);

    al::PlacementInfo playerListInfo;
    s32 playerListNum = 0;
    al::getPlacementInfoAndCount(&playerListInfo, &playerListNum, pStageInfo, "PlayerList");

    al::PlacementInfo playerInfo;
    const char* objectName = nullptr;
    bool isFound = false;
    for (s32 i = 0; i < playerListNum; i++) {
        al::getPlacementInfoByIndex(&playerInfo, playerListInfo, i);
        al::getObjectName(&objectName, playerInfo);
        if (al::isEqualString(objectName, "Player")) {
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        return;
    }

    al::ActorInitInfo playerActorInfo;
    playerActorInfo.initNoViewId(&playerInfo, rInfo);
    mMainPlayer = nullptr;
    auto* selector = static_cast<PlayerRetargettingSelectorSceneObj*>(
        al::createSceneObj(this, SceneObjID_PlayerRetargettingSelector));

    if (!CourseInfoFunction::isClear(
            GameDataHolderAccessor(mGameDataHolder),
            GameDataFunction::calcCourseId(GameDataHolderAccessor(mGameDataHolder), 1, 1))) {
        mPanicControl = new PanicControlAlways();
    } else if (GameDataFunction::isStageLastPlayAndFirstClear(
                   GameDataHolderAccessor(mGameDataHolder),
                   GameDataFunction::findKoopaCastleCourseId(
                       GameDataHolderAccessor(mGameDataHolder), 7))) {
        mPanicControl = new PanicControlKoopaCastleClear(mDirector);
    }

    s32 mainUserId = 0;
    {
        s32 userIds[cControlUserNum];
        s32 userNum = rc::findActiveUserIdList(userIds, GameDataHolderAccessor(mGameDataHolder));
        for (s32 j = 0; j < userNum; j++) {
            if (!rc::isDeadControlUserInStage(GameDataHolderAccessor(mGameDataHolder),
                                              userIds[j])) {
                mainUserId = userIds[j];
                break;
            }
        }
    }

    for (s32 characterType = 0; characterType < rc::getPlayerCharacterNumMax(); characterType++) {
        s32 userId = rc::tryCalcControlUserIdByCharacterType(GameDataHolderAccessor(this),
                                                             characterType, false);
        bool isMainPort;
        bool isDead;
        if (userId != -1) {
            isMainPort = rc::getControlUserPortNumber(GameDataHolderAccessor(this), userId) ==
                         al::getMainControllerPort();
            isDead = rc::isDeadControlUserInStage(GameDataHolderAccessor(mGameDataHolder), userId);
        } else {
            isMainPort = false;
            isDead = true;
        }

        al::PlayerHolder* playerHolder = mLiveActorKit->getPlayerHolder();
        al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
        GameDataHolder* holder = mGameDataHolder;
        IUsePlayerPanicControl* panicControl = mPanicControl;
        const sead::Matrix34f* viewMtx =
            isMainPort ? &cameraDirector->mMainViewMtx : &cameraDirector->mSubViewMtx;
        const char* characterName = rc::getPlayerCharacterName(characterType);

        u32 figureType;
        s32 port;
        if (userId != -1) {
            figureType = rc::getControlUserFigureType(GameDataHolderAccessor(holder), userId);
            port = rc::getControlUserPortNumber(GameDataHolderAccessor(holder), userId);
        } else {
            figureType = 0;
            port = 0;
        }

        auto* player = new CourseSelectPlayerActor(viewMtx);
        player->createBubble(playerActorInfo);

        PlayerConstParam* constParam = nullptr;
        if (al::getPlayerNumMax(playerHolder) >= 1) {
            constParam =
                static_cast<PlayerActor*>(al::getPlayerActor(playerHolder, 0))->getConstParam();
        }

        PlayerActionGraphBuilderCourseSelect builder(&playerActorInfo.getActorSceneInfo(),
                                                     holder, player, panicControl, constParam);
        player->initSpecial(playerActorInfo, port, characterName, selector, selector, &builder,
                            "プレイヤー", 2, 0, nullptr);
        player->initCourseSelectPlayer(*playerActorInfo.getLayoutInitInfo(), false);
        rc::initPlayerFigureType(player, figureType, false);

        if (userId == -1) {
            rc::deactivatePlayer(player);
        } else {
            f32 displayOffset =
                -rc::calcControlUserDisplayOrder(GameDataHolderAccessor(holder), userId) *
                cPlayerPlacementInterval;
            s32 activeUserNum = rc::getActiveControlUserNum(GameDataHolderAccessor(holder));

            const sead::Vector3f& front = player->getProperty()->getFront();
            sead::Vector3f side(-front.z, 0.0f, front.x);
            side.normalize();

            sead::Vector3f trans =
                player->getProperty()->getTrans() +
                side * ((activeUserNum - 1) * 0.5f * cPlayerPlacementInterval) +
                side * displayOffset;
            player->getProperty()->mTrans = trans;
            al::setTrans(player, trans);

            if (isDead) {
                rc::deactivatePlayer(player);
            }
        }

        player->setEnableSingleJoyCamera(false);

        if (mainUserId == userId) {
            mMainPlayer = player;
            mDirector->setMainPlayer(player);
            rc::setMainPlayerActor(player);
        }

        alPlayerFunction::registerPlayer(player, player->getPadRumbleKeeper(), true);
    }
}

/**
 * Places the objects of a stage info and registers the miniatures to the director.
 * @param pStageInfo The stage info holding the placement.
 * @param rInfo The actor init info.
 * @param pListName Unused: the objects are always taken from the "ObjectList".
 */
void CourseSelectScene::initPlacementObject(const al::StageInfo* pStageInfo,
                                            const al::ActorInitInfo& rInfo,
                                            const char* pListName) {
    al::PlacementInfo listInfo;
    s32 listNum = 0;
    al::getPlacementInfoAndCount(&listInfo, &listNum, pStageInfo, "ObjectList");

    ProjectActorFactory factory;
    for (s32 i = 0; i < listNum; i++) {
        al::PlacementInfo info;
        al::getPlacementInfoByIndex(&info, listInfo, i);

        const char* objectName = nullptr;
        al::getObjectName(&objectName, info);
        al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &info);
        if (actor == nullptr) {
            continue;
        }

        s32 worldId = rc::tryFindPlacementWorldId(actor, nullptr);
        if (worldId != -1) {
            mDirector->registerObject(actor, worldId);
        }

        if (al::isEqualString(objectName, "CourseSelectMiniature")) {
            mDirector->registerStage(static_cast<CourseSelectMiniature*>(actor));
        }
    }
}

/**
 * Initializes the course select nodes once they are connected.
 * @param rInfo The actor init info.
 */
void CourseSelectScene::initPlacementNodeItem(const al::ActorInitInfo& rInfo) {
    for (s32 i = 0; i < mDirector->getNodeNum(); i++) {
        CourseSelectNode* node = mDirector->getNodes()[i];
        al::ActorInitInfo nodeInfo;
        nodeInfo.initViewIdHostActor(rInfo, node);
        node->initAfterConnect(nodeInfo);
        node->initAfterPlacement();
    }
}

#include "Scene/TitleScene.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include <nerd/nerdMath.h>
#include <thread/seadEvent.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Course/CourseSelectLayoutKiosk.hpp"
#include "Layout/PlayerEntry.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/RCS_SaveDataLayout.hpp"
#include "Layout/TitleLogo.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/PadReplayFunction.hpp"
#include "Library/Controller/PadRumbleDirector.hpp"
#include "Library/Draw/GraphicsInitArg.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/FootPrintServer.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Player/FurEnv.hpp"
#include "Player/Giga/PlayerActionGraphBuilder.hpp"
#include "Player/IUsePlayerFlagSwitch.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGroupSceneObj.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "Player/PlayerCooperation.hpp"
#include "Player/PlayerFireBallAppearWatcher.hpp"
#include "Player/PlayerProcess.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Scene/PlayerStocker.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "Scene/TitleDemoInfo.hpp"
#include "Scene/TitleSceneFunction.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Application.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameSystem.hpp"
#include "System/ProductSequence.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"

/** Frame counter of the Bowser's Fury title demo (shared with the player code). */
extern s32 gFrameCount;
/** Set-once flag of the title demo frame counter. */
extern bool sWasSet;

/**
 * Declares a TitleScene nerve whose execute function has a different name than the nerve.
 * @param Action The nerve name.
 * @param Func The TitleScene::exe* function the nerve runs.
 */
#define TITLE_SCENE_NERVE(Action, Func)                                                            \
    class TitleSceneNrv##Action : public al::Nerve {                                               \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<TitleScene>()->exe##Func();                                         \
        }                                                                                          \
    };

namespace {
NERVE_DECL(TitleScene, Load)
NERVES_MAKE_NOSTRUCT(TitleScene, Load)

NERVE_DECL(TitleScene, AppearWait)
NERVE_DECL(TitleScene, Title)
TITLE_SCENE_NERVE(PlayerSelect, PlayerSelect)
NERVE_DECL(TitleScene, NewGameWarning)
NERVE_DECL(TitleScene, FileSelect)
NERVE_DECL(TitleScene, WaitTextFade)
TITLE_SCENE_NERVE(RestartWhiteFade, Restart)
TITLE_SCENE_NERVE(PlayerSelectNewGame, PlayerSelect)
TITLE_SCENE_NERVE(WipeCloseFileSelect, WipeClose)
NERVE_DECL(TitleScene, WipeCloseTopMenu)
TITLE_SCENE_NERVE(PlayerSelectResume, PlayerSelect)
NERVE_DECL(TitleScene, ConfirmLuigi)
NERVE_DECL(TitleScene, DeleteFile)
NERVE_DECL(TitleScene, WipeCloseLuigiBros)
TITLE_SCENE_NERVE(PlayerSelectAfterDelete, PlayerSelect)
TITLE_SCENE_NERVE(WipeCloseCourseSelect, WipeClose)
TITLE_SCENE_NERVE(WipeCloseAppearWait, WipeClose)
TITLE_SCENE_NERVE(WipeClosePlayerSelect, WipeClose)
NERVE_DECL(TitleScene, CourseSelect)
NERVE_DECL(TitleScene, Restart)
NERVES_MAKE_NOSTRUCT(TitleScene, AppearWait, Title, PlayerSelect, NewGameWarning, FileSelect,
                     WaitTextFade, RestartWhiteFade, PlayerSelectNewGame, WipeCloseFileSelect,
                     WipeCloseTopMenu, PlayerSelectResume, ConfirmLuigi, DeleteFile,
                     WipeCloseLuigiBros, PlayerSelectAfterDelete, WipeCloseCourseSelect,
                     WipeCloseAppearWait, WipeClosePlayerSelect, CourseSelect, Restart)

/** Recorded title demos: player count, character of every player and demo stage. */
const s32 cDemoPlayers0[] = {0, 1, 2, 3};
const s32 cDemoPlayers1[] = {1, 3, -1, -1};
const s32 cDemoPlayers2[] = {0, 2, -1, -1};
const s32 cDemoPlayers3[] = {0, -1, -1, -1};
const s32 cDemoPlayers5[] = {2, 4, -1, -1};

TitleDemoInfo sTitleDemoInfos[] = {
    TitleDemoInfo(4, cDemoPlayers0, "TitleDemo03Stage"),
    TitleDemoInfo(2, cDemoPlayers1, "TitleDemo01Stage"),
    TitleDemoInfo(2, cDemoPlayers2, "TitleDemo02Stage"),
    TitleDemoInfo(1, cDemoPlayers3, "TitleDemo00Stage"),
    TitleDemoInfo(1, cDemoPlayers3, "TitleDemo05Stage"),
    TitleDemoInfo(2, cDemoPlayers5, "TitleDemo04Stage"),
};

/** Title demo id of the Bowser's Fury (Rosetta) demo. */
constexpr s32 cTitleDemoIdRosetta = 5;
/** Distance between two players placed side by side in the title demo. */
constexpr f32 cPlayerPlacementInterval = 150.0f;
/** Number of save files. */
constexpr s32 cFileNum = 3;

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}

/**
 * Gets the pad port of the given title demo player.
 * @param index The index of the player.
 * @return The pad port.
 */
u32 getPadPort(s32 index) {
    return GameDataConst::getPadPortList()[index];
}

/**
 * Creates a player of the title demo.
 * @param rInfo The actor init info of the player placement.
 * @param pViewMtx The view matrix of the camera following the player.
 * @param pCharacterName The name of the player's character.
 * @param pSelector The retargetting selector of the players.
 * @return The created player.
 */
PlayerActor* createPlayer(const al::ActorInitInfo& rInfo, const sead::Matrix34f* pViewMtx,
                          const char* pCharacterName, PlayerRetargettingSelector* pSelector) {
    auto* player = new PlayerActor(pViewMtx);
    PlayerActionGraphBuilder builder(false);
    player->initSpecial(rInfo, 0, pCharacterName, pSelector, pSelector, &builder, "プレイヤー", 0,
                        0, nullptr);
    return player;
}
}  // namespace

/**
 * Constructs the title scene.
 * @param pStageWipeKeeper The wipes shared with the stage scenes.
 * @param demoId The id of the title demo to play.
 */
TitleScene::TitleScene(StageWipeKeeper* pStageWipeKeeper, s32 demoId)
    : al::Scene("タイトルシーン"), mStageWipeKeeper(pStageWipeKeeper), mDemoId(demoId) {
    mAudioReadyEvent = new sead::Event(true);
    mPlayerNum = sTitleDemoInfos[mDemoId].mDemoId;
}

/**
 * Destroys the title scene and restores the player settings changed for the title demo.
 */
TitleScene::~TitleScene() {
    gFrameCount = 0;

    if (mAudioReadyEvent != nullptr) {
        delete mAudioReadyEvent;
    }

    rc::setMainPlayerActor(nullptr);
    getFramework()->mIsClearRenderBuffer = true;
    mLiveActorKit->getEffectSystem()->endScene();
    rc::setUsingOldPlayerParams(false);
    nerd::setUseFastsqrte(false);
}

/**
 * Initializes the scene: title demo save file, kits, layouts, placement and pad replays.
 * @param rInfo The scene init info.
 */
void TitleScene::init(const al::SceneInitInfo& rInfo) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mGameDataHolder->setSingleMode(false);
    rc::setUsingOldPlayerParams(true);
    nerd::setUseFastsqrte(true);
    al::initRandomSeed(0);
    al::initRandomSeedNonSync(0);
    initAndLoadStageResource(getDemoStageName(), 1);
    al::tryRequestPreLoadFile(this, rInfo, 1, nullptr);

    mTitleDemoFile = new GameDataFile(mGameDataHolder, 0);
    mTitleDemoFile->getStageDataHolderPtr()->startTitle();

    for (s32 i = 0; i < mPlayerNum; i++) {
        mTitleDemoFile->entryPlayer(i, sTitleDemoInfos[mDemoId].mPlayerIds[i]);
    }

    initSceneStopCtrl();
    mMainViewport = al::getDisplayViewport();
    mSubViewport = al::getSubDisplayViewport();
    mScreenCaptureExecutor = rInfo.mScreenCaptureExecutor;
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    al::setSceneObj(this, mGameDataHolder, SceneObjID_GameDataHolder);
    al::createSceneObj(this, SceneObjID_ScoreHolder);
    al::createSceneObj(this, SceneObjID_ControllerEventWatcher);
    auto* fireBallWatcher = static_cast<PlayerFireBallAppearWatcher*>(al::createSceneObj(this, 15));
    PlayerStockerFunction::createPlayerStocker(this, false);
    initSceneAudio(rInfo, getDemoStageName(), 60, 30, 1, false, "Scene", 20, 1.0f);
    alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "TitleVolumes", 0,
                                                false);

    al::GraphicsInitArg graphicsArg;
    graphicsArg.mViewRendererCreator = new al::ViewRendererCreator();
    graphicsArg.mIsUsingViewRenderer = true;
    graphicsArg.setViewNum(2);
    initLiveActorKitWithGraphics(graphicsArg, rInfo, 1536, 64, 2, 0, false, false);
    mLiveActorKit->initHitSensorDirector(1, false);

    if (fireBallWatcher != nullptr) {
        fireBallWatcher->setGraphicsSystemInfo(mLiveActorKit->getGraphicsSystemInfo());
    }

    al::setSceneObj(this, new PlayerCooperation(64), 14);
    mLiveActorKit->getCameraDirector()->setCameraAspect(mMainViewport, mSubViewport);
    mLiveActorKit->getCameraDirector()->setStageName(getDemoStageName());
    initAudioKeeper(nullptr);
    mAudioReadyEvent->setSignal();
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::initItemDirector(this, new ProjectItemDirector(layoutInfo, mGameDataHolder,
                                                       mLiveActorKit->getPlayerHolder(),
                                                       mLiveActorKit->getAreaObjDirector()));
    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    static_cast<ProjectItemDirector*>(mLiveActorKit->getItemDirector())
        ->createItemHolder(actorInfo, false);
    al::setSceneObj(this, new al::FootPrintServer(actorInfo, "FootPrint", 32), 7);

    mTitleLogo = new TitleLogo(layoutInfo, mGameDataHolder);
    mSaveDataLayout = new RCS_SaveDataLayout(layoutInfo, mGameDataHolder, mScreenCaptureExecutor,
                                             mControlGuideBar, false);
    mPlayerEntry = new PlayerEntry(layoutInfo, actorInfo, mGameDataHolder);
    mCourseSelectLayout = new CourseSelectLayoutKiosk(layoutInfo);
    mControlGuideBar = new RCSControlGuideBar(layoutInfo);
    mCourseSelectLayout->setGuideBar(mControlGuideBar);
    mWipe = new al::WipeSimple("終了ワイプ", "WipeCircle", layoutInfo, nullptr);
    mSaveDataLayout->setGuideBar(mControlGuideBar);
    mTitleLogo->setGuideBar(mControlGuideBar);
    al::setSceneObj(this, new CoinRotater(mLiveActorKit->getExecuteDirector()), 1);

    al::GraphicsSystemInfo* graphicsInfo = mLiveActorKit->getGraphicsSystemInfo();
    const al::Resource* resource = al::getStageInfoDesign(this, 0)->getResource();
    graphicsInfo->initStageResource(resource, getDemoStageName(), mLiveActorKit, false, 0);
    initPlacement(actorInfo);
    mWindowConfirm =
        new WindowConfirm(WindowConfirmType_Double, layoutInfo, "PauseMenu", false);
    mLiveActorKit->getCameraDirector()->init(mLiveActorKit->getPlayerHolder());
    mLiveActorKit->getCameraDirector()->initAudioKeeper(actorInfo);
    endInit(actorInfo, nullptr);

    rInfo.mGameSystemInfo->getGamePadSystem()->changeMultiPlayMode(4, 1);
    rInfo.mGameSystemInfo->getGamePadSystem()->setPadName(0, sead::WSafeString::cEmptyString);
    rInfo.mGameSystemInfo->getGamePadSystem()->setPadName(1, sead::WSafeString::cEmptyString);
    rInfo.mGameSystemInfo->getGamePadSystem()->setPadName(2, sead::WSafeString::cEmptyString);
    rInfo.mGameSystemInfo->getGamePadSystem()->setPadName(3, sead::WSafeString::cEmptyString);
    rInfo.mGameSystemInfo->getGamePadSystem()->set40(false);
    initNerve(&NrvTitleSceneLoad, 0);

    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    mCameraTitleFlag = &cameraDirector->mIsTitleFlag;
    cameraDirector->mIsTitleFlag = false;
    initPadReplayData();
    mPadRumbleDirector = mLiveActorKit->getPadRumbleDirector();
    mPadRumbleDirector->invalidate();
}

/**
 * Gets the name of the stage the title demo plays in.
 * @return The demo stage name.
 */
const char* TitleScene::getDemoStageName() const {
    return sTitleDemoInfos[mDemoId].mStageName;
}

/**
 * Places the area objects, players, cameras, sky and objects of the demo stage.
 * @param rInfo The actor init info.
 */
void TitleScene::initPlacement(const al::ActorInitInfo& rInfo) {
    ProjectActorFactory factory;
    al::initPlacementAreaObj(this, rInfo, nullptr);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
    s32 mapNum = al::getStageInfoMapNum(this);
    auto* selector = static_cast<PlayerRetargettingSelectorSceneObj*>(al::createSceneObj(this, 25));
    al::setSceneObj(this, new FurEnv(mLiveActorKit->getGraphicsSystemInfo()), SceneObjID_FurEnv);
    initPlacementPlayer(al::getStageInfoMap(this, 0), rInfo, selector);
    rc::setPlayerUseOldParams(mLiveActorKit->getPlayerHolder(), true);
    mLiveActorKit->getCameraDirector()->initCameraCreator(mapNum, al::isStageOneResource(this));

    for (s32 i = 0; i < mapNum; i++) {
        mLiveActorKit->getCameraDirector()->setCameraResource(
            al::getStageInfoMap(this, i)->getResource(), i);
    }

    initPlacementSky(al::getStageInfoMap(this, 0), rInfo);

    for (s32 i = 0; i < mapNum; i++) {
        initPlacementObject(al::getStageInfoMap(this, i), rInfo, "ObjectList");
    }

    for (s32 i = 0; i < al::getStageInfoDesignNum(this); i++) {
        initPlacementObject(al::getStageInfoDesign(this, i), rInfo, "ObjectList");
    }

    for (s32 i = 0; i < al::getStageInfoSoundNum(this); i++) {
        initPlacementObject(al::getStageInfoSound(this, i), rInfo, "ObjectList");
    }
}

/**
 * Creates the pad replay of every demo player and loads the recorded input.
 */
void TitleScene::initPadReplayData() {
    for (s32 i = 0; i < mPlayerNum; i++) {
        u32 port = getPadPort(i);
        al::createReplayController(port);
        al::createAndSetPadDataArcReader("SystemData/TitleDemoData", getDemoStageName(), port);
        al::invalidatePadReplay(port);
    }
}

/**
 * Makes the scene appear and starts loading the save data.
 */
void TitleScene::appear() {
    gFrameCount = 0;
    sWasSet = false;
    al::Scene::appear();
    al::setNerve(this, &NrvTitleSceneLoad);
    getFramework()->mIsClearRenderBuffer = false;
    mIsAppearWaitSkip = false;
    mIsAppearFromBoot = false;

    for (s32 i = 0; i < mPlayerNum; i++) {
        al::validatePadReplay(getPadPort(i));
    }
}

/**
 * Waits for the title to appear.
 */
void TitleScene::setAppearWait() {
    al::setNerve(this, &NrvTitleSceneAppearWait);
}

/**
 * Shows the title menu.
 * @param isUnused Unused.
 */
void TitleScene::setTitleNerve(bool isUnused) {
    al::setNerve(this, &NrvTitleSceneTitle);
}

/**
 * Makes the scene appear right after the boot sequence.
 */
void TitleScene::appearFromBoot() {
    appear();
    mIsAppearFromBoot = true;
}

/**
 * Makes the scene appear after a save file was loaded, going straight to the character select.
 */
void TitleScene::appearFromLoad() {
    al::Scene::appear();
    getFramework()->mIsClearRenderBuffer = false;
    mIsAppearWaitSkip = false;
    mIsAppearFromBoot = false;
    mIsAppearFromLoad = true;
    mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_Default,
                                 al::getMainControllerPort(), true);
    mControlGuideBar->appearTitle();
    mScreenCaptureExecutor->offDraw(1);
    mScreenCaptureExecutor->offDraw(2);

    if (!mIsStartBgm) {
        al::startSequenceBgm(this, "Title", -1, 0);
        mIsStartBgm = true;
    }

    al::setNerve(this, &NrvTitleScenePlayerSelect);
}

/**
 * Kills the scene.
 */
void TitleScene::kill() {
    gFrameCount = 0;
    al::Scene::kill();
}

/**
 * Updates the title demo while it replays, otherwise only the 2D layouts.
 */
void TitleScene::control() {
    if (isTitleDraw3D()) {
        if (mDemoId == cTitleDemoIdRosetta) {
            gFrameCount++;
        }

        sWasSet = false;
        update3D();
        mLiveActorKit->preDrawGraphics();
        al::getSceneObj<FurEnv>(this, SceneObjID_FurEnv)->update();
        al::updateKitList(this, "２Ｄ（ポーズ無視）");
        return;
    }

    if (!al::isNerve(this, &NrvTitleSceneNewGameWarning) &&
        !al::isNerve(this, &NrvTitleSceneFileSelect) &&
        !al::isNerve(this, &NrvTitleSceneWaitTextFade)) {
        al::updateKitList(this, "２Ｄ");
    }

    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Gets whether the 3D title demo is drawn (its recorded input is being read).
 * @return true if the title demo is drawn.
 */
bool TitleScene::isTitleDraw3D() const {
    return al::isReadPadReplayData(al::getMainControllerPort());
}

/**
 * Updates the 3D title demo: camera, players, effects and graphics.
 */
void TitleScene::update3D() {
    bool isDraw3D = isTitleDraw3D();

    if (al::isStopScene(this)) {
        al::updateKitList(this, "２Ｄ");
        al::updateEffectSystem(this);
        return;
    }

    mLiveActorKit->getGraphicsSystemInfo()->clearGraphicsRequest();

    if (isDraw3D) {
        if (!mIsPlayerChangeDemo && !al::isStopScene(this)) {
            mLiveActorKit->getCameraDirector()->update(false);
        }

        if (mIsPlayerChangeDemo) {
            al::updateKitList(this, "プレイヤー前処理");
            al::updateKitList(this, "プレイヤー[Movement]");
            al::updateKitList(this, "プレイヤー後処理");
            al::updateKitList(this, "シャドウマスク");
            al::updateKitList(this, "グラフィックス要求者");
            al::updateKitList(this, "２Ｄ");
            al::updateEffectPlayer(this);

            if (!rc::isPlayerChangeDemoAny(mLiveActorKit->getPlayerHolder())) {
                al::PlayerHolder* playerHolder = mLiveActorKit->getPlayerHolder();

                for (s32 i = 0; i < playerHolder->getPlayerNum(); i++) {
                    auto* player = static_cast<PlayerActor*>(playerHolder->getPlayer(i));

                    if (player != nullptr && !al::isDead(player)) {
                        player->setTitleDemoChange(false);
                    }
                }

                mIsPlayerChangeDemo = false;
            }
        } else {
            mLiveActorKit->getCameraDirector()->update(false);

            // The result of this check is unused in the original code as well.
            if (al::getReplayController(al::getMainControllerPort()) != nullptr) {
                al::isPadReplaying(al::getMainControllerPort());
            }

            al::updateKit(this);

            if (rc::isPlayerChangeDemoAny(mLiveActorKit->getPlayerHolder())) {
                al::PlayerHolder* playerHolder = mLiveActorKit->getPlayerHolder();

                for (s32 i = 0; i < playerHolder->getPlayerNum(); i++) {
                    auto* player = static_cast<PlayerActor*>(playerHolder->getPlayer(i));

                    if (player != nullptr && !al::isDead(player)) {
                        player->setTitleDemoChange(true);
                    }
                }

                mIsPlayerChangeDemo = true;
            }
        }
    } else {
        al::updateKitList(this, "２Ｄ");
        al::updateEffectLayout(this);
        al::drawKit(this, "ShadowMaskPreAdd");
    }

    mLiveActorKit->getGraphicsSystemInfo()->updateGraphics(false);
}

/**
 * Draws the 3D title demo, the 2D layouts and the screen captures to the main screen.
 */
void TitleScene::drawMain_() const {
    mMainViewport->setByFrameBuffer(*getFramework()->getCurrentRenderBuffer());
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFramework()->mIsDocked);
    alSystemKitFunction::applyViewportTop(*mMainViewport);
    mLayoutKit->setFrameBuffer(getFramework()->getCurrentRenderBuffer(), mMainViewport);

    if (isTitleDraw3D()) {
        al::LiveActorKit* kit = mLiveActorKit;
        al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
        viewRenderer->drawView(0, 0, kit, getSceneCameraInfo(),
                               getFramework()->getCurrentRenderBuffer(), *mMainViewport, true,
                               false, static_cast<agl::ShaderMode>(4));
    }

    al::drawKit(this, "２Ｄベース（メイン画面）");

    if (mScreenCaptureExecutor->isDraw(1)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     getFramework()->getCurrentRenderBuffer(), 1);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           getFramework()->getCurrentRenderBuffer(), 1);
    }

    al::drawKit(this, "2DDrawAboveBlur1");

    if (mScreenCaptureExecutor->isDraw(2)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     getFramework()->getCurrentRenderBuffer(), 2);
    } else {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           getFramework()->getCurrentRenderBuffer(), 2);
    }

    al::drawKit(this, "2DDrawAboveBlur2");
    mLiveActorKit->getCameraDirector()->getProjection()->setOffset(sead::Vector2f::zero);
}

/**
 * Draws nothing to the sub screen.
 */
void TitleScene::drawSub_() const {}

/**
 * Gets whether the player chose to return to the top menu.
 * @return true if the top menu was chosen.
 */
bool TitleScene::isReturnToTopMenu() const {
    return mTitleLogo->isReturnToTopMenu();
}

/**
 * Gets whether the scene audio has been initialized.
 * @return true if the audio is ready.
 */
bool TitleScene::isAudioReady() {
    return mAudioReadyEvent->wait(sead::TickSpan(0));
}

/**
 * Loads the save data, then waits for the title to appear.
 */
void TitleScene::exeLoad() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataRead(mGameDataHolder, false);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        mGameDataHolder->initialize3DWorldData();
        al::setNerve(this, &NrvTitleSceneAppearWait);
    }
}

/**
 * Starts the title demo and waits for the wipes to end before showing the title logo.
 */
void TitleScene::exeAppearWait() {
    if (al::isFirstStep(this)) {
        mScreenCaptureExecutor->offDraw(1);
        mScreenCaptureExecutor->offDraw(2);
        mTitleLogo->appear();
        mControlGuideBar->showTitle();
        mTitleLogo->updateLuigiButtonState();
        mGameDataHolder->setGameFileForTitleDemo(mTitleDemoFile);
        decidePlayerPlacement();
        al::initRandomSeed(0);
        al::initRandomSeedNonSync(0);
        gFrameCount = 0;
        sWasSet = false;
        startReplay();
    }

    if (mStageWipeKeeper != nullptr && (mStageWipeKeeper->isActiveBootWipe() ||
                                        mStageWipeKeeper->isActiveNoResultWipe())) {
        return;
    }

    if (mWipe->isAlive() || mIsAppearWaitSkip) {
        return;
    }

    mTitleLogo->startAppearAnim(mIsAppearFromBoot || mIsAppearFromLoad);

    if (!mIsStartBgm) {
        mIsStartBgm = true;
        al::startSequenceBgm(this, "Title", -1, 0);
    }

    al::setNerve(this, &NrvTitleSceneTitle);
}

/**
 * Places the players of the title demo side by side, facing the camera.
 */
void TitleScene::decidePlayerPlacement() {
    for (s32 i = 0; i < rc::getPlayerCharacterNumMax(); i++) {
        s32 userId = rc::tryCalcControlUserIdByCharacterType(GameDataHolderAccessor(this), i, false);

        if (userId == -1) {
            continue;
        }

        PlayerActor* player = mPlayers[i];
        GameDataHolderAccessor accessor(mGameDataHolder);
        al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
        bool isMainPort = rc::getControlUserPortNumber(accessor, userId) == al::getMainControllerPort();
        player->setViewMtx(isMainPort ? &cameraDirector->mMainViewMtx :
                                        &cameraDirector->mSubViewMtx);
        player->replaceInputPort(rc::getControlUserPortNumber(accessor, userId));
        rc::initPlayerFigureType(player, rc::getControlUserFigureType(accessor, userId), false);

        const sead::Vector3f& front = player->getProperty()->getFront();
        sead::Vector3f side(front.z, 0.0f, -front.x);
        side.normalize();
        s32 activeNum = rc::getActiveControlUserNum(accessor);
        s32 order = rc::calcControlUserDisplayOrder(accessor, userId);
        const sead::Vector3f& trans = player->getProperty()->getTrans();
        f32 center = (activeNum - 1) * 0.5f * cPlayerPlacementInterval;
        f32 offset = -order * cPlayerPlacementInterval;
        sead::Vector3f pos = trans + side * center + side * offset;
        player->getProperty()->mTrans = pos;
        al::setTrans(player, pos);
        player->updatePosture();
        player->appear();
        player->getModelHolder()->appear();

        if (rc::calcControlUserDisplayOrder(GameDataHolderAccessor(mGameDataHolder), userId) == 0) {
            rc::setMainPlayerActor(player);
        }
    }
}

/**
 * Starts the pad replay of every demo player.
 */
void TitleScene::startReplay() {
    for (s32 i = 0; i < mPlayerNum; i++) {
        al::startPadReplay(getPadPort(i));
    }
}

/**
 * Runs the title menu while the title demo plays.
 */
void TitleScene::exeTitle() {
    if (!isReplayActive()) {
        al::setNerve(this, &NrvTitleSceneRestartWhiteFade);
        return;
    }

    if (mTitleLogo->isStartNewGame()) {
        if (mTitleLogo->getNewFileId() < 0) {
            mScreenCaptureExecutor->requestCapture(false, 1, true);
            al::setNerve(this, &NrvTitleSceneNewGameWarning);
            return;
        }

        al::setNerve(this, &NrvTitleScenePlayerSelectNewGame);
        mControlGuideBar->appearTitle();
        return;
    }

    if (mTitleLogo->isDecideResume() && !mSaveDataLayout->isAlive() &&
        GameDataFunction::getNewFileNum(GameDataHolderAccessor(mGameDataHolder)) != cFileNum) {
        al::setNerve(this, &NrvTitleSceneWipeCloseFileSelect);
        return;
    }

    if (mTitleLogo->isEnd()) {
        pauseReplay();

        if (mTitleLogo->isReturnToTopMenu()) {
            al::stopSequenceBgm(this, "Title", 60);
            al::setNerve(this, &NrvTitleSceneWipeCloseTopMenu);
            return;
        }

        if (mTitleLogo->isDecideResume() &&
            GameDataFunction::getNewFileNum(GameDataHolderAccessor(mGameDataHolder)) == cFileNum) {
            mGameDataHolder->setPlayingFileId(mGameDataHolder->getLastPlayingFileId());
            mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_Default,
                                         al::getMainControllerPort(), true);
            mControlGuideBar->appearTitle();
            al::setNerve(this, &NrvTitleScenePlayerSelectResume);
            return;
        }

        al::setNerve(this, &NrvTitleSceneWipeCloseFileSelect);
        return;
    }

    if (mTitleLogo->isConfirmLuigi()) {
        mScreenCaptureExecutor->requestCapture(false, 1, true);
        al::setNerve(this, &NrvTitleSceneConfirmLuigi);
    }
}

/**
 * Pauses the pad replay of every demo player that is still replaying.
 */
void TitleScene::pauseReplay() {
    for (s32 i = 0; i < mPlayerNum; i++) {
        u32 port = getPadPort(i);

        if (al::isPadReplaying(port)) {
            al::pausePadReplay(port);
        }
    }
}

/**
 * Warns that every save file is in use and offers to delete one.
 */
void TitleScene::exeNewGameWarning() {
    if (al::isFirstStep(this)) {
        pauseReplay();
        mTitleLogo->kill();
        al::updateKitList(this, "２Ｄ");
        mControlGuideBar->hide();
        mWindowConfirm->appearWithSystemMessage("WindowConfirmTitleScene", "NewFileWarning",
                                                al::getMainControllerPort(), "Rボタン");
    }

    // The result of this check is unused in the original code as well.
    al::isStep(this, 2);

    if (mWindowConfirm->isAlive()) {
        return;
    }

    if (mWindowConfirm->isDecideLeftEnd()) {
        startReplay();
        mScreenCaptureExecutor->offDraw(1);
        mTitleLogo->appearFromMenu();
        mControlGuideBar->show();
        al::setNerve(this, &NrvTitleSceneTitle);
    } else if (mWindowConfirm->isDecideRightEnd()) {
        al::setNerve(this, &NrvTitleSceneDeleteFile);
    }
}

/**
 * Ends the title demo: enables the pad rumble again and removes the pad replays.
 */
void TitleScene::endTitle() {
    mPadRumbleDirector->validate();

    for (s32 i = 0; i < mPlayerNum; i++) {
        u32 port = getPadPort(i);
        al::endPadReplay(port);
        al::unregistReplayController(port);
    }
}

/**
 * Asks whether to start the Luigi Bros mini game.
 */
void TitleScene::exeConfirmLuigi() {
    if (al::isFirstStep(this)) {
        al::startAction(mTitleLogo, "Hide", "Visibility");
        al::updateKitList(this, "２Ｄ");
        mControlGuideBar->hide();
    }

    if (al::isStopScene(this)) {
        return;
    }

    pauseReplay();

    if (mTitleLogo->isConfirmLuigi()) {
        return;
    }

    if (mTitleLogo->isStartLuigi()) {
        mIsLuigiBros = true;
        al::setNerve(this, &NrvTitleSceneWipeCloseLuigiBros);
        return;
    }

    startReplay();
    mScreenCaptureExecutor->offDraw(1);
    mControlGuideBar->show();
    al::startAction(mTitleLogo, "Show", "Visibility");
    al::setNerve(this, &NrvTitleSceneTitle);
}

/**
 * Runs the save file select.
 */
void TitleScene::exeFileSelect() {
    if (al::isFirstStep(this)) {
        mLastPlayingFileId = mGameDataHolder->getLastPlayingFileId();
        pauseReplay();
    }

    if (mSaveDataLayout->isEndBack()) {
        mTitleLogo->appearFromMenu();
        startReplay();
        mScreenCaptureExecutor->offDraw(2);
        mScreenCaptureExecutor->offDraw(1);
        al::setNerve(this, &NrvTitleSceneTitle);
    } else if (mSaveDataLayout->isNeedLoad()) {
        mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_Default,
                                     al::getMainControllerPort(), true);
        al::setNerve(this, &NrvTitleScenePlayerSelect);
    }
}

/**
 * Lets the player delete a save file when every file is in use.
 */
void TitleScene::exeDeleteFile() {
    if (al::isFirstStep(this)) {
        mSaveDataLayout->appearDelete(al::getMainControllerPort(), false);
    }

    if (mSaveDataLayout->isNeedLoad()) {
        al::setNerve(this, &NrvTitleScenePlayerSelectAfterDelete);
    }

    if (mSaveDataLayout->isAlive()) {
        return;
    }

    startReplay();
    mScreenCaptureExecutor->offDraw(1);
    mTitleLogo->appearFromMenu();
    mControlGuideBar->showTitle();
    al::setNerve(this, &NrvTitleSceneTitle);
}

/**
 * Runs the character select.
 */
void TitleScene::exePlayerSelect() {
    if (al::isFirstStep(this)) {
        mLastPlayingFileId = mGameDataHolder->getLastPlayingFileId();

        if (al::isNerve(this, &NrvTitleScenePlayerSelectNewGame)) {
            mGameDataHolder->setPlayingFileId(mTitleLogo->getNewFileId());
            GameDataFunction::initTotalPlayTimeOG(GameDataHolderAccessor(mGameDataHolder), mTitleLogo->getNewFileId());
        }

        if (!al::isNerve(this, &NrvTitleScenePlayerSelect) &&
            !al::isNerve(this, &NrvTitleScenePlayerSelectAfterDelete)) {
            pauseReplay();
            mScreenCaptureExecutor->requestCapture(false, 1, true);
        }

        mPlayerEntry->startCharacterSelect();
        mPlayerEntry->appearWithDecidePort(al::getMainControllerPort());
    }

    if (rc::isControllerAssignmentChanged()) {
        mPlayerEntry->resetButtonIcons();
    }

    if (mPlayerEntry->isRequestBack()) {
        mGameDataHolder->setPlayingFileId(mLastPlayingFileId);

        if (!al::isNerve(this, &NrvTitleScenePlayerSelect) && !isReplayActive()) {
            mScreenCaptureExecutor->offDraw(1);
            mGameDataHolder->setGameFileForTitleDemo(mTitleDemoFile);
        }

        if (mIsAppearFromLoad) {
            al::startSe(mPlayerEntry, "End");
            mControlGuideBar->endTitle(true);
            mIsAppearFromLoad = false;
            mScreenCaptureExecutor->requestCapture(true, 0, false);
            al::setNerve(this, &NrvTitleSceneRestartWhiteFade);
            return;
        }

        if (al::isNerve(this, &NrvTitleScenePlayerSelectNewGame) ||
            al::isNerve(this, &NrvTitleScenePlayerSelectAfterDelete) ||
            al::isNerve(this, &NrvTitleScenePlayerSelectResume)) {
            mControlGuideBar->endTitle(true);
            mScreenCaptureExecutor->offDraw(1);

            if (!isReplayActive()) {
                startReplay();
            }
        }

        if (mPlayerEntry->isFinishBack()) {
            mGameDataHolder->setGameFileForTitleDemo(mTitleDemoFile);

            if (al::isNerve(this, &NrvTitleScenePlayerSelect)) {
                mSaveDataLayout->setSelectLoad();
            }

            mPlayerEntry->kill();

            if (al::isNerve(this, &NrvTitleScenePlayerSelectNewGame) ||
                al::isNerve(this, &NrvTitleScenePlayerSelectAfterDelete) ||
                al::isNerve(this, &NrvTitleScenePlayerSelectResume)) {
                mScreenCaptureExecutor->offDraw(1);
                mTitleLogo->appearFromMenu();

                if (!isReplayActive()) {
                    startReplay();
                }

                al::setNerve(this, &NrvTitleSceneTitle);
                return;
            }

            mSaveDataLayout->validateButtons();
            mSaveDataLayout->setSelectLoad();
            mControlGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_FileSelect,
                                         al::getMainControllerPort(), true);
            al::setNerve(this, &NrvTitleSceneFileSelect);
            return;
        }
    }

    if (mPlayerEntry->isAllPlayerDecided()) {
        mPlayerEntry->hidePlayerAll();
        mScreenCaptureExecutor->offDraw(1);
        al::setNerve(this, &NrvTitleSceneWipeCloseCourseSelect);
    }
}

/**
 * Gets whether any demo player is still replaying its recorded input.
 * @return true if a pad replay is active.
 */
bool TitleScene::isReplayActive() const {
    bool isActive = false;

    for (s32 i = 0; i < mPlayerNum; i++) {
        bool isReplaying = al::isPadReplaying(getPadPort(i));
        isActive |= isReplaying;

        if (isReplaying) {
            break;
        }
    }

    return isActive;
}

/**
 * Returns from the character select to the title menu.
 */
void TitleScene::exeReturnPlayerSelect() {
    if (al::isFirstStep(this)) {
        startReplay();
    }

    if (mPlayerEntry->isFinishBack()) {
        mPlayerEntry->kill();
        mTitleLogo->appearFromMenu();
        mControlGuideBar->endTitle(true);
        GameDataFunction::isAllNewFile(GameDataHolderAccessor(mGameDataHolder));
        al::setNerve(this, &NrvTitleSceneTitle);
    }
}

/**
 * Fades out, then ends the scene to return to the top menu.
 */
void TitleScene::exeWipeCloseTopMenu() {
    if (al::isFirstStep(this)) {
        mStageWipeKeeper->closeWipeFadeBlack(-1);
    }

    if (mStageWipeKeeper->isCloseEndFadeBlack()) {
        mGameDataHolder->resetGameFileForTitleDemo();
        mTitleLogo->kill();
        kill();
    }
}

/**
 * Closes the retry wipe, then goes on to the menu the wipe was started for.
 */
void TitleScene::exeWipeClose() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvTitleSceneWipeCloseFileSelect)) {
            mControlGuideBar->appearTitle();
            al::setNerve(this, &NrvTitleSceneWaitTextFade);
            return;
        }

        mStageWipeKeeper->closeRetryWipe(false);
    }

    if (!mStageWipeKeeper->isCloseEndRetryWipe()) {
        return;
    }

    if (al::isNerve(this, &NrvTitleSceneWipeCloseAppearWait)) {
        mStageWipeKeeper->openRetryWipe();
        mSaveDataLayout->kill();
        al::setNerve(this, &NrvTitleSceneAppearWait);
    } else if (al::isNerve(this, &NrvTitleSceneWipeClosePlayerSelect)) {
        mStageWipeKeeper->openRetryWipe();
        mControlGuideBar->setCharacter("None");
        mControlGuideBar->overridePort(al::getMainControllerPort());

        if (mCourseSelectLayout->isAlive()) {
            mCourseSelectLayout->kill();
        }

        al::setNerve(this, &NrvTitleScenePlayerSelectNewGame);
    } else if (al::isNerve(this, &NrvTitleSceneWipeCloseCourseSelect)) {
        mStageWipeKeeper->openRetryWipe();
        al::setNerve(this, &NrvTitleSceneCourseSelect);
    }
}

/**
 * Fades out, then ends the scene to start the Luigi Bros mini game.
 */
void TitleScene::exeWipeCloseLuigiBros() {
    if (al::isFirstStep(this)) {
        mStageWipeKeeper->closeWipeFadeBlack(-1);
    } else if (mStageWipeKeeper->isCloseEndFadeBlack()) {
        al::stopSequenceBgm(this, "Title", 0);
        mGameDataHolder->resetGameFileForTitleDemo();
        kill();
    }
}

/**
 * Opens the scene's wipe, then goes on to the next nerve.
 */
void TitleScene::exeWipeOpen() {
    if (al::isFirstStep(this)) {
        mWipe->startOpen(-1);
    }

    if (!mWipe->isAlive()) {
        al::setNerve(this, mWipeOpenNextNerve);
    }
}

/**
 * Ends the scene to restart the title, with a white fade when the demo ended.
 */
void TitleScene::exeRestart() {
    if (al::isNerve(this, &NrvTitleSceneRestartWhiteFade)) {
        if (al::isFirstStep(this)) {
            mStageWipeKeeper->closeNoResultWipeRestartTitleWhiteFade(false);
            return;
        }

        if (!mStageWipeKeeper->isCloseEndNoResultWipe()) {
            return;
        }
    }

    mGameDataHolder->resetGameFileForTitleDemo();
    kill();
}

/**
 * Waits for the guide bar text to change, then opens the save file select.
 */
void TitleScene::exeWaitTextFade() {
    if (al::isFirstStep(this)) {
        mControlGuideBar->changeTextTitle(RCSControlGuideBar::GuideBarMsgType_FileSelect,
                                          al::getMainControllerPort());
        return;
    }

    if (mControlGuideBar->isChangingText()) {
        return;
    }

    if (!mSaveDataLayout->isAlive()) {
        mSaveDataLayout->appearLoad(al::getMainControllerPort());
    }

    mScreenCaptureExecutor->requestCapture(false, 1, true);
    al::setNerve(this, &NrvTitleSceneFileSelect);
}

/**
 * Runs the kiosk course select and starts the chosen course.
 */
void TitleScene::exeCourseSelect() {
    if (al::isFirstStep(this)) {
        mCourseSelectLayout->appear();
    }

    if (mCourseSelectLayout->isDecideBack()) {
        al::setNerve(this, &NrvTitleSceneWipeClosePlayerSelect);
    }

    if (!mCourseSelectLayout->isCourseDecided()) {
        return;
    }

    mGameDataHolder->setPlayingFileId(0);
    auto* sequence =
        static_cast<ProductSequence*>(GameSystemFunction::getGameSystem()->getSequence());

    if (sequence != nullptr) {
        sequence->setKioskStageStartParam(mCourseSelectLayout->getDecidedWorldId(),
                                          mCourseSelectLayout->getDecidedStageId());
        sequence->requestCaptureTopBottom();
        al::stopSequenceBgm(this, "Title", 120);
        kill();
    }
}

/**
 * Gets whether every player decided a character this frame.
 * @return true if every player decided.
 */
bool TitleScene::isAllPlayerDecideTrigger() const {
    return mPlayerEntry->isAllPlayerDecideStart();
}

/**
 * Gets whether the title screen is ready to be shown.
 * @return true if the title screen can be shown.
 */
bool TitleScene::isReadyShow() const {
    if (al::isNerve(this, &NrvTitleSceneTitle)) {
        return true;
    }

    if (al::isNerve(this, &NrvTitleSceneAppearWait) && al::isGreaterEqualStep(this, 1)) {
        return true;
    }

    if (!mIsAppearFromLoad) {
        return false;
    }

    return al::isNerve(this, &NrvTitleScenePlayerSelect);
}

/**
 * Gets whether the scene is ending to restart the title.
 * @return true if the title restarts.
 */
bool TitleScene::isRestart() const {
    return al::isNerve(this, &NrvTitleSceneRestart) ||
           al::isNerve(this, &NrvTitleSceneRestartWhiteFade);
}

/**
 * Gets whether the title restarts with a white fade.
 * @return true if the title restarts with a white fade.
 */
bool TitleScene::isRestartWhiteFade() const {
    return al::isNerve(this, &NrvTitleSceneRestartWhiteFade);
}

/**
 * Creates the players of every character for each player placement of the demo stage.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 * @param pSelector The retargetting selector of the players.
 */
void TitleScene::initPlacementPlayer(const al::StageInfo* pStageInfo,
                                     const al::ActorInitInfo& rInfo,
                                     PlayerRetargettingSelector* pSelector) {
    al::PlacementInfo placementInfo;
    s32 count = 0;
    al::getPlacementInfoAndCount(&placementInfo, &count, pStageInfo, "PlayerList");
    static_cast<PlayerGroupSceneObj*>(al::createSceneObj(this, SceneObjID_PlayerGroup))
        ->initGroup(128);
    al::setSceneObj(this, new PlayerProcess(mLiveActorKit->getPlayerHolder()), 17);

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo info;
        al::getPlacementInfoByIndex(&info, placementInfo, i);
        al::ActorInitInfo actorInfo;
        actorInfo.initNoViewId(&info, rInfo);

        for (s32 j = 0; j < rc::getPlayerCharacterNumMax(); j++) {
            PlayerActor* player =
                createPlayer(actorInfo, &mLiveActorKit->getCameraDirector()->mMainViewMtx,
                             rc::getPlayerCharacterName(j), pSelector);
            rc::deactivatePlayer(player);
            player->createInterfaceSilhouetteHiddenFlag()->turnOn();
            alPlayerFunction::registerPlayer(player, player->getPadRumbleKeeper(), true);
            mPlayers.pushBack(player);
        }
    }
}

/**
 * Creates the sky actors listed in the stage's SkyList.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 */
void TitleScene::initPlacementSky(const al::StageInfo* pStageInfo,
                                  const al::ActorInitInfo& rInfo) {
    al::PlacementInfo placementInfo;
    al::ByamlIter skyListIter;

    if (!pStageInfo->getPlacementIter().tryGetIterByKey(&skyListIter, "SkyList")) {
        return;
    }

    placementInfo.set(skyListIter, pStageInfo->getZoneIter(), pStageInfo->getParentInfo(),
                      pStageInfo->getID());
    s32 count = al::getCountPlacementInfo(placementInfo);
    ProjectActorFactory factory;

    for (s32 i = 0; i < count; i++) {
        al::PlacementInfo info;
        al::getPlacementInfoByIndex(&info, placementInfo, i);
        al::createPlacementActorFromFactory(factory, rInfo, &info);
    }
}

/**
 * Creates the actors listed in the stage's ObjectList.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 * @param pListName The placement list name (unused, ObjectList is always used).
 */
void TitleScene::initPlacementObject(const al::StageInfo* pStageInfo,
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

namespace TitleSceneFunction {

s32 getTitleDemoIdRosetta() {
    return cTitleDemoIdRosetta;
}

s32 getTitleDemoIdSuperPlay() {
    return 4;
}

s32 getTitleDemoNumDefault() {
    return 4;
}

s32 getTitleDemoNumIncludeSuperPlay() {
    return 5;
}

s32 getTitleDemoNumAll() {
    return 6;
}

}  // namespace TitleSceneFunction

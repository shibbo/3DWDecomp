#include "Scene/DemoScene.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Camera/CameraPoserFactory.hpp"
#include "Demo/DemoPlayerModelDirector.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/CameraPoserSceneInfo_RS.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Draw/GraphicsInitArg.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Projection/Projection.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "System/Application.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

class PlayerRetargettingSelector;

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
DemoSceneActorHolder* createDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
                                            PlayerRetargettingSelector* pSelector,
                                            const sead::Matrix34f* pMtx, bool flag, int count);
bool tryStartDemo(DemoSceneActorHolder* pDemo);
}  // namespace rc

namespace {
NERVE_DECL(DemoScene, Start)
NERVE_DECL(DemoScene, Play)
NERVES_MAKE_NOSTRUCT(DemoScene, Start, Play)

/**
 * Invisible actor registered as the player so that player-dependent systems work during the demo.
 */
class DummyPlayer : public al::LiveActor {
public:
    /**
     * Constructs the dummy player.
     * @param pName The actor name.
     */
    DummyPlayer(const char* pName) : al::LiveActor(pName) {}

    /**
     * Initializes the scene info, executor and pose of the actor, then makes it alive.
     * @param rInfo The actor init info.
     */
    void init(const al::ActorInitInfo& rInfo) override {
        al::initActorSceneInfo(this, rInfo);
        al::initExecutorWatchObj(this, rInfo);
        al::initActorPoseTRSV(this);
        makeActorAppeared();
    }
};

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}
}  // namespace

/**
 * Constructs the demo scene.
 * @param unused Unused flag.
 * @param isNoBgm Whether the stage BGM must not be started when the scene appears.
 */
DemoScene::DemoScene(bool unused, bool isNoBgm) : al::Scene("デモシーン"), mIsNoBgm(isNoBgm) {}

/**
 * Destroys the demo scene.
 */
DemoScene::~DemoScene() {
    rc::setMainPlayerActor(nullptr);
    getFramework()->mIsClearRenderBuffer = true;

    if (mLiveActorKit != nullptr) {
        mLiveActorKit->getEffectSystem()->endScene();
    }
}

/**
 * Initializes the scene: audio, kits, cameras, layouts and placement of the demo stage.
 * @param rInfo The scene init info.
 */
void DemoScene::init(const al::SceneInitInfo& rInfo) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mIsUseCameraRS = GameDataFunction::isSingleMode(GameDataHolderAccessor(mGameDataHolder));
    mStageName = rInfo.mStageName;
    initSceneStopCtrl();
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    al::setSceneObj(this, mGameDataHolder, 8);

    mMainViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(6));
    mSubViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(9));
    initSceneAudio(rInfo, mStageName.cstr(), 60, 30, 1, false, "Scene", 20, 1.0f);

    al::GraphicsInitArg graphicsArg;
    graphicsArg.mViewRendererCreator = new al::ViewRendererCreator();
    graphicsArg.setViewNum(2);
    graphicsArg.mIsUsingViewRenderer = true;
    initLiveActorKitWithGraphics(graphicsArg, rInfo, 1024, rc::getControlUserNumMax(), 2, 0,
                                 mIsUseCameraRS, false);
    mLiveActorKit->initHitSensorDirector(1, false);

    auto* demoDirector = new ProjectDemoDirector(mLiveActorKit->getPlayerHolder(), -1);

    if (mIsUseCameraRS) {
        demoDirector->getPlayerModelDirector()->requestCreateAllFigure(0, nullptr, true);
    }

    mLiveActorKit->mDemoDirector = demoDirector;

    const sead::LookAtCamera* camera;
    const sead::PerspectiveProjection* projection;

    if (mIsUseCameraRS) {
        camera = &mLiveActorKit->getCameraDirector_RS()->getLookAtMain();
        projection = static_cast<const sead::PerspectiveProjection*>(
            &mLiveActorKit->getCameraDirector_RS()->getProjectionMain());
    } else {
        camera = &getSceneCameraInfo()->getViewAt(0)->getLookAtCam();
        projection = &getSceneCameraInfo()->getViewAt(0)->getProjection().getProjectionSead();
        mLiveActorKit->getCameraDirector()->setCameraAspect(mMainViewport, mSubViewport);
        mLiveActorKit->getCameraDirector()->setStageName(mStageName.cstr());
    }

    initSceneAudio3D(rInfo, &camera->getPos(), &camera->getMatrix(), projection, &camera->getAt(),
                     "ターゲット寄り中間", mLiveActorKit->getAreaObjDirector(), false);
    initAudioKeeper(nullptr);
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());

    if (mIsUseLayout) {
        initLayoutKit(rInfo);
    }

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::setSceneObj(this, new CoinRotater(mLiveActorKit->getExecuteDirector()), 1);
    initAndLoadStageResource(mStageName.cstr(), 1);

    if (mIsUseCameraRS) {
        mCameraPoserSceneInfo = new al::CameraPoserSceneInfo_RS();
        mCameraPoserSceneInfo->init(mLiveActorKit->getAreaObjDirector(),
                                    mLiveActorKit->getCollisionDirector(), mAudioDirector);
        auto* cameraPoserFactory = new al::CameraPoserFactory("CameraPoserFactory");
        al::initCameraDirector_RS(this, mStageName.cstr(), cameraPoserFactory,
                                  mLiveActorKit->getCameraDirector()->getSceneCameraInfo());
        mLiveActorKit->getCameraDirector()->reviseCameraInfo(
            mLiveActorKit->getCameraDirector_RS()->getSceneCameraInfo());
    }

    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, mIsUseCameraRS);
    initPlacement(actorInfo);

    if (!mIsUseCameraRS) {
        mLiveActorKit->getCameraDirector()->init(mLiveActorKit->getPlayerHolder());
        mLiveActorKit->getCameraDirector()->initAudioKeeper(actorInfo);
    }

    if (mIsUseLayout) {
        mDemoSkipLayout = new DemoSkipLayout(layoutInfo, mIsUseCameraRS);
        mDemoSkipLayout->appear();
    }

    endInit(actorInfo, nullptr);

    if (mIsUseCameraRS) {
        mLiveActorKit->getCameraDirector_RS()->endInit(mLiveActorKit->getPlayerHolder(), -1,
                                                       true);
    }

    initNerve(&NrvDemoSceneStart, 0);
}

/**
 * Makes the scene appear, starts the stage BGM and stops the framework from clearing the render
 * buffer.
 */
void DemoScene::appear() {
    mIsSkipped = false;
    al::Scene::appear();

    if (!mIsNoBgm) {
        al::startBgm(this, "Stage", -1, 0, -1, -1);
    }

    getFramework()->mIsClearRenderBuffer = false;
}

/**
 * Updates the graphics, camera and kit.
 */
void DemoScene::control() {
    mLiveActorKit->preDrawGraphics();

    if (al::isStopScene(this)) {
        return;
    }

    if (mIsUseCameraRS) {
        mLiveActorKit->getCameraDirector_RS()->execute();
    } else {
        mLiveActorKit->getCameraDirector()->update(false);
    }

    al::updateKit(this);
}

/**
 * Draws the 3D view and the 2D layouts to the main screen.
 */
void DemoScene::drawMain_() const {
    mMainViewport->setByFrameBuffer(*getFramework()->getCurrentRenderBuffer());
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFramework()->mIsDocked);
    mLayoutKit->setFrameBuffer(getFramework()->getCurrentRenderBuffer(), mMainViewport);
    alSystemKitFunction::applyViewportTop(*mMainViewport);

    al::LiveActorKit* kit = mLiveActorKit;
    al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
    viewRenderer->drawView(0, 0, kit, getSceneCameraInfo(),
                           getFramework()->getCurrentRenderBuffer(), *mMainViewport, true, false,
                           static_cast<agl::ShaderMode>(4));
    al::drawKit(this, "２Ｄベース（メイン画面）");

    sead::PerspectiveProjection* projection;

    if (mIsUseCameraRS) {
        projection = const_cast<sead::PerspectiveProjection*>(
            static_cast<const sead::PerspectiveProjection*>(
                &mLiveActorKit->getCameraDirector_RS()->getProjectionMain()));
    } else {
        projection = mLiveActorKit->getCameraDirector()->getProjection();
    }

    projection->setOffset(sead::Vector2f::zero);
}

/**
 * Draws nothing to the sub screen.
 */
void DemoScene::drawSub_() const {}

/**
 * Starts the demo and switches to playing it.
 */
void DemoScene::exeStart() {
    if (mDemoActorHolder != nullptr) {
        rc::tryStartDemo(mDemoActorHolder);
        mDemoActorHolder->startAction(0, false);
    }

    al::setNerve(this, &NrvDemoScenePlay);
}

/**
 * Plays the demo until its camera ends or it is skipped, then kills the scene.
 */
void DemoScene::exePlay() {
    if (mIsUseLayout) {
        u64 activePorts = rc::getActiveInputPortList(GameDataHolderAccessor(this));

        if (mDemoSkipLayout->isSkip(activePorts)) {
            if (mIsCancelAudioOnSkip) {
                mDemoActorHolder->tryCancelAudio(30, true);
            }

            mDemoActorHolder->kill();
            kill();
            mIsSkipped = true;
            return;
        }
    }

    if (mDemoActorHolder->isActionEndCamera(0)) {
        mDemoActorHolder->kill();
        kill();
    }
}

/**
 * Gets the length of the demo.
 * @return The last camera frame of the demo.
 */
s32 DemoScene::getDemoLength() const {
    return mDemoActorHolder->getMaxCameraFrame();
}

/**
 * Places the graphics resources, area objects, cameras, dummy player, sky, objects and demo of
 * the stage.
 * @param rInfo The actor init info.
 */
void DemoScene::initPlacement(const al::ActorInitInfo& rInfo) {
    al::GraphicsSystemInfo* graphicsInfo = mLiveActorKit->getGraphicsSystemInfo();
    graphicsInfo->initStageResource(al::tryGetStageResourceDesign(this, 0), mStageName.cstr(),
                                    mLiveActorKit, false, 0);
    // The factory is unused in the original code as well.
    ProjectActorFactory factory;
    al::initPlacementAreaObj(this, rInfo, nullptr);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
    s32 mapNum = al::getStageInfoMapNum(this);
    mLiveActorKit->getCameraDirector()->initCameraCreator(mapNum, al::isStageOneResource(this));

    for (s32 i = 0; i < mapNum; i++) {
        mLiveActorKit->getCameraDirector()->setCameraResource(
            al::getStageInfoMap(this, i)->getResource(), i);
    }

    auto* dummyPlayer = new DummyPlayer("DummyPlayer");
    dummyPlayer->init(rInfo);
    alPlayerFunction::registerPlayer(
        dummyPlayer, al::createPadRumbleKeeper(dummyPlayer, al::getMainControllerPort()), true);
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

    initPlacementDemo(al::getStageInfoMap(this, 0), rInfo);
}

/**
 * Creates the sky actors listed in the stage's SkyList.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 */
void DemoScene::initPlacementSky(const al::StageInfo* pStageInfo,
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
void DemoScene::initPlacementObject(const al::StageInfo* pStageInfo,
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

/**
 * Creates the demo actor holder of the stage if the stage has a DemoObjList.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 */
void DemoScene::initPlacementDemo(const al::StageInfo* pStageInfo,
                                  const al::ActorInitInfo& rInfo) {
    al::PlacementInfo placementInfo;

    if (!al::tryGetPlacementInfo(&placementInfo, pStageInfo, "DemoObjList")) {
        return;
    }

    // The first selector is unused in the original code as well.
    rc::createPlayerRetargettingSelector(this);
    mDemoActorHolder =
        rc::createDemoSceneHolder(mStageName.cstr(), rInfo,
                                  rc::createPlayerRetargettingSelector(this), nullptr, false, 4);
}

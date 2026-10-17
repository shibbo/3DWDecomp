#include "Scene/DemoSingleModeEndingScene.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadColor.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Camera/CameraPoserFactory.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Layout/StaffRollLayoutHolder.hpp"
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
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "System/Application.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/SaveDataAccessFunction.hpp"
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
NERVE_DECL(DemoSingleModeEndingScene, EndRoll)
NERVE_DECL(DemoSingleModeEndingScene, Cancel)
NERVE_DECL(DemoSingleModeEndingScene, ThanksForPlaying)
NERVES_MAKE_NOSTRUCT(DemoSingleModeEndingScene, EndRoll, Cancel, ThanksForPlaying)

/** Step of the staff roll at which the screen starts fading out. */
const s32 cEndRollFadeStep = 7380;
/** Number of steps the "Thanks for playing" screen is shown before fading out. */
const s32 cThanksForPlayingStep = 420;

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}
}  // namespace

/**
 * Constructs the Bowser's Fury ending demo scene.
 */
DemoSingleModeEndingScene::DemoSingleModeEndingScene()
    : al::Scene("エンディングデモシーン"), mStageName("") {}

/**
 * Destroys the Bowser's Fury ending demo scene.
 */
DemoSingleModeEndingScene::~DemoSingleModeEndingScene() {
    rc::setMainPlayerActor(nullptr);
    getFramework()->mIsClearRenderBuffer = true;

    if (mLiveActorKit != nullptr) {
        mLiveActorKit->getEffectSystem()->endScene();
    }
}

/**
 * Initializes the scene: stage resources, audio, kits, layouts, cameras and placement, then
 * records the ending as seen and saves.
 * @param rInfo The scene init info.
 */
void DemoSingleModeEndingScene::init(const al::SceneInitInfo& rInfo) {
    initNerve(&NrvDemoSingleModeEndingSceneEndRoll, 0);
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mGameDataHolder->setSingleMode(true);
    al::StringTmp<64> sceneStageName(rInfo.mStageName);
    mStageName = sceneStageName;
    initAndLoadStageResource(mStageName.cstr(), 1);
    initSceneStopCtrl();
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    al::setSceneObj(this, mGameDataHolder, 8);

    mMainViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(6));
    mSubViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(9));
    initSceneAudio(rInfo, mStageName.cstr(), 60, 30, 1, false, "SingleModeScene", 20, 1.0f);

    al::GraphicsInitArg graphicsArg;
    graphicsArg.mViewRendererCreator = new al::ViewRendererCreator();
    graphicsArg.setViewNum(2);
    graphicsArg.mIsUsingViewRenderer = true;
    initLiveActorKitWithGraphics(graphicsArg, rInfo, 256, rc::getControlUserNumMax(), 2, 0, true,
                                 false);
    mLiveActorKit->initHitSensorDirector(1, false);

    al::LiveActorKit* kit = mLiveActorKit;
    kit->mDemoDirector = new ProjectDemoDirector(kit->getPlayerHolder(), -1);

    const sead::LookAtCamera& camera = mLiveActorKit->getCameraDirector_RS()->getLookAtMain();
    initSceneAudio3D(rInfo, &camera.getPos(), &camera.getMatrix(),
                     static_cast<const sead::PerspectiveProjection*>(
                         &mLiveActorKit->getCameraDirector_RS()->getProjectionMain()),
                     &camera.getAt(), "ターゲット寄り中間", mLiveActorKit->getAreaObjDirector(),
                     false);
    initAudioKeeper(nullptr);
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());
    mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer()->setFrameBufferDrawer(this);
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    mWipe = new al::WipeSimple("黒フェード", "WipeFadeBlack", layoutInfo, nullptr);
    mDemoSkipLayout = new DemoSkipLayout(layoutInfo, true);
    mDemoSkipLayout->appear();
    mThanksForPlayingLayout = new al::LayoutActor("ThanksForPlaying");
    al::initLayoutActor(mThanksForPlayingLayout, layoutInfo, "ThankYouForPlaying", nullptr);

    al::GraphicsSystemInfo* graphicsInfo = mLiveActorKit->getGraphicsSystemInfo();
    graphicsInfo->mAreaTarget = 2;
    const al::Resource* resource = al::getStageInfoDesign(this, 0)->getResource();
    graphicsInfo->initStageResource(resource, mStageName.cstr(), mLiveActorKit, false, 0);

    mCameraPoserSceneInfo = new al::CameraPoserSceneInfo_RS();
    mCameraPoserSceneInfo->init(mLiveActorKit->getAreaObjDirector(),
                                mLiveActorKit->getCollisionDirector(), mAudioDirector);
    auto* cameraPoserFactory = new al::CameraPoserFactory("CameraPoserFactory");
    al::initCameraDirector_RS(this, mStageName.cstr(), cameraPoserFactory,
                              mLiveActorKit->getCameraDirector()->getSceneCameraInfo());
    mLiveActorKit->getCameraDirector()->reviseCameraInfo(
        mLiveActorKit->getCameraDirector_RS()->getSceneCameraInfo());
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, true);
    // The stage name is fetched here but its value is unused in the original code as well.
    mStageName.cstr();
    initPlacement(actorInfo);

    mStaffRollLayoutHolder = new StaffRollLayoutHolder(layoutInfo, true);
    al::setSceneObj(this, mStaffRollLayoutHolder, 32);
    endInit(actorInfo, nullptr);

    SingleModeDataFunction::setPhase4DarkBowserHitPoint(GameDataHolderAccessor(mGameDataHolder),
                                                        -1);
    SingleModeDataFunction::setPhase4DarkBowserHitPointPreBoss(
        GameDataHolderAccessor(mGameDataHolder));

    if (SingleModeDataFunction::isAllShineCollected(GameDataHolderAccessor(mGameDataHolder)) &&
        !SingleModeDataFunction::isCompleteEndingPictureSeen(
            GameDataHolderAccessor(mGameDataHolder))) {
        SingleModeDataFunction::setCompleteEndingPictureSeen(
            GameDataHolderAccessor(mGameDataHolder));
    }

    SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
}

/**
 * Places the area objects, sky, objects and the ending demo of the stage.
 * @param rInfo The actor init info.
 */
void DemoSingleModeEndingScene::initPlacement(const al::ActorInitInfo& rInfo) {
    al::initPlacementAreaObj(this, rInfo, nullptr);
    rc::initAreaObjIndex(mLiveActorKit->getAreaObjDirector());
    s32 mapNum = al::getStageInfoMapNum(this);
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
 * Makes the scene appear and stops the framework from clearing the render buffer.
 */
void DemoSingleModeEndingScene::appear() {
    al::Scene::appear();
    getFramework()->mIsClearRenderBuffer = false;
}

/**
 * Kills the scene and the ending demo.
 */
void DemoSingleModeEndingScene::kill() {
    al::Scene::kill();
    mDemoActorHolder->kill();
}

/**
 * Updates the graphics, camera, kit and staff roll.
 */
void DemoSingleModeEndingScene::control() {
    mLiveActorKit->preDrawGraphics();

    if (al::isStopScene(this)) {
        return;
    }

    mLiveActorKit->getCameraDirector_RS()->execute();
    al::updateKit(this);
    mStaffRollLayoutHolder->update();
}

/**
 * Draws the layouts drawn above the blur into the given frame buffer.
 * @param pRenderBuffer The render buffer to draw into.
 * @param pViewport The viewport to draw with.
 */
void DemoSingleModeEndingScene::drawToFrameBuffer(const agl::RenderBuffer* pRenderBuffer,
                                                  const sead::Viewport* pViewport) {
    mLayoutKit->setFrameBuffer(pRenderBuffer, pViewport);
    al::drawKit(this, "2DDrawAboveBlur2");
}

/**
 * Clears the screen, then draws the 3D view and the 2D layouts to the main screen.
 */
void DemoSingleModeEndingScene::drawMain_() const {
    agl::RenderBuffer* renderBuffer = getFramework()->getCurrentRenderBuffer();
    mMainViewport->setByFrameBuffer(*renderBuffer);
    renderBuffer->fastClear(al::GameFrameworkNx::getAglDrawContext(), 0, 7,
                            sead::Color4f::cBlack, 1.0f, 0, sead::Viewport(*renderBuffer), true);
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFramework()->mIsDocked);
    alSystemKitFunction::applyViewportTop(*mMainViewport);

    al::LiveActorKit* kit = mLiveActorKit;
    al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
    viewRenderer->drawView(0, 0, kit, kit->getCameraDirector_RS()->getSceneCameraInfo(),
                           getFramework()->getCurrentRenderBuffer(), *mMainViewport, true, false,
                           static_cast<agl::ShaderMode>(4));
    mLayoutKit->setFrameBuffer(renderBuffer, mMainViewport);
    al::drawKit(this, "２Ｄベース（メイン画面）");

    auto& projection = const_cast<sead::PerspectiveProjection&>(
        static_cast<const sead::PerspectiveProjection&>(
            mLiveActorKit->getCameraDirector_RS()->getProjectionMain()));
    projection.setOffset(sead::Vector2f::zero);
}

/**
 * Draws nothing to the sub screen.
 */
void DemoSingleModeEndingScene::drawSub_() const {}

/**
 * Plays the ending demo with the staff roll until the screen has faded out or it is skipped.
 */
void DemoSingleModeEndingScene::exeEndRoll() {
    if (al::isFirstStep(this)) {
        appearDemoActorHolder();
        al::startBgm(this, "StaffRollSingleMode", 0, 40, -1, -1);
    }

    if (al::isStep(this, 0)) {
        mStaffRollLayoutHolder->startAppear();
    } else if (al::isGreaterEqualStep(this, cEndRollFadeStep)) {
        if (al::isStep(this, cEndRollFadeStep)) {
            mWipe->startClose(-1);
        }
    } else if (mDemoSkipLayout->isSkip(sead::BitFlag<u16>(1 << al::getMainControllerPort()))) {
        al::setNerve(this, &NrvDemoSingleModeEndingSceneCancel);
        return;
    }

    if (mGameDataHolder->isSingleMode() && mWipe->isCloseEnd()) {
        mDemoActorHolder->kill();
        al::setNerve(this, &NrvDemoSingleModeEndingSceneThanksForPlaying);
    }
}

/**
 * Starts the ending demo.
 */
void DemoSingleModeEndingScene::appearDemoActorHolder() {
    rc::tryStartDemo(mDemoActorHolder);
    mDemoActorHolder->startAction(0, false);
}

/**
 * Stops the staff roll after a skip and shows the "Thanks for playing" screen once the wipe has
 * closed.
 */
void DemoSingleModeEndingScene::exeCancel() {
    if (al::isFirstStep(this)) {
        al::stopBgm(this, "StaffRollSingleMode", 70, -1);
        mWipe->startClose(-1);
        mDemoSkipLayout->kill();
    }

    if (mWipe->isCloseEnd()) {
        mDemoActorHolder->kill();
        al::setNerve(this, &NrvDemoSingleModeEndingSceneThanksForPlaying);
    }
}

/**
 * Shows the "Thanks for playing" screen with the collected Cat Shine count, then fades out and
 * ends the scene.
 */
void DemoSingleModeEndingScene::exeThanksForPlaying() {
    if (al::isFirstStep(this)) {
        mWipe->startOpen(-1);

        if (SingleModeDataFunction::isAllShineCollected(GameDataHolderAccessor(mGameDataHolder))) {
            al::startAction(mThanksForPlayingLayout, "Incomplete_Hide", "Incomplete");
        } else {
            al::startAction(mThanksForPlayingLayout, "Incomplete", "Incomplete");
        }

        sead::WFormatFixedSafeString<16> shineCount(
            u"%03d",
            SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(mGameDataHolder)));
        al::setPaneString(mThanksForPlayingLayout, "TxtShineCounter", shineCount.cstr());
        al::startAction(mThanksForPlayingLayout, "Appear");
        mThanksForPlayingLayout->appear();
    }

    if (al::isLessStep(this, cThanksForPlayingStep)) {
        return;
    }

    if (al::isStep(this, cThanksForPlayingStep)) {
        mWipe->startClose(-1);
        return;
    }

    if (mWipe->isCloseEnd()) {
        kill();
    }
}

/**
 * Creates the sky actors listed in the stage's SkyList.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 */
void DemoSingleModeEndingScene::initPlacementSky(const al::StageInfo* pStageInfo,
                                                 const al::ActorInitInfo& rInfo) {
    al::PlacementInfo placementInfo;
    al::ByamlIter skyListIter;

    if (!pStageInfo->getPlacementIter().tryGetIterByKey(&skyListIter, "SkyList")) {
        return;
    }

    placementInfo.set(skyListIter, pStageInfo->getZoneIter(), pStageInfo->getParentInfo(),
                      pStageInfo->getID());
    ProjectActorFactory factory;

    for (s32 i = 0; i < al::getCountPlacementInfo(placementInfo); i++) {
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
void DemoSingleModeEndingScene::initPlacementObject(const al::StageInfo* pStageInfo,
                                                    const al::ActorInitInfo& rInfo,
                                                    const char* pListName) {
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
 * Creates the ending demo actor holder.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 */
void DemoSingleModeEndingScene::initPlacementDemo(const al::StageInfo* pStageInfo,
                                                  const al::ActorInitInfo& rInfo) {
    al::PlacementInfo placementInfo;
    al::tryGetPlacementInfo(&placementInfo, pStageInfo, "DemoObjList");
    PlayerRetargettingSelector* selector = rc::createPlayerRetargettingSelector(this);
    mDemoActorHolder =
        rc::createDemoSceneHolder(mStageName.cstr(), rInfo, selector, nullptr, false, 4);
}

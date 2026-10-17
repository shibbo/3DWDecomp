#include "Scene/DemoOpeningScene.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Draw/GraphicsInitArg.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/PostProcessing/DepthOfFieldDrawer.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/DemoOpeningState.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "System/Application.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DemoOpeningScene, Play)
NERVES_MAKE_NOSTRUCT(DemoOpeningScene, Play)
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}
}  // namespace

DemoOpeningScene::DemoOpeningScene() : al::Scene("デモシーン") {}

DemoOpeningScene::~DemoOpeningScene() {
    rc::setMainPlayerActor(nullptr);
    getFramework()->mIsClearRenderBuffer = true;

    if (mLiveActorKit != nullptr) {
        mLiveActorKit->getEffectSystem()->endScene();
    }
}

void DemoOpeningScene::init(const al::SceneInitInfo& rInfo) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mStageName = "DemoOpeningStage";
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
    initLiveActorKitWithGraphics(graphicsArg, rInfo, 1024, rc::getControlUserNumMax(), 2, 0, false, false);
    mLiveActorKit->initHitSensorDirector(1, false);
    al::LiveActorKit* kit = mLiveActorKit;
    kit->mDemoDirector = new ProjectDemoDirector(kit->getPlayerHolder(), -1);
    mLiveActorKit->getCameraDirector()->setCameraAspect(mMainViewport, mSubViewport);
    mLiveActorKit->getCameraDirector()->setStageName(mStageName.cstr());
    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    sead::LookAtCamera* camera = cameraDirector->getLookAtCamera();
    initSceneAudio3D(rInfo, &camera->getPos(), &camera->getMatrix(), cameraDirector->getProjection(),
                    &camera->getAt(), "ターゲット寄り中間", mLiveActorKit->getAreaObjDirector(), false);
    initAudioKeeper(nullptr);
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());
    initLayoutKit(rInfo);
    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    al::setSceneObj(this, new CoinRotater(mLiveActorKit->getExecuteDirector()), 1);
    initPlacement(actorInfo);
    al::DepthOfFieldDrawer* depthOfField = mLiveActorKit->getGraphicsSystemInfo()->mDepthOfFieldDrawer;
    depthOfField->init(mLiveActorKit->getAreaObjDirector(), actorInfo.getActorSceneInfo().sceneCameraInfo,
                       mLiveActorKit->getPlayerHolder());
    depthOfField->setStageName(mStageName.cstr());
    initNerve(&NrvDemoOpeningScenePlay, 1);
    mOpeningState = new DemoOpeningState(this, actorInfo, mLiveActorKit->getGraphicsSystemInfo(), nullptr);
    al::initNerveState(this, mOpeningState, &NrvDemoOpeningScenePlay, "DemoOpening");
    mLiveActorKit->getCameraDirector()->init(mLiveActorKit->getPlayerHolder());
    mLiveActorKit->getCameraDirector()->initAudioKeeper(actorInfo);
    endInit(actorInfo, nullptr);
}

void DemoOpeningScene::appear() {
    al::Scene::appear();
    getFramework()->mIsClearRenderBuffer = false;
}
void DemoOpeningScene::control() {
    mLiveActorKit->preDrawGraphics();
    if (!al::isStopScene(this)) {
        mLiveActorKit->getCameraDirector()->update(false);
        al::updateKit(this);
    }
}
void DemoOpeningScene::drawMain_() const {
    mMainViewport->setByFrameBuffer(*getFramework()->getCurrentRenderBuffer());
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(getFramework()->mIsDocked);
    alSystemKitFunction::applyViewportTop(*mMainViewport);
    al::LiveActorKit* kit = mLiveActorKit;
    al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
    viewRenderer->drawView(0, 0, kit, getSceneCameraInfo(), getFramework()->getCurrentRenderBuffer(),
                           *mMainViewport, true, false, static_cast<agl::ShaderMode>(4));
    al::drawKit(this, "２Ｄベース（メイン画面）");
    mLiveActorKit->getCameraDirector()->getProjection()->setOffset(sead::Vector2f::zero);
}
void DemoOpeningScene::drawSub_() const {}
void DemoOpeningScene::exePlay() {
    if (al::updateNerveState(this))
        kill();
}

void DemoOpeningScene::initPlacement(const al::ActorInitInfo& rInfo) {
    initAndLoadStageResource(mStageName.cstr(), 1);
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

void DemoOpeningScene::initPlacementSky(const al::StageInfo* pStageInfo,
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

void DemoOpeningScene::initPlacementObject(const al::StageInfo* pStageInfo,
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



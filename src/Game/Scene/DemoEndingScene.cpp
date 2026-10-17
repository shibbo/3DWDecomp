#include "Scene/DemoEndingScene.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <gfx/seadViewport.h>
#include "AreaObj/ProjectAreaObjFactory.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Layout/StaffRollLayoutHolder.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Draw/GraphicsInitArg.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
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
#include "Project/Base/StringUtil.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "System/Application.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

class PlayerRetargettingSelector;

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
DemoSceneActorHolder* createDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
                                            PlayerRetargettingSelector* pSelector,
                                            const sead::Matrix34f* pMtx, bool flag, int count);
bool tryStartDemo(DemoSceneActorHolder* pDemo);
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pDemo,
                                     const GameDataHolder* pGameDataHolder);
}  // namespace rc

namespace {
NERVE_DECL(DemoEndingScene, EndRoll)
NERVE_DECL(DemoEndingScene, Cancel)
NERVE_DECL(DemoEndingScene, EndRollEnd)
NERVES_MAKE_NOSTRUCT(DemoEndingScene, EndRoll, Cancel, EndRollEnd)

/** Demo stage names used once the Rosetta (Bowser's Fury) content has been opened. */
const char* const cRosettaDemoNames[] = {"DemoEndRollRosettaStage", "DemoEndRollEndStage"};
/** Demo stage names used before the Rosetta content has been opened. */
const char* const cDemoNames[] = {"DemoEndRollStage", "DemoEndRollEndStage"};

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}

/**
 * Gets whether the Rosetta content has been opened.
 * @param pGameDataHolder The game data holder.
 * @return true if the Rosetta content has been opened.
 */
bool isRosetta(GameDataHolder* pGameDataHolder) {
    return GameDataFlagFunction::isAlreadyOpenRosetta(GameDataHolderAccessor(pGameDataHolder));
}

/**
 * Gets the demo stage name list for the current save state.
 * @param pGameDataHolder The game data holder.
 * @return The demo stage name list.
 */
const char* const* getDemoNames(GameDataHolder* pGameDataHolder) {
    return isRosetta(pGameDataHolder) ? cRosettaDemoNames : cDemoNames;
}
}  // namespace

/**
 * Constructs the ending demo scene.
 */
DemoEndingScene::DemoEndingScene() : al::Scene("エンディングデモシーン"), mStageName("") {
    mDemoActorHolders = new DemoSceneActorHolder*[2];
}

/**
 * Destroys the ending demo scene.
 */
DemoEndingScene::~DemoEndingScene() {
    rc::setMainPlayerActor(nullptr);
    getFramework()->mIsClearRenderBuffer = true;

    if (mLiveActorKit != nullptr) {
        mLiveActorKit->getEffectSystem()->endScene();
    }
}

/**
 * Initializes the scene: stage resources, audio, kits, layouts and placement.
 * @param rInfo The scene init info.
 */
void DemoEndingScene::init(const al::SceneInitInfo& rInfo) {
    initNerve(&NrvDemoEndingSceneEndRoll, 0);
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    al::StringTmp<64> sceneStageName(rInfo.mStageName);
    mStageName = isRosetta(mGameDataHolder) ? "DemoEndRollRosettaStage" : "DemoEndRollStage";
    initAndLoadStageResource(mStageName.cstr(), 1);

    al::StringTmp<64> preLoadName(
        "%s%d", isRosetta(mGameDataHolder) ? "DemoEndingRosettaStage" : "DemoEndingStage", 1);
    al::tryRequestPreLoadFile(
        al::getPreLoadFileListArc(), preLoadName, nullptr,
        static_cast<al::IAudioResourceLoader*>(rInfo.mGameSystemInfo->_0));
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
    initLiveActorKitWithGraphics(graphicsArg, rInfo, 256, rc::getControlUserNumMax(), 2, 0, false,
                                 false);
    mLiveActorKit->initHitSensorDirector(1, false);

    al::LiveActorKit* kit = mLiveActorKit;
    kit->mDemoDirector = new ProjectDemoDirector(kit->getPlayerHolder(), -1);
    mLiveActorKit->getCameraDirector()->setCameraAspect(mMainViewport, mSubViewport);
    mLiveActorKit->getCameraDirector()->setStageName(mStageName.cstr());

    al::CameraDirector* cameraDirector = mLiveActorKit->getCameraDirector();
    sead::LookAtCamera* camera = cameraDirector->getLookAtCamera();
    initSceneAudio3D(rInfo, &camera->getPos(), &camera->getMatrix(),
                     cameraDirector->getProjection(), &camera->getAt(), "ターゲット寄り中間",
                     mLiveActorKit->getAreaObjDirector(), false);
    initAudioKeeper(nullptr);
    mLiveActorKit->getAreaObjDirector()->init(new ProjectAreaObjFactory());
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    mWipe = new al::WipeSimple("黒フェード", "WipeFadeBlack", layoutInfo, nullptr);
    mDemoSkipLayout = new DemoSkipLayout(layoutInfo, false);
    mDemoSkipLayout->appear();

    al::GraphicsSystemInfo* graphicsInfo = mLiveActorKit->getGraphicsSystemInfo();
    graphicsInfo->mAreaTarget = 2;
    const al::Resource* resource = al::getStageInfoDesign(this, 0)->getResource();
    graphicsInfo->initStageResource(resource, getDemoNames(mGameDataHolder)[0], mLiveActorKit,
                                    false, 0);
    // The result of this check is unused in the original code as well.
    isRosetta(mGameDataHolder);
    initPlacement(actorInfo);
    mLiveActorKit->getCameraDirector()->init(mLiveActorKit->getPlayerHolder());
    mLiveActorKit->getCameraDirector()->initAudioKeeper(actorInfo);

    mStaffRollLayoutHolder = new StaffRollLayoutHolder(layoutInfo, false);
    al::setSceneObj(this, mStaffRollLayoutHolder, 32);
    endInit(actorInfo, nullptr);
}

/**
 * Places the area objects, cameras, sky, objects and demos of the stage.
 * @param rInfo The actor init info.
 */
void DemoEndingScene::initPlacement(const al::ActorInitInfo& rInfo) {
    al::initPlacementAreaObj(this, rInfo, nullptr);
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

    initPlacementDemo(al::getStageInfoMap(this, 0), rInfo);
}

/**
 * Makes the scene appear and stops the framework from clearing the render buffer.
 */
void DemoEndingScene::appear() {
    al::Scene::appear();
    getFramework()->mIsClearRenderBuffer = false;
}

/**
 * Kills the scene and the active demo.
 */
void DemoEndingScene::kill() {
    al::Scene::kill();
    mDemoActorHolder->kill();
}

/**
 * Updates the graphics, camera, kit and staff roll.
 */
void DemoEndingScene::control() {
    mIsControlled = true;
    mLiveActorKit->preDrawGraphics();

    if (al::isStopScene(this)) {
        return;
    }

    mLiveActorKit->getCameraDirector()->update(false);
    al::updateKit(this);
    mStaffRollLayoutHolder->update();
}

/**
 * Draws the 3D view and the 2D layouts to the main screen.
 */
void DemoEndingScene::drawMain_() const {
    agl::RenderBuffer* renderBuffer = getFramework()->getCurrentRenderBuffer();
    mMainViewport->setByFrameBuffer(*renderBuffer);
    mLiveActorKit->getGraphicsSystemInfo()->getGraphicsStressDirector()->setFullResolution(
        getFramework()->mIsDocked);
    alSystemKitFunction::applyViewportTop(*mMainViewport);
    mLayoutKit->setFrameBuffer(renderBuffer, mMainViewport);

    if (mIsControlled) {
        al::LiveActorKit* kit = mLiveActorKit;
        al::ViewRenderer* viewRenderer = kit->getGraphicsSystemInfo()->getViewRenderer();
        viewRenderer->drawView(0, 0, kit, getSceneCameraInfo(),
                               getFramework()->getCurrentRenderBuffer(), *mMainViewport, true,
                               false, static_cast<agl::ShaderMode>(4));
    }

    al::drawKit(this, "２Ｄベース（メイン画面）");
    al::drawKit(this, "2DDrawAboveBlur2");
    mLiveActorKit->getCameraDirector()->getProjection()->setOffset(sead::Vector2f::zero);
}

/**
 * Draws nothing to the sub screen.
 */
void DemoEndingScene::drawSub_() const {}

/**
 * Plays the ending demo with the staff roll until the camera ends or it is skipped.
 */
void DemoEndingScene::exeEndRoll() {
    if (al::isFirstStep(this)) {
        appearDemoActorHolder(0);
        al::startBgm(this, "StaffRoll", 0, 0, -1, -1);
    }

    if (al::isStep(this, 0)) {
        mStaffRollLayoutHolder->startAppear();
    } else if (al::isGreaterEqualStep(this, 6990)) {
        if (al::isStep(this, 6990)) {
            mWipe->startClose(-1);
        }
    } else {
        u64 activePorts = rc::getActiveInputPortList(GameDataHolderAccessor(this));

        if (mDemoSkipLayout->isSkip(activePorts)) {
            al::setNerve(this, &NrvDemoEndingSceneCancel);
            return;
        }
    }

    if (mDemoActorHolder->isActionEndCamera(-1)) {
        mDemoActorHolder->kill();
        al::setNerve(this, &NrvDemoEndingSceneEndRollEnd);
    }
}

/**
 * Starts the given demo actor holder.
 * @param index The index of the demo actor holder.
 */
void DemoEndingScene::appearDemoActorHolder(s32 index) {
    mDemoActorHolder = mDemoActorHolders[index];
    rc::tryStartDemo(mDemoActorHolder);
    mDemoActorHolder->startAction(0, false);
    rc::tryHideDemoDisableFairyPrincess(mDemoActorHolder, mGameDataHolder);
}

/**
 * Plays the final demo after the staff roll, then fades out and ends the scene.
 */
void DemoEndingScene::exeEndRollEnd() {
    if (al::isFirstStep(this)) {
        appearDemoActorHolder(1);
    }

    if (al::isStep(this, 90)) {
        mWipe->startOpen(-1);
    }

    if (al::isStep(this, 1000)) {
        mWipe->startClose(90);
    }

    if (al::isStep(this, 720)) {
        mStaffRollLayoutHolder->startFin();
    }

    if (al::isGreaterEqualStep(this, 1000) && mWipe->isCloseEnd()) {
        kill();
    }
}

/**
 * Stops the staff roll after a skip and ends the scene once the wipe has closed.
 */
void DemoEndingScene::exeCancel() {
    if (al::isFirstStep(this)) {
        al::stopBgm(this, "StaffRoll", 28, -1);
        mWipe->startClose(-1);
        mDemoSkipLayout->kill();
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
void DemoEndingScene::initPlacementSky(const al::StageInfo* pStageInfo,
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
void DemoEndingScene::initPlacementObject(const al::StageInfo* pStageInfo,
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
 * Creates the ending demo actor holders.
 * @param pStageInfo The stage info.
 * @param rInfo The actor init info.
 */
void DemoEndingScene::initPlacementDemo(const al::StageInfo* pStageInfo,
                                        const al::ActorInitInfo& rInfo) {
    al::PlacementInfo placementInfo;
    al::tryGetPlacementInfo(&placementInfo, pStageInfo, "DemoObjList");
    PlayerRetargettingSelector* selector = rc::createPlayerRetargettingSelector(this);
    mDemoActorHolders[0] =
        rc::createDemoSceneHolder(getDemoNames(mGameDataHolder)[0], rInfo, selector, nullptr,
                                  false, isRosetta(mGameDataHolder) ? 5 : 4);
    mDemoActorHolders[1] =
        rc::createDemoSceneHolder(getDemoNames(mGameDataHolder)[1], rInfo, selector, nullptr,
                                  false, isRosetta(mGameDataHolder) ? 5 : 4);
}

#include "Scene/IslandScene.hpp"

#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageResourceKeeper.hpp"
#include "Library/Stage/StageResourceList.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "System/GameDataHolder.hpp"
#include "System/IslandDataList.hpp"
#include "Util/AreaObjUtil.hpp"

IslandScene::IslandScene() : SingleModeScene("IslandScene") {}

IslandScene::~IslandScene() = default;

void IslandScene::initOceanResourceKeeper() {
    mOceanResourceKeeper = new al::StageResourceKeeper;
    mOceanResourceKeeper->initAndLoadResource("IslandSceneStage", 0);
}

void IslandScene::preInitPlacement(const al::ActorInitInfo& rInfo) {
    ProjectActorFactory factory;
    const al::StageResourceList* resources = mOceanResourceKeeper->getMapStageInfo();
    s32 resourceNum = resources->getStageResourceNum();
    al::initPlacementByStageInfo(resources->getStageInfo(0), "SkyList", factory, rInfo);
    for (s32 i = 0; i < resourceNum; ++i)
        initPlacementObject(resources->getStageInfo(i), rInfo, "Map");
}

void IslandScene::initPlacementZoneHolders(const al::ActorInitInfo&) {}

void IslandScene::initIslandDataList() {
    mIslandDataList = new IslandDataList(2);
    mIslandDataList->RegisterIslandData(al::getStageInfoMap(this, 0), 1);
    mIslandDataList->endInit();
    mGameDataHolder->setIslandDataList(mIslandDataList);
}

void IslandScene::initIslandKeeper() {}

void IslandScene::endInitIslandKeeper() {}

void IslandScene::initPlacementOceanWater(al::ActorInitInfo& rInfo) {
    SingleModeScene::initPlacementOceanWater(
        mOceanResourceKeeper->getMapStageInfo()->getStageInfo(0), rInfo);
}

void IslandScene::initAreaObj(const al::ActorInitInfo& rInfo) {
    al::initPlacementAreaObj(this, rInfo, mOceanResourceKeeper->getMapStageInfo());
    rc::initAreaObjIndex(getLiveActorKit()->getAreaObjDirector());
}

const al::StageInfo* IslandScene::findOceanStageInfo() {
    return mOceanResourceKeeper->getMapStageInfo()->getStageInfo(0);
}

void IslandScene::appear() {
    SingleModeScene::appear();
}

bool SingleModeScene::allowRestartPoint() const {
    return true;
}

bool IslandScene::isIslandScene() const {
    return true;
}

bool SingleModeScene::isBossScene() const {
    return false;
}

bool IslandScene::doZoneIDCheck() const {
    return false;
}

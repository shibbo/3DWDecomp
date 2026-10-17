#include "Scene/DemoSceneSingleMode.hpp"

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageInfo.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/OceanWave/OceanWaveDirector.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"

DemoSceneSingleMode::DemoSceneSingleMode(bool unused, bool isNoBgm)
    : DemoScene(unused, isNoBgm) {}

DemoSceneSingleMode::~DemoSceneSingleMode() = default;

void DemoSceneSingleMode::init(const al::SceneInitInfo& rInfo) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mGameDataHolder->setSingleMode(true);
    mIsUseCameraRS = true;
    DemoScene::init(rInfo);
}

void DemoSceneSingleMode::initPlacement(const al::ActorInitInfo& rInfo) {
    mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer()->setSingleModeRendering();
    DemoScene::initPlacement(rInfo);
    initPlacementOceanWater(al::getStageInfoMap(this, 0), const_cast<al::ActorInitInfo&>(rInfo));
}

void DemoSceneSingleMode::initPlacementOceanWater(const al::StageInfo* pStageInfo,
                                                 al::ActorInitInfo& rInfo) {
    al::PlacementInfo listInfo;
    s32 count = 0;
    if (al::tryGetPlacementInfoAndCount(&listInfo, &count, pStageInfo, "OceanList") && count > 0) {

        ProjectActorFactory factory;
        al::PlacementInfo placementInfo;
        al::getPlacementInfoByIndex(&placementInfo, listInfo, 0);
        al::LiveActor* actor = al::createPlacementActorFromFactory(factory, rInfo, &placementInfo);
        if (!actor)
            return;

        al::ViewRenderer* viewRenderer = mLiveActorKit->getGraphicsSystemInfo()->getViewRenderer();
        auto* ocean = static_cast<al::OceanWaveDirector*>(actor);
        mOceanWaveDirector = ocean;
        const u8* byml = pStageInfo->getResource()->tryGetByml("Ocean");
        if (byml)
            ocean->initFromYaml(al::ByamlIter(byml), mStageName.cstr());
        else
            ocean->setStageName(mStageName.cstr());

        rInfo.mOceanWaveDirector = ocean;
        al::setSceneObj(this, ocean, SceneObjID_OceanWater);
        if (ocean->getRenderType() == 1)
            viewRenderer->enableSSR();
    }
}

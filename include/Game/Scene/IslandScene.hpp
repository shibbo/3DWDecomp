#pragma once

#include "Scene/SingleModeScene.hpp"

/** A standalone island using the shared IslandSceneStage ocean resources. */
class IslandScene : public SingleModeScene {
public:
    IslandScene();
    ~IslandScene() override;

    void initOceanResourceKeeper() override;
    void preInitPlacement(const al::ActorInitInfo& rInfo) override;
    void initPlacementZoneHolders(const al::ActorInitInfo& rInfo) override;
    void initIslandDataList() override;
    void initIslandKeeper() override;
    void endInitIslandKeeper() override;
    void initPlacementOceanWater(al::ActorInitInfo& rInfo) override;
    void initAreaObj(const al::ActorInitInfo& rInfo) override;
    const al::StageInfo* findOceanStageInfo() override;
    void appear() override;
    bool isIslandScene() const override;
    bool doZoneIDCheck() const override;

private:
    al::StageResourceKeeper* mOceanResourceKeeper; // 0x380
};
static_assert(sizeof(IslandScene) == 0x388);

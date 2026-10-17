#pragma once

#include "Scene/DemoScene.hpp"

/** Demo playback with Bowser's Fury rendering and ocean placement. */
class DemoSceneSingleMode : public DemoScene {
public:
    DemoSceneSingleMode(bool unused = false, bool isNoBgm = false);
    ~DemoSceneSingleMode() override;
    void init(const al::SceneInitInfo& rInfo) override;
    void initPlacement(const al::ActorInitInfo& rInfo) override;
    void initPlacementOceanWater(const al::StageInfo* pStageInfo, al::ActorInitInfo& rInfo);
};
static_assert(sizeof(DemoSceneSingleMode) == 0x170);

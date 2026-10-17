#pragma once
#include "Library/Scene/Scene.hpp"
#include <prim/seadSafeString.h>
namespace sead { class Viewport; }
namespace al { class StageInfo; }
class GameDataHolder;
class DemoOpeningState;

class DemoOpeningScene : public al::Scene {
public:
    DemoOpeningScene();
    ~DemoOpeningScene() override;
    void init(const al::SceneInitInfo& rInfo) override;
    void appear() override;
    void control() override;
    void drawMain_() const override;
    void drawSub_() const override;
    void exePlay();
    void initPlacement(const al::ActorInitInfo& rInfo);
    void initPlacementSky(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo, const char* pListName);
private:
    sead::FixedSafeString<64> mStageName;
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    GameDataHolder* mGameDataHolder = nullptr;
    DemoOpeningState* mOpeningState = nullptr;
};
static_assert(sizeof(DemoOpeningScene) == 0x160);

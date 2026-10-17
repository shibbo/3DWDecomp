#pragma once

#include <prim/seadSafeString.h>

#include "Library/Scene/Scene.hpp"

namespace sead {
class Viewport;
}  // namespace sead

namespace al {
class ActorInitInfo;
class StageInfo;
class WipeSimple;
}  // namespace al

class DemoSceneActorHolder;
class DemoSkipLayout;
class GameDataHolder;
class StaffRollLayoutHolder;

/**
 * Scene that plays the ending demo followed by the staff roll.
 */
class DemoEndingScene : public al::Scene {
public:
    DemoEndingScene();
    ~DemoEndingScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void initPlacement(const al::ActorInitInfo& rInfo);
    void appear() override;
    void kill() override;
    void control() override;
    void drawMain_() const override;
    void drawSub_() const override;

    void exeEndRoll();
    void appearDemoActorHolder(s32 index);
    void exeEndRollEnd();
    void exeCancel();

    void initPlacementSky(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             const char* pListName);
    void initPlacementDemo(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);

private:
    sead::FixedSafeString<64> mStageName;
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    GameDataHolder* mGameDataHolder = nullptr;
    DemoSceneActorHolder** mDemoActorHolders = nullptr;
    DemoSceneActorHolder* mDemoActorHolder = nullptr;
    StaffRollLayoutHolder* mStaffRollLayoutHolder = nullptr;
    al::WipeSimple* mWipe = nullptr;
    DemoSkipLayout* mDemoSkipLayout;
    u8 _180[0x8];
    bool mIsControlled = false;
};

#pragma once

#include <prim/seadSafeString.h>

#include "Library/Scene/Scene.hpp"

namespace sead {
class Viewport;
}  // namespace sead

namespace al {
class ActorInitInfo;
class StageInfo;
}  // namespace al

class DemoSceneActorHolder;
class DemoSkipLayout;
class GameDataHolder;

/**
 * Scene that plays a single stage demo (e.g. the opening demo) and then kills itself.
 */
class DemoScene : public al::Scene {
public:
    DemoScene(bool unused, bool isNoBgm);
    ~DemoScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void appear() override;
    void control() override;
    void drawMain_() const override;
    void drawSub_() const override;

    void exeStart();
    void exePlay();

    s32 getDemoLength() const;

    virtual void initPlacement(const al::ActorInitInfo& rInfo);
    void initPlacementSky(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             const char* pListName);
    void initPlacementDemo(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);

    /** @brief Whether the demo was skipped by the player. @return The flag. */
    bool isSkipped() const { return mIsSkipped; }

private:
    sead::FixedSafeString<64> mStageName;
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    GameDataHolder* mGameDataHolder = nullptr;
    DemoSceneActorHolder* mDemoActorHolder = nullptr;
    bool mIsUseLayout = true;
    bool mIsSkipped = false;
    bool mIsNoBgm;
    bool mIsCancelAudioOnSkip = false;
    DemoSkipLayout* mDemoSkipLayout;
};

static_assert(sizeof(DemoScene) == 0x170);

#pragma once

#include <prim/seadSafeString.h>

#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Scene/Scene.hpp"

namespace agl {
class RenderBuffer;
}  // namespace agl

namespace sead {
class Viewport;
}  // namespace sead

namespace al {
class ActorInitInfo;
class LayoutActor;
class StageInfo;
class WipeSimple;
}  // namespace al

class DemoSceneActorHolder;
class DemoSkipLayout;
class GameDataHolder;
class StaffRollLayoutHolder;

/**
 * Scene that plays the Bowser's Fury ending demo, the staff roll and the "Thanks for playing"
 * screen.
 */
class DemoSingleModeEndingScene : public al::Scene, public al::IUseFrameBufferDrawer {
public:
    DemoSingleModeEndingScene();
    ~DemoSingleModeEndingScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    void initPlacement(const al::ActorInitInfo& rInfo);
    void appear() override;
    void kill() override;
    void control() override;
    void drawToFrameBuffer(const agl::RenderBuffer* pRenderBuffer,
                           const sead::Viewport* pViewport) override;
    void drawMain_() const override;
    void drawSub_() const override;

    void exeEndRoll();
    void appearDemoActorHolder();
    void exeCancel();
    void exeThanksForPlaying();

    void initPlacementSky(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             const char* pListName);
    void initPlacementDemo(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);

private:
    sead::FixedSafeString<64> mStageName;
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    GameDataHolder* mGameDataHolder = nullptr;
    DemoSceneActorHolder* mDemoActorHolder = nullptr;
    StaffRollLayoutHolder* mStaffRollLayoutHolder = nullptr;
    al::WipeSimple* mWipe = nullptr;
    DemoSkipLayout* mDemoSkipLayout;
    al::LayoutActor* mThanksForPlayingLayout = nullptr;
};

static_assert(sizeof(DemoSingleModeEndingScene) == 0x188);

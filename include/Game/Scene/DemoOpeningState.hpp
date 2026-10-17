#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadMatrix.h>
namespace al {
class ActorInitInfo;
class GraphicsSystemInfo;
class Scene;
class WipeSimple;
class PlayerHolder;
} // namespace al
class DemoSceneActorHolder;
class DemoSkipLayout;

class DemoOpeningState : public al::NerveStateBase {
public:
    DemoOpeningState(al::Scene* pScene, const al::ActorInitInfo& rInfo,
                     al::GraphicsSystemInfo* pGraphicsSystemInfo, sead::Matrix34f* pMtx);
    ~DemoOpeningState() override = default;
    void appear() override;
    void control() override;
    void exeFade();
    void exeStart();
    void exeRouteDokan();
    void exeCourseSelect();
    void exeDemoEnd();
    void exeCancelDemo();
    void setDemoPlayerCourseSelectStartPos();

private:
    al::Scene* mScene;
    al::WipeSimple* mWhiteWipe;
    al::WipeSimple* mBlackWipe;
    const al::Nerve* mNextNerve;
    bool mIsRouteDokan = false;
    DemoSceneActorHolder** mDemoActorHolders = nullptr;
    DemoSceneActorHolder* mDemoActorHolder = nullptr;
    al::PlayerHolder* mPlayerHolder = nullptr;
    al::GraphicsSystemInfo* mGraphicsSystemInfo;
    DemoSkipLayout* mSkipLayout;
};
static_assert(sizeof(DemoOpeningState) == 0x68);

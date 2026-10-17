#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { template <class T> class DeriveActorGroup; }
class Crawler;

class CrawlerLaunchStateParam {
public:
    CrawlerLaunchStateParam();
    CrawlerLaunchStateParam(f32 signSpeed, f32 launchSpeed, f32 jumpSpeed, f32 gravity,
                            s32 signMoveTime, s32 signTime, s32 launchTime);
    f32 mSignSpeed;
    f32 mLaunchSpeed;
    f32 mJumpSpeed;
    f32 mGravity;
    s32 mSignMoveTime;
    s32 mSignTime;
    s32 _18;
    s32 mLaunchTime;
};
static_assert(sizeof(CrawlerLaunchStateParam) == 0x20);

class CrawlerLaunchState : public al::ActorStateBase {
public:
    CrawlerLaunchState(al::LiveActor* pHost, const CrawlerLaunchStateParam* pParam,
                       al::DeriveActorGroup<Crawler>* pGroup);
    void appear() override;
    void control() override;
    void exeLaunchSet();
    void exeAppearSign();
    void exeAppear();
    void exeAppearEnd();
private:
    const CrawlerLaunchStateParam* mParam;
    al::DeriveActorGroup<Crawler>* mGroup;
    Crawler* mCrawler = nullptr;
    f32 mHeight = 0.0f;
    f32 mDistance = 0.0f;
    f32 mJumpSpeed = 0.0f;
    s32 mNextIndex = 0;
};
static_assert(sizeof(CrawlerLaunchState) == 0x48);

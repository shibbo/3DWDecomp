#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class LiveActorGroup; }
class BoxKiller;
class BoxKillerLauncher : public al::LiveActor {
public:
    explicit BoxKillerLauncher(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void exeWait();
    void exeDelayWait();
    void exeShot();
    void exeRotateSearchWait();
    void exeRotateSearch();
private:
    BoxKiller* mBox = nullptr;
    al::LiveActorGroup* mBlocks = nullptr;
    s32 mDelayTime = 0;
    s32 mBulletFlyTime = 100;
    s32 mSearchType = 0;
    s32 _164 = 0;
};
static_assert(sizeof(BoxKillerLauncher) == 0x168);

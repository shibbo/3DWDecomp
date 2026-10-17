#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
class DarkBowser;
class BowserSpike;
namespace al { class ActorInitInfo; class HitSensor; }
class DarkBowserShellSpike : public al::HostStateBase<DarkBowser> {
public:
    DarkBowserShellSpike(DarkBowser*, const al::ActorInitInfo&);
    ~DarkBowserShellSpike() override;
    void appear() override;
    void kill() override;
    void forceKill();
    void attackSensor(al::HitSensor*, al::HitSensor*);
    void exeBegin();
    void shuffle();
    void exeLaunchSpike();
    void launchRandom();
    void exeEnd();
    void setLevel(s32 level) { mLevel = level; }
private:
    int mLevel = 0;
    sead::PtrArray<BowserSpike> mSpikes;
    int mLaunchCount = 0;
    int mPoolGroup = 0;
    int mAttackCount = 0;
    sead::Vector3f mTarget;
    int mBatchCount = 3;
    int mOrder[24];
};
static_assert(sizeof(DarkBowserShellSpike) == 0xb8);

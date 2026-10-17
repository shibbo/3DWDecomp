#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
#include <container/seadBuffer.h>
#include <prim/seadBitFlag.h>
namespace al {
class CollisionObj;
}
class KoopaLastWallClimb;
class KoopaLastDefendeBlockPow;
class KoopaLastBlockPowStateDemoEnd;
class KoopaLastBlockPow : public al::LiveActor {
  public:
    explicit KoopaLastBlockPow(const char *pName);
    ~KoopaLastBlockPow() override;
    void init(const al::ActorInitInfo &) override;
    bool receiveMsg(const al::SensorMsg *, al::HitSensor *, al::HitSensor *) override;
    void exeWait();
    void exeReaction();
    void exeDemo();

  private:
    int mHitCount = 0;
    sead::PtrArray<KoopaLastWallClimb> mWallClimbers;
    KoopaLastDefendeBlockPow *mDefender = nullptr;
    KoopaLastBlockPowStateDemoEnd *mDemoState = nullptr;
    sead::BitFlag16 mHitUsers;
    sead::Buffer<int> mUserHitTimers;
    al::CollisionObj *mCollision = nullptr;
};
static_assert(sizeof(KoopaLastBlockPow) == 0x188);

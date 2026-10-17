#pragma once
#include "Library/LiveActor/LiveActor.hpp"

/** @brief Rail-moving Boo wall that advances while nearby players look away. */
class TeresaWall : public al::LiveActor {
public:
    explicit TeresaWall(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeStop();
    bool isStartMove();
    void exeMoveSign();
    void exeMove();
private:
    f32 mMoveSpeed = 3.0f;
    f32 mFindDistance = 1500.0f;
    s32 mWaitTime;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
};
static_assert(sizeof(TeresaWall) == 0x160);

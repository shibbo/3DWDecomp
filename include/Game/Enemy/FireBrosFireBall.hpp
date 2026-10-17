#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadVector.h>
class ActorStateRouteDokanMove;

/** @brief Bouncing fireball thrown by a Fire Bro. */
class FireBrosFireBall : public al::LiveActor {
public:
    explicit FireBrosFireBall(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void updateCollider() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void shoot(const al::LiveActor* pHost, const sead::Matrix34f* pMtx,
               const sead::Vector3f& rOffset, f32 rate);
    void exeMove();
    void exeRouteDokan();
private:
    void hitDisappear();
    void updateAppearAction();
    u32 mLifeStep = 0;
    const al::LiveActor* mHost = nullptr;
    f32 mShotRate = 0.0f;
    bool mHasBounced = false;
    bool mIsAppearing = true;
    bool mUpdateShadow = false;
    ActorStateRouteDokanMove* mRouteDokan = nullptr;
};
static_assert(sizeof(FireBrosFireBall) == 0x160);

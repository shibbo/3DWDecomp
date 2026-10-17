#pragma once
#include "Boss/BombBound.hpp"
#include "Library/LiveActor/LiveActor.hpp"
namespace al { template <class T> class DeriveActorGroup; }

/** @brief Launches pooled bouncing bombs at a fixed interval. */
class BombBoundGenerator : public al::LiveActor {
public:
    explicit BombBoundGenerator(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void start();
    void switchAppear();
    void switchKill();
    void exeIdle();
    void exeWaitStarting();
    void exeReload();
    void exeLaunch();
    void exeInterval();
private:
    BombBound::Type mType = BombBound::Type_Roll;
    al::DeriveActorGroup<BombBound>* mBombs = nullptr;
    BombBound* mCurrentBomb = nullptr;
    f32 mShootSpeed = 10.0f;
    s32 mShootInterval = 300;
    bool mIsImmediateStarting = false;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    s32 mTimer = 0;
    s32 mInitialDelay = 0;
};
static_assert(sizeof(BombBoundGenerator) == 0x178);

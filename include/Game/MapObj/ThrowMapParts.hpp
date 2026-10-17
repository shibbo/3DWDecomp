#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
class BallStateThrow;
class BallStateFall;
class BallStateThrowParam;
class ItemStateGigaPlayerHold;
class ThrowMapParts : public al::LiveActor {
public:
    explicit ThrowMapParts(const char*);
    ~ThrowMapParts() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isEnableHold(al::HitSensor*);
    void control() override;
    void updateCollider() override;
    void requestRelease(al::HitSensor*, BallStateThrowParam*, const char*);
    void exeWait();
    void exePullOutSuccess();
    void exePullOutMiss();
    void exePullOutMissWait();
    void exePlayerHold();
    void exeThrow();
    void exeFall();
    void exeStuck();
    void exeDestroy();
private:
    sead::Matrix34f mInitialMtx = sead::Matrix34f::ident;
    BallStateThrow* mThrowState = nullptr;
    BallStateFall* mFallState = nullptr;
    ItemStateGigaPlayerHold* mHoldState = nullptr;
    int mPullOutCount = 3;
    al::HitSensor* mStuckSensor = nullptr;
    const sead::Matrix34f* mStuckMtx;
    sead::Matrix34f mStuckOffset = sead::Matrix34f::ident;
    al::HitSensor* mHolderSensor = nullptr;
};
static_assert(sizeof(ThrowMapParts) == 0x1e0);

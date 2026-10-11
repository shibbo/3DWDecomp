#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Crab : public al::LiveActor {
public:
    explicit Crab(const char*);
    ~Crab() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void setNextTargetPos();
    void exeWalk();
    void exeDisappear();
private:
    sead::Vector3f mCenter = sead::Vector3f::zero;
    sead::Vector3f mSide = sead::Vector3f::ex;
    sead::Vector3f mStart = sead::Vector3f::zero;
    sead::Vector3f mTarget = sead::Vector3f::zero;
    int mWaitTime = 0;
    float mWalkTime = 0.0f;
    int mItemId = -1;
    float mMinRange = 400.0f;
    float mMaxRange = 450.0f;
    bool mPositive = true;
};
static_assert(sizeof(Crab) == 0x190);

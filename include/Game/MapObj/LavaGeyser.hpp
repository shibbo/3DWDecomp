#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class LavaGeyser : public al::LiveActor {
public:
    LavaGeyser(const char* name);
    ~LavaGeyser() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void start();
    void calcEffectTrans();
    void exeWait();
    void exeReady();
    void exeRise();
    void exeKeepRising();
    void exeFallDown();
    void exeSwitchOffStart();
    void exeDelay();
private:
    sead::Vector3f mStartPos = sead::Vector3f::zero;
    sead::Vector3f mEndPos = sead::Vector3f::zero;
    int mWaitTime = 0;
    int mDelayTime = 0;
    sead::Vector3f mEffectTrans = sead::Vector3f::zero;
    float mSearchDistanceUp = 0.0f;
    float mSearchDistanceDown = 0.0f;
    sead::Vector3f mClippingCenter = sead::Vector3f(0.0f, 0.0f, 0.0f);
};
static_assert(sizeof(LavaGeyser) == 0x188);

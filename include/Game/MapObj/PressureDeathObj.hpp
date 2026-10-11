#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class PressureDeathObj : public al::LiveActor {
public:
    explicit PressureDeathObj(const char*);
    ~PressureDeathObj() override;
    void init(const al::ActorInitInfo&) override;
    void start();
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool isInsidePressureArea(const al::HitSensor*);
    void exeMove();
    void exeDelay();
    void exeWait();
    void exeStop();
private:
    al::KeyPoseKeeper* mKeyPose = nullptr;
    sead::Vector3f mClippingCenter{0.0f, 0.0f, 0.0f};
    sead::Vector3f mPressureHalfSize{0.0f, 0.0f, 0.0f};
    int mMoveTime = 0;
    int mDelayTime = 0;
    int mWaitTime = 0;
};
static_assert(sizeof(PressureDeathObj) == 0x178);

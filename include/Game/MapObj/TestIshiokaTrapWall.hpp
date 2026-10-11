#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class TestIshiokaTrapWall : public al::LiveActor {
public:
    explicit TestIshiokaTrapWall(const char*);
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    void start();
    void exeMove();
    void exeDelay();
    void exeWait();
    void exeStop();
private:
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    int mMoveFrames = 0;
    int mDelayFrames = 0;
    int mWaitFrames = 0;
};
static_assert(sizeof(TestIshiokaTrapWall) == 0x168);

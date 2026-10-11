#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class RideUserInfoKeeper;
namespace al { class KeyPoseKeeper; }
class MultiLift : public al::LiveActor {
public:
    explicit MultiLift(const char*);
    ~MultiLift() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWaitForRide();
    void exeRideCheck();
    void exeWait();
    void exePreMove();
    void exeMove();
    void exeStop();
private:
    RideUserInfoKeeper* mRiders = nullptr;
    al::KeyPoseKeeper* mKeyPoses = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mMoveTime = 0;
    int mRequiredRiders = 2;
    bool mAntiLightStrong = false;
};
static_assert(sizeof(MultiLift) == 0x170);

#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class RotateHangDynamics;
class CollisionObj;
} // namespace al
class RideUserInfoKeeper;
class Gondola : public al::LiveActor {
  public:
    explicit Gondola(const char*);
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void killBySwitch();
    void updateDynamics();
    void exeAppear();
    void exeRailRunBeforeWait();
    void exeRailRun();
    void exeRailReachGoal();
    void exeDisappear();
    void exeDisappearSound();

  private:
    sead::Vector3f mClippingCenter;
    float mSpeed = 0.0f;
    float mSpeedMin = 5.0f;
    float mSpeedMax = 50.0f;
    float mGravity = 1.0f;
    float mDamping = 0.99f;
    float mRadius = 100.0f;
    int _168 = 0;
    al::RotateHangDynamics* mDynamics = nullptr;
    RideUserInfoKeeper* mRideInfo = nullptr;
    al::LiveActor* mBody = nullptr;
    al::CollisionObj* mFrame = nullptr;
};
static_assert(sizeof(Gondola) == 0x190);

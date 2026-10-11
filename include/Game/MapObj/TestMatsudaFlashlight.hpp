#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class RigidBodyCore;
class TestMatsudaFlashlight : public al::LiveActor {
public:
    TestMatsudaFlashlight(const char* name);
    ~TestMatsudaFlashlight() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeFall();
    void exeHold();
    void calcHoldPos(sead::Vector3f* position);
private:
    al::HitSensor* mHolder = nullptr;
    RigidBodyCore* mRigidBody = nullptr;
};
static_assert(sizeof(TestMatsudaFlashlight) == 0x158);

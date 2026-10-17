#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TestIshiokaTrapRock : public al::LiveActor {
public:
    explicit TestIshiokaTrapRock(const char* name);
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    void updateCollider() override;
    void start();
    void exeWait();
    void exeMove();
private:
    sead::Vector3f mAngularVelocity = {0, 0, 0};
};

static_assert(sizeof(TestIshiokaTrapRock) == 0x150);

#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ItemStatePlayerHold;
class ItemStatePopUpFront;
class TestNut : public al::LiveActor {
public:
    TestNut(const char*);
    ~TestNut() override;
    void init(const al::ActorInitInfo&) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
    void exePlayerHold();
    void exeThrow();
private:
    ItemStatePlayerHold* mHoldState = nullptr;
    ItemStatePopUpFront* mThrowState = nullptr;
    al::HitSensor* mThrower = nullptr;
};

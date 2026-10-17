#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockSwitch : public al::LiveActor {
public:
    explicit BlockSwitch(const char*);
    ~BlockSwitch() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void listenSwitchOn();
    void listenSwitchOff();
    void exeOff();
    void exeOffWait();
    void exeOn();
    void exeOnWait();
private:
    int mHitDelay = 0;
    float mDisplayRotateX = 0.0f;
};
static_assert(sizeof(BlockSwitch) == 0x150);

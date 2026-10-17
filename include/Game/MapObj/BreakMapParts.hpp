#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class BreakMapParts : public al::LiveActor {
public:
    explicit BreakMapParts(const char* name);
    ~BreakMapParts() override;
    void init(const al::ActorInitInfo& info) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgSand(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool receiveMsgBomb(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool receiveMsgGiant(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool receiveMsgMapParts(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    void startBreak(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    void startBreakWithTouch(const al::SensorMsg*, const al::HitSensor*);
    void startBreakBySwitch();
private:
    int mBreakType = 0;
    al::LiveActor* mBreakModel = nullptr;
    al::LiveActor* mTraceModel = nullptr;
    bool mRandomRotate = false;
    bool mSilent = false;
};
static_assert(sizeof(BreakMapParts) == 0x160);

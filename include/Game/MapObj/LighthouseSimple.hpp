#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class LighthouseSimple : public al::LiveActor {
public:
    explicit LighthouseSimple(const char*);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void setPhaseColor();
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
private:
    al::LiveActor* mInk = nullptr;
    al::LiveActor* mFlag = nullptr;
    int mIslandId = -1;
};
static_assert(sizeof(LighthouseSimple) == 0x160);

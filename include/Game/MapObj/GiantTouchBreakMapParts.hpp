#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class GiantTouchBreakMapParts : public al::LiveActor {
public:
    explicit GiantTouchBreakMapParts(const char* pName);
    ~GiantTouchBreakMapParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
};

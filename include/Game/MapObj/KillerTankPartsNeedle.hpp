#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class KillerTankPartsNeedle : public al::LiveActor {
public:
    explicit KillerTankPartsNeedle(const char* pName);
    ~KillerTankPartsNeedle() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
};

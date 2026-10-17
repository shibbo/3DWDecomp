#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class HexScrollAppearParts : public al::LiveActor {
public:
    explicit HexScrollAppearParts(const char* pName);
    ~HexScrollAppearParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
};

#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TeresaFakeDokan : public al::LiveActor {
public:
    explicit TeresaFakeDokan(const char* pName);
    ~TeresaFakeDokan() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
};

#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class FortressGoal : public al::LiveActor {
public:
    explicit FortressGoal(const char* pName);
    ~FortressGoal() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
};

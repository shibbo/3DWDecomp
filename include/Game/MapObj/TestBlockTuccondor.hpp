#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class TestBlockTuccondor : public al::LiveActor {
public:
    TestBlockTuccondor(const char* pName);
    ~TestBlockTuccondor() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void exeWait();
    void exeReaction();
};

#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TestAndoDashPanel : public al::LiveActor {
public:
    explicit TestAndoDashPanel(const char* pName);
    ~TestAndoDashPanel() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
};

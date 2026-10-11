#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class BreakModel; }

class TestTatsutaBlock : public al::LiveActor {
public:
    TestTatsutaBlock(const char* pName);
    ~TestTatsutaBlock() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void exeStop();
    void exeWait();
    void exeMove();
    void exeBreak();

private:
    al::BreakModel* mBreakModel = nullptr;
};

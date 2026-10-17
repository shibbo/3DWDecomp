#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class Gong : public al::LiveActor {
public:
    Gong(const char* pName);
    ~Gong() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();
    void exeSound();
};

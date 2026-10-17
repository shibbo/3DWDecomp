#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class NpcStateWait;
class NpcStateWaitParam;
class RabbitNpc : public al::LiveActor {
public:
    explicit RabbitNpc(const char* pName);
    ~RabbitNpc() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();

private:
    NpcStateWait* mWaitState;
};
static_assert(sizeof(RabbitNpc) == 0x150);

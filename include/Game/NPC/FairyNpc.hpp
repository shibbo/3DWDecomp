#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class NpcStateWait;
class NpcStateWaitParam;
class FairyNpc : public al::LiveActor {
public:
    explicit FairyNpc(const char* pName);
    ~FairyNpc() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();

private:
    NpcStateWait* mWaitState;
    NpcStateWaitParam* mWaitParam;
};
static_assert(sizeof(FairyNpc) == 0x158);

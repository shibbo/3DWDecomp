#pragma once
#include "Raidon/RaidonActor.hpp"
class RaidonEndState;
class ActorStateSupportStroke;
class RaidonNpc : public RaidonActor {
public:
    explicit RaidonNpc(const char* pName);
    ~RaidonNpc() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();

private:
    RaidonEndState* mEndState;
    ActorStateSupportStroke* mStroke;
};
static_assert(sizeof(RaidonNpc) == 0x180);

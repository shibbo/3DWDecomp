#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class NpcStateWait;
class NpcStateWaitParam;
class Tico : public al::LiveActor {
public:
    explicit Tico(const char* pName);
    ~Tico() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void invalidateTurn();
    void startGoalPose();
    void exeGoalPose();
    void exeWait();

private:
    NpcStateWait* mWaitState;
    sead::Vector3f mInitialFront;
};
static_assert(sizeof(Tico) == 0x160);

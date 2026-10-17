#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class NpcStateWait;
class NpcStateWaitParam;
class FairyNpcWithGlasses : public al::LiveActor {
public:
    explicit FairyNpcWithGlasses(const char* pName);
    ~FairyNpcWithGlasses() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();
    void exeViewPoint();

private:
    NpcStateWait* mWaitState;
    sead::Vector3f mInitialFront;
    al::LiveActor* mGlasses;
    bool mTurnComplete;
};
static_assert(sizeof(FairyNpcWithGlasses) == 0x170);

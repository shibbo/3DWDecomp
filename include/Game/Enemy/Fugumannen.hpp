#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class EnemyStateBlowDown;
class EnemyStateBlowDownParam;
class ActorStateSupportFreeze;
struct ActorStateSupportFreezeParam;
class Fugumannen : public al::LiveActor {
public:
    explicit Fugumannen(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void calcBrakeDistance();
    void exeSwimWait();
    void exeTurn();
    void exeSwimBlowDown();
    void exeBrake();
    void exeAccelerate();
    void exeSupportFreeze();
private:
    f32 mSwimSpeed = 5.0f;
    EnemyStateBlowDown* mBlowDown = nullptr;
    EnemyStateBlowDownParam* mBlowDownParam = nullptr;
    ActorStateSupportFreeze* mFreeze = nullptr;
    ActorStateSupportFreezeParam* mFreezeParam = nullptr;
    f32 mBrakeDistance = 0.0f;
};
static_assert(sizeof(Fugumannen) == 0x170);

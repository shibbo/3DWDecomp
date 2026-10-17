#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class EnemyStateBlowDown;
class EnemyStateBlowDownParam;

class PukupukuFly : public al::LiveActor {
public:
    explicit PukupukuFly(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool isEnableDown();
    void exeFlyWait();
    void exePressDown();
    void exeBlowDown();
private:
    f32 mSpeed = 10.0f;
    EnemyStateBlowDown* mBlowDown = nullptr;
    EnemyStateBlowDownParam* mBlowDownParam;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
};
static_assert(sizeof(PukupukuFly) == 0x168);

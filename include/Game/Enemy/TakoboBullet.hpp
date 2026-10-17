#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ActorStateSupportFreeze;
class BombStateExplosion;

/** @brief Explosive projectile fired by Takobo. */
class TakoboBullet : public al::LiveActor {
public:
    TakoboBullet(const char* pName, al::LiveActor* pHost);
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeMove();
    bool incrementCountAndChagneNerve();
    void exeExplode();
    void exeExplodeSmall();
    void exeFallDown();
    void exeSupportFreeze();
private:
    ActorStateSupportFreeze* mSupportFreeze = nullptr;
    BombStateExplosion* mExplosion = nullptr;
    al::LiveActor* mHost;
    s32 mLifeStep = 0;
};
static_assert(sizeof(TakoboBullet) == 0x168);

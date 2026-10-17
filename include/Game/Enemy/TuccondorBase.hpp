#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class EnemyStateBlowDown;

/** @brief Shared collision, movement, and defeat behavior for Conkdors. */
class TuccondorBase : public al::LiveActor {
public:
    explicit TuccondorBase(const char* pName);
    void initNerveState();
    bool isHitNeckSensor(const al::HitSensor* pSelf, const al::HitSensor* pOther, f32 radius) const;
    void updateVelocity();
    bool tryRequestPressDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, bool isStanding);
    bool tryRequestBlowDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    void exeBlowDown();
    void exePressDown();
    void exePressDownAttackWait();
    void requestHitReactionAttack();
    bool isNerveDead() const;
protected:
    EnemyStateBlowDown* mBlowDown;
    f32 _150 = 5.0f;
    f32 _154 = 40.0f;
};
static_assert(sizeof(TuccondorBase) == 0x158);

#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <prim/seadEnum.h>

namespace al {
class HitSensor;
class SensorMsg;
}  // namespace al
class KoopaChase;
class KoopaChaseStateDamage : public al::NerveStateBase {
public:
    SEAD_ENUM(DamageResult, None, Small, Large);
    void kill() override;
    DamageResult doDamage(al::HitSensor* pOther, bool isJumpOrWarp);
    DamageResult doSmallDamage(al::HitSensor* pOther, bool isJumpOrWarp);
    DamageResult doFireBallDamage(al::HitSensor* pOther, bool isJumpOrWarp);
    bool isInvalidFireBallDamage() const;
    void decLife();
    void exeWait();
    void exeDamage();
    void exeSmallDamage();
    KoopaChaseStateDamage(KoopaChase* pHost);
    bool receiveMsg(bool* pIsDamage, const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf, bool isJumpOrWarp);
    bool isDamageFinish() const;

    int getDamageCount() const { return mDamageCount; }

    KoopaChase* mHost;
    int mDamageCount = 0;
    int mSmallDamageCount = 0;
    int mFireBallDamageCount = 0;
    int mLastFireBallStep = 0;
};

static_assert(sizeof(KoopaChaseStateDamage) == 0x30);

#pragma once

#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ComboCounter;
class HitSensor;
class LiveActor;
}  // namespace al

/** @brief Tuning of an explosion; the state falls back to built-in defaults when none is given. */
struct BombStateExplosionParam {
    f32 radiusMax;               // 0x0
    f32 radiusMin;               // 0x4
    f32 radiusSpeed;             // 0x8
    sead::Vector3f effectScale;  // 0xc
    f32 rate;                    // 0x18
    s32 attackStep;              // 0x1c
};
static_assert(sizeof(BombStateExplosionParam) == 0x20);

class BombStateExplosion : public al::ActorStateBase {
public:
    BombStateExplosion(al::LiveActor* pActor, bool isUseExplosionSensor,
                       const BombStateExplosionParam* pParam);
    void reset();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther, al::ComboCounter* pCounter);
    void attackSensorGiga(al::HitSensor* pSelf, al::HitSensor* pOther, al::ComboCounter* pCounter);

    /** @brief Sets whether the explosion also damages players. */
    void setIsAttackToPlayer(bool isAttackToPlayer) { mIsAttackToPlayer = isAttackToPlayer; }

private:
    bool _20;
    bool mIsAttackToPlayer;  // 0x21
    u8 mUnreconstructed[0xe];
};
static_assert(sizeof(BombStateExplosion) == 0x30);

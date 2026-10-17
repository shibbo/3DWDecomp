#pragma once

#include <gfx/seadColor.h>

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
    sead::Color4f lightColor;    // 0xc
    s32 attackStep;              // 0x1c
};
static_assert(sizeof(BombStateExplosionParam) == 0x20);

class BombStateExplosion : public al::ActorStateBase {
public:
    BombStateExplosion(al::LiveActor* pActor, bool isUseExplosionSensor,
                       const BombStateExplosionParam* pParam);
    void init() override;
    void appear() override;
    void reset();
    void exeExplosion();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther, al::ComboCounter* pCounter);
    void attackSensorGiga(al::HitSensor* pSelf, al::HitSensor* pOther, al::ComboCounter* pCounter);

    /** @brief Sets whether the explosion also damages players. */
    void setIsAttackToPlayer(bool isAttackToPlayer) { mIsAttackToPlayer = isAttackToPlayer; }

private:
    bool mIsUseExplosionSensor;
    bool mIsAttackToPlayer;  // 0x21
    const BombStateExplosionParam* mParam;
};
static_assert(sizeof(BombStateExplosion) == 0x30);

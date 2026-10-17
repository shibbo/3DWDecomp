#pragma once

#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class HitSensor;
class SensorMsg;
}  // namespace al
class PackunFlowerHead;

/** @brief State of a Piranha Plant being carried (and eating) while held by the player. */
class PackunStateHold : public al::ActorStateBase {
public:
    PackunStateHold(al::LiveActor* pHost, PackunFlowerHead** pHeads);
    ~PackunStateHold() override;

    void init() override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool tryStartCarry(const al::SensorMsg* pMsg, al::HitSensor* pOther);
    void initColliderControl();
    void updateCollider(al::HitSensor* pSensor);
    void requestRelease();
    void setHoldPosAndDir();

    void exeCarryStart();
    void exeHold();
    void exeEat();
    void exeEatStandBy();
    void exeSwallow();

private:
    void* _20 = nullptr;
    void* _28;
    PackunFlowerHead** mHeads;
    sead::Vector3f _38 = sead::Vector3f::zero;
    s32 _44 = 0;
    bool _48 = false;
};

static_assert(sizeof(PackunStateHold) == 0x50);

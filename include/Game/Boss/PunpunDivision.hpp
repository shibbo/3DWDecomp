#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief One of the clones Punpun (Motley Bossblob) splits into. */
class PunpunDivision : public al::LiveActor {
public:
    explicit PunpunDivision(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void startDivide();
    void startDivideAppear();
    bool tryStartThrow();
    void exeAppear();
    void exeDivideAppear();
    void exeThrowWait();
    void exeThrowSign();
    void exeThrow();

    /**
     * @brief Sets where this clone flies to while splitting off.
     * @param rTrans Target position.
     */
    void setDivideTargetTrans(const sead::Vector3f& rTrans) { mDivideTargetTrans = rTrans; }

    /** @brief Gets where this clone flies to while splitting off. */
    const sead::Vector3f& getDivideTargetTrans() const { return mDivideTargetTrans; }

private:
    u8 mUnknown144[0x158 - 0x144];
    sead::Vector3f mDivideTargetTrans;  // 0x158
    u8 mUnknown164[0x170 - 0x164];
};

static_assert(sizeof(PunpunDivision) == 0x170);

#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class PunpunShuriken;
class ActorJointLookController;
struct RenderMaterialIndirectParam;

/** @brief One of the clones Punpun (Motley Bossblob) splits into. */
class PunpunDivision : public al::LiveActor {
public:
    explicit PunpunDivision(const char* pName);

    ~PunpunDivision() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeDivide();
    void updateShurikenPose();
    void exeThrowEnd();
    void exeAttackSuccess();
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void startDivide();
    void startDivideAppear();
    void tryStartThrow();
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
    PunpunShuriken* mShuriken;
    ActorJointLookController* mLookController = nullptr;
    sead::Vector3f mDivideTargetTrans = {0.0f, 0.0f, 0.0f};  // 0x158
    RenderMaterialIndirectParam* mMaterialParam;
};

static_assert(sizeof(PunpunDivision) == 0x170);

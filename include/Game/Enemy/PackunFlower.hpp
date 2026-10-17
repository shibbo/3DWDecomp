#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class MtxConnector;
}  // namespace al
class ActorMicRumbler;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;

/** @brief Piranha Plant (and Big Piranha Plant) growing out of the ground. */
class PackunFlower : public al::LiveActor {
public:
    /** @brief How much damage a Big Piranha Plant can still take before it is knocked out. */
    enum class HitState : s32 {
        Damaged = 1,
        Healthy = 2,
    };

    explicit PackunFlower(const char* pName);
    /** @brief Destroys the Piranha Plant. */
    ~PackunFlower() override = default;

    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void initAfterPlacement() override;
    void reappear() override;
    void killComplete(bool isNoReaction) override;
    void control() override;
    bool isEnableAttack();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isDamaged();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool checkCollision(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    void tryAppearTrace();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeSleep();
    void exeFind();
    void exeWait();
    void exeTurn();
    bool isInSight();
    void exeTurnFast();
    void exeAttack();
    void exeAfterAttack();
    void exePressDown();
    void exeBlowDown();
    void exeSupportFreeze();
    void exeSupportFreezeSwoon();
    void exeTrampled();
    void exeSwoon();
    void exeSwoonEnd();
    void exeDamage();

private:
    al::LiveActor* mTrace = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    al::MtxConnector* mMtxConnector = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    ActorMicRumbler* mMicRumbler = nullptr;
    f32 mTurnDegree = 4.0f;
    bool mIsRemainTrace = false;
    bool mIsBig = false;
    bool mIsBlowBackDir = false;
    sead::Vector3f mStemDir = sead::Vector3f::ez;
    const sead::Matrix34f* mSpinMtx = nullptr;
    sead::Vector3f mUpDir = sead::Vector3f::ey;
    sead::Quatf mPoseOffsetQuat = sead::Quatf::unit;
    s32 mInvincibleTimer = 0;
    s32 mTouchCooldown = 0;
    HitState mHitState = HitState::Healthy;
    sead::Quatf mInitQuat = sead::Quatf::unit;
};

static_assert(sizeof(PackunFlower) == 0x1c8);

#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class JointSpringControllerHolder;
class Nerve;
}  // namespace al
class ActorStateSupportFreeze;
class PipePackunBody;
class PipePackunWaterSurfaceModel;

/** @brief Pipe Piranha Plant: a Piranha Plant head that stretches in and out on a long stem. */
class PipePackun : public al::LiveActor {
public:
    /** @brief How the head moves along the stem. */
    enum class MoveType : s32 {
        Extend = 0,     ///< Extends to the end of the stem, then falls asleep.
        Reciprocate = 1 ///< Keeps moving back and forth between both ends of the stem.
    };

    explicit PipePackun(const char* pName);
    /** @brief Destroys the Pipe Piranha Plant. */
    ~PipePackun() override = default;

    void kill() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    void killSwitch();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableTrample(const al::SensorMsg* pMsg) const;
    void hitInvincibleAttack(const al::SensorMsg* pMsg, al::HitSensor* pOther);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableSupportFreeze() const;
    void control() override;

    void exeWait();
    void exeSleepStart();
    void exeSleep();
    void exeMove();
    void exeTrampled();
    void exeAttackSuccess();
    void exeSupportFreeze();
    void exeDown();

private:
    PipePackunBody* mBody = nullptr;
    PipePackunWaterSurfaceModel* mWaterSurfaceModel = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    al::JointSpringControllerHolder* mJointSpringControllerHolder;
    const al::Nerve* mNextNerve = nullptr;
    al::HitSensor* mTrampleSensor = nullptr;
    al::LiveActor* mHoleModel = nullptr;
    sead::Vector3f mUpDir = sead::Vector3f::ey;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    f32 mMinCoord = 0.0f;
    f32 mCoord = 0.0f;
    f32 mTrampleStartCoord = 0.0f;
    f32 mTrampleDepth = 500.0f;
    s32 mTrampleFrame = 50;
    f32 mLeafDegree = 0.0f;
    f32 mMoveSpeed = 7.0f;
    MoveType mMoveType = MoveType::Extend;
    s32 mInvincibleTimer = 0;
    bool mIsDamageType2D = false;
    bool mIsMoveBack = false;
    bool mIsInWater = false;
    bool mIsTrampledBySensor = false;
};

static_assert(sizeof(PipePackun) == 0x1c0);

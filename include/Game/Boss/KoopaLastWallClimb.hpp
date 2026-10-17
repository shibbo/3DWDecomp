#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class KeyPoseKeeper;
}
class KoopaLastStateAttackBreathFire;
class KoopaLastStateTop;

/** @brief How a final-battle wall-climbing Koopa behaves once it reaches its last key pose. */
enum class KoopaLastWallClimbBehaviorType : s32 {
    Hide = 0,
    Jump = 1,
    RouteDokan = 2,
    DoubleCherry = 3,
    Top = 4,
    Attack = 5,
};

class KoopaLastWallClimb : public al::LiveActor {
public:
    explicit KoopaLastWallClimb(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void startLastPowDamage();

    void exeAppear();
    void exeLand();
    void exeClimb();
    void exeWait();
    void exeClimbEnd();
    void exeEnd();
    void exeJumpWait();
    void exeJumpStart();
    void exeJump();
    void exeAttack();
    void exeTop();
    void exeChaseRouteDokan();
    void exePowBlockDamage();
    void exePowBlockDamageFall();
    void exeLastPowDamage();
    void exeHide();

private:
    bool mIsUseAppear = false;
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    sead::Vector3f mClippingTrans = {0.0f, 0.0f, 0.0f};
    KoopaLastWallClimbBehaviorType mBehaviorType = KoopaLastWallClimbBehaviorType::Hide;
    sead::Vector3f mAppearTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mJumpVelocity = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mDamageVelocity;
    f32 mMoveSpeed = 0.0f;
    f32 mMoveDistance = 0.0f;
    f32 mKeyDistance = 0.0f;
    f32 mUnk190 = 0.0f;
    f32 mUnk194 = 0.0f;
    f32 mUnk198 = 0.0f;
    s32 mLandWaitTime = 0;
    s32 mWaitTime = 0;
    KoopaLastStateAttackBreathFire* mAttackState = nullptr;
    KoopaLastStateTop* mTopState = nullptr;
};

static_assert(sizeof(KoopaLastWallClimb) == 0x1B8);

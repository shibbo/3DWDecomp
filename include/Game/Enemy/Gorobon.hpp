#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class BreakModel;
class JointAimInfo;
}  // namespace al
class ActorStateSupportFreeze;
class ItemStatePlayerHold;
class TargetFinder;

/** @brief Rolling rock enemy (Gorobon) that can be carried, thrown and kicked by the player. */
class Gorobon : public al::LiveActor {
public:
    explicit Gorobon(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void control() override;
    void updateCollider() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isRockState() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void startSeHit();
    bool isEnableHold(al::HitSensor* pSensor);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void initAtBossStage();
    void forceBreak();
    void appearRock();

    void exeBreak();
    void exeSinkMagma();
    void exeDeath();
    void exeWaitGround();
    void exeWaitGroundWithStageSwitch();
    void exeAppearSign();
    void exeAppear();
    void startEffect();
    void exeLand();
    void exeDemoWait();
    void exeWalk();
    void endWalk();
    void exeRockStart();
    void exeRockStartAgain();
    void exeRockWait();
    void exeRockEnd();
    void exeRecovery();
    void exeLandRecovery();
    void exePlayerHold();
    void endHold();
    void exeRockThrow();
    void exeRockSpinShot();
    void exeSupportFreeze();

    bool mIsBossStage = false;  // 0x144
    bool mIsBossDemo = false;   // 0x145

private:
    bool isNerveInactive() const;

    bool mIsRotateForward = true;
    bool mIsRecover = true;
    bool mIsEnableAppearSign = true;
    bool mIsSinkByFire = false;
    f32 mRotateX = 0.0f;
    s32 mRebirthTime = 200;
    s32 mHitCoolTime = 0;
    s32 mReflectCoolTime = 0;
    s32 mAttackCoolTime = 0;
    s32 mHoldTime = 0;
    sead::Vector3f mFrontDir = sead::Vector3f::ez;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    sead::Vector3f mSideDir = sead::Vector3f::zero;
    sead::Vector3f mThrowVelocity = sead::Vector3f::zero;
    sead::Vector3f mAppearSignPos = sead::Vector3f::zero;
    sead::Vector3f mPrevTrans = sead::Vector3f::zero;
    sead::Quatf mInitQuat = sead::Quatf::unit;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    TargetFinder* mTargetFinder = nullptr;
    al::HitSensor* mHolderSensor = nullptr;
    al::BreakModel* mBreakModel = nullptr;
    al::ComboCounter* mComboCounter = new al::ComboCounter();
    ItemStatePlayerHold* mStatePlayerHold = nullptr;
    al::JointAimInfo* mJointAimInfo = nullptr;
};
static_assert(sizeof(Gorobon) == 0x228);

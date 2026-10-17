#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class BreakModel;
class JointAimInfo;
template <class T>
class DeriveActorGroup;
}  // namespace al

class BossDemoStartInfo;
class BossGorobonRock;
class BossStateDemoStart;
class Gorobon;
class MeraWanwan;
class TargetFinder;

/** @brief The rolling Gorobon boss (and its gatekeeper variant guarding a stage). */
class BossGorobon : public al::LiveActor {
public:
    explicit BossGorobon(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void setColorAnim(al::LiveActor* pActor, const char* pAnimName);
    void setVisibilityAnim();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isInvincibleNerve();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void appear() override;
    void control() override;
    void exeDemoBattleStart();
    void exePrepareBattleStart();
    void exeWalk();
    void chasePlayer();
    void checkGorobonPosition();
    void exeWalkSlowDown();
    void exeJumpStart();
    void exeJump();
    void exeLand();
    void revivalGorobon();
    void tryAppearMeraWanwan();
    void exeWait();
    void exeSpinStart();
    void exeSpin();
    void createBossGorobonRock();
    void exeSpinEnd();
    void exeDamage();
    void exeDown();
    ~BossGorobon() override;

private:
    bool mIsGateKeeper = false;                                   // 0x144
    bool mIsDamageableSpinStart = false;                          // 0x145
    s32 mHitPoint = 3;                                            // 0x148
    s32 mGorobonNum = 0;                                          // 0x14C
    s32 mMeraWanwanNum;                                           // 0x150
    s32 mInvincibleTimer = 0;                                     // 0x154
    f32 mRollDegree = 0.0f;                                       // 0x158
    f32 mTiltDegree = 0.0f;                                       // 0x15C
    f32 mWalkSpeed = 5.0f;                                        // 0x160
    f32 mStageRadius = 1200.0f;                                   // 0x164
    f32 mOffsetRadiusGorobon = 300.0f;                            // 0x168
    sead::Vector3f mFrontDir = sead::Vector3f::ez;                // 0x16C
    sead::Vector3f mStageCenterPos = sead::Vector3f::zero;        // 0x178
    sead::Matrix34f mWalkEffectMtx = sead::Matrix34f::ident;      // 0x184
    TargetFinder* mTargetFinder = nullptr;                        // 0x1B8
    al::DeriveActorGroup<Gorobon>* mGorobonGroup = nullptr;       // 0x1C0
    al::DeriveActorGroup<MeraWanwan>* mMeraWanwanGroup = nullptr; // 0x1C8
    al::DeriveActorGroup<BossGorobonRock>* mRockGroup = nullptr;  // 0x1D0
    BossDemoStartInfo* mDemoStartInfo = nullptr;                  // 0x1D8
    BossStateDemoStart* mStateDemoStart = nullptr;                // 0x1E0
    al::JointAimInfo* mEyeAimInfo = nullptr;                      // 0x1E8
    al::BreakModel* mBreakModel = nullptr;                        // 0x1F0
};
static_assert(sizeof(BossGorobon) == 0x1f8);

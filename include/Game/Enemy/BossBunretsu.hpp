#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorSensorControllerList;
class CameraInfo;
}  // namespace al

class BossBunretsuChip;
class BossBunretsuCore;
class BossDemoStartInfo;
class BossStateDemoStart;
class DoubleMario;
class RingBeamerBeam;
class TargetFinder;

/** @brief The splitting boss (Bunretsu): a body made of chunks around a core that jumps at the
 * players, splits up when stomped and gathers again. */
class BossBunretsu : public al::LiveActor {
public:
    static s32 calcAppearDoubleMarioNum(const al::ActorInitInfo& rInfo);
    explicit BossBunretsu(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void calcAnim() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isAttackable() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void startBreakup();
    void startGather(bool isRecover);
    bool startJump();
    bool receiveDamage();
    s32 receivedDamageNum() const;
    void startDeath();
    bool isBodySizeSmall() const;
    void exeWait();
    void exeAppear();
    void exeDemo();
    void exeBattleStart();
    void exeJumpStart();
    bool faceToTarget();
    void exeJump();
    f32 calcJumpVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rTarget, f32 gravity,
                         f32 jumpSpeed);
    void exeJumpEnd();
    void exeHipDrop();
    void exeHipDropFall();
    void exeHipDropEnd();
    void exeTrample();
    void exeBreakup();
    void exeBreakupWait();
    void exeGather();
    void exeGatherEnd();
    void exeGatherEndFall();
    bool isReceivableAttack() const;
    ~BossBunretsu() override;

    const sead::Vector3f& getInitTrans() const { return mInitTrans; }

    /** @brief Gets the core, which the chunks walk back to. */
    BossBunretsuCore* getCore() const { return mCore; }

private:
    al::ActorSensorControllerList* mSensorControllerList = nullptr;  // 0x148
    BossStateDemoStart* mStateDemoStart = nullptr;                    // 0x150
    BossDemoStartInfo* mDemoStartInfo = nullptr;                      // 0x158
    s32 mHitPoint = 3;                                                // 0x160
    BossBunretsuChip** mChips = nullptr;                              // 0x168
    BossBunretsuCore* mCore = nullptr;                                // 0x170
    al::LiveActor* mTransformModel = nullptr;                         // 0x178 model while gathering
    DoubleMario** mDoubleMarios = nullptr;                            // 0x180
    s32 mDoubleMarioNum = 0;                                          // 0x188
    TargetFinder* mTargetFinder = nullptr;                            // 0x190
    sead::Vector3f mJumpTarget;                                       // 0x198
    s32 mJumpCount = 0;                                               // 0x1A4
    s32 mHipDropStep = 0;                                             // 0x1A8
    al::CameraInfo* mCameraInfo = nullptr;                            // 0x1B0
    sead::Vector3f mInitTrans = {0.0f, 0.0f, 0.0f};                   // 0x1B8
    s32 mChipNum = 30;                                                // 0x1C4
    s32 mActiveChipNum = 30;                                          // 0x1C8
    bool mIsFirstJump = true;                                         // 0x1CC
    sead::Vector3f mCameraLookAtPos = {0.0f, 0.0f, 0.0f};             // 0x1D0
    s32 mLevel = 1;                                                   // 0x1DC
    RingBeamerBeam** mRingBeams = nullptr;                            // 0x1E0
    s32 mRingBeamNum = 0;                                             // 0x1E8
};
static_assert(sizeof(BossBunretsu) == 0x1f0);

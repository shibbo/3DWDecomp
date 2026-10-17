#pragma once

#include <basis/seadTypes.h>
#include <math/seadBoundBox.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class BossWackun;
class BossWackunHand;

/** @brief The moving cube body of BossWackun that tumbles and slides across the arena. */
class BossWackunBody : public al::LiveActor {
public:
    BossWackunBody(BossWackun* pBoss, BossWackunHand* pHand);

    void makeActorDead() override;
    void kill() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    s32 calcNextMoveTargetIndex(bool isForward) const;
    void nextMoveTarget(bool isForward);
    bool isTurnEnd() const;
    bool isDamage() const;
    void startDemo();
    void startBattle();
    void startWait();
    void startMove(const sead::Vector3f& rAxis, s32 rotateType, s32 moveNum, s32 moveStep,
                   s32 moveTime);
    void startRotate(s32 rotateType);
    void startLand(s32 rotateType);
    void startStandUp();
    void startRevival();
    void startDown(const sead::Vector3f& rDir);
    void exeStartDemo();
    void updatePose();
    void exeWait();
    void exeTurn();
    void exeMoveStart();
    void exeMove();
    void exeMoveEnd();
    void exeDamage();
    void exeRecover();
    void exeDown();
    ~BossWackunBody() override;

    /** @return Local position of the body relative to the boss (where the attack sign points). */
    const sead::Vector3f& getSignLocalDir() const { return mLocalTrans; }

    /** @return How many times the boss has been damaged. */
    s32 getDamageCount() const { return mDamageCount; }

private:
    void startMoveAction(const char* pRight, const char* pLeft, const char* pDown,
                         const char* pUp);

    BossWackun* mBoss;                                                 // 0x148
    BossWackunHand* mHand;                                             // 0x150
    sead::BoundBox3f mAttackBox{{-185.0f, -185.0f, -110.0f}, {185.0f, 185.0f, -20.0f}};  // 0x158
    sead::BoundBox3f mTrampleBox{{-195.0f, -195.0f, -100.0f}, {195.0f, 195.0f, -20.0f}};  // 0x170
    sead::Quatf mLocalQuat = sead::Quatf::unit;                        // 0x188
    sead::Vector3f mLocalTrans = {175.0f, 175.0f, 0.0f};               // 0x198
    f32 mTurnDegree = 0.0f;                                            // 0x1A4
    f32 mTurnStartDegree = 0.0f;                                       // 0x1A8
    s32 mDamageCount = 0;                                              // 0x1AC
    sead::Vector3f mDownDir = sead::Vector3f::ez;                      // 0x1B0
    sead::Vector3f mMoveStartTrans = {175.0f, 175.0f, 0.0f};           // 0x1BC
    sead::Vector3f mMoveTargetTrans = {175.0f, 175.0f, 0.0f};          // 0x1C8
    s32 mMoveTargetIndex = 0;                                          // 0x1D4
    bool mIsTurn = false;                                              // 0x1D8
    s32 mMoveNum = 1;                                                  // 0x1DC
    bool mIsMoveForward = true;                                        // 0x1E0
    s32 mMoveStep = 20;                                                // 0x1E4
    s32 mMoveTime = 30;                                                // 0x1E8
};
static_assert(sizeof(BossWackunBody) == 0x1f0);

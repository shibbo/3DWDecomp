#pragma once
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class RumbleCalculatorCosMultLinear;
}  // namespace al

class ActorJointLookController;
class TentackBase;
class TentackHill;
class TentackRockFaller;

/** @brief Main body (head) of the Tentack boss, rising out of the floor between tentacle attacks. */
class TentackHead : public al::LiveActor {
public:
    TentackHead(const char* pName, TentackBase* pHost, bool isSubHead);
    ~TentackHead() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isPushSensor(const al::HitSensor* pSensor) const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeDemoStart();
    void exeWait();
    void updateLookPlayer();
    bool turnToTargetGently(const sead::Vector3f& rTarget, f32 speed);
    void exeAttackRockStart();
    void exeAttackTentacleStart();
    void exeDamage();
    void exeBack();
    void exeBackWait();
    void exePushSign();
    void exePush();
    void exeEat();
    void exeDisappear();
    void exeShot();
    void exeCry();
    void exeDemoEnd();
    bool turnToDirectionGently(const sead::Vector3f& rDir, f32 speed);
    void changeLookTarget();
    bool tryStartActionAttackRockIfWait();
    bool isWaitAll() const;
    void startActionAttackTentacle();
    bool tryStartActionEat();
    bool tryStartActionEatAndDisappear();
    bool tryStartActionShot();
    bool tryStartActionCry();
    void setWait();
    void setWaitFixed();
    void setDisappear();
    bool isWaitFixed() const;
    bool isBackOrPush() const;
    bool isDamageAction() const;
    bool isDemoEnd() const;
    void cancelDemoAppear();
    static f32 getHeadRadius();

private:
    bool isEnableAttack() const;

public:
    TentackBase* mHost;                         // 0x148
private:
    TentackHill* mHill;                         // 0x150 floor model around the head
public:
    TentackRockFaller* mRockFaller;             // 0x158
    s32 mDamage;                                // 0x160 number of hits taken
    sead::Vector3f mFrontDir;                   // 0x164 initial facing direction
private:
    sead::Matrix34f mLightMtx;                  // 0x170 appearance spotlight
    f32 mTurnSpeed;                             // 0x1a0 turn step kept after a target change
    al::LiveActor* mLookPlayer;                 // 0x1a8
    s32 mLookChangeStep;                        // 0x1b0 frames left blending to a new target
    al::RumbleCalculatorCosMultLinear* mRumble; // 0x1b8 squash when trampled
    s32 mRumbleStep;                            // 0x1c0 trample squash frame, -1 when idle
    al::HitSensor** mPushSensors;               // 0x1c8 BodySpine1..3
    bool mIsReappearRockFaller;                 // 0x1d0
    ActorJointLookController* mJointLook;       // 0x1d8
    bool mIsSubHead;                            // 0x1e0
};
static_assert(sizeof(TentackHead) == 0x1e8);

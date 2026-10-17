#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorStateSupportFreeze;
class EnemyStateBlowDown;

/** @brief Charging bull enemy (Bull) that wears a helmet which breaks after the first hit. */
class Bull : public al::LiveActor {
public:
    explicit Bull(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnablePush() const;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableTrample(const al::SensorMsg* pMsg) const;
    bool isEnableBlowDown() const;
    void notice();
    bool isEnableJumpPanel() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableSupportFreeze() const;
    void control() override;
    void reappear() override;
    bool isEndRun() const;
    void updateVelocity();
    bool tryNextTarget();
    bool tryRevival();

    void exeWait();
    void exeFind();
    void exeRun();
    bool trySendMsgAttackCollide();
    void exeBrake();
    void exeStop();
    void exeTurn();
    void exeJump();
    void exeBlow();
    void exeTrampled();
    void exeTrampledEnd();
    void exeAngry();
    void exeRevival();
    void exeBlowDown();
    void exePressDown();
    void exeAttackSuccess();
    void exeSupportFreeze();

private:
    al::LiveActor* mTarget = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    sead::Vector3f mInitScale;
    s32 mLife = 2;
    s32 mMaxLife = 2;
    s32 mRevivalTimer = 0;
    bool mIsCovered = true;
    bool mIsNoticed = false;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    sead::Vector3f mInitFront = sead::Vector3f::ez;
};

static_assert(sizeof(Bull) == 0x198);

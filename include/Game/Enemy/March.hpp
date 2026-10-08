#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorStateSupportFreeze;
class EnemyAttachItem;
class EnemyStateBlowDown;
class TargetFinder;
class WalkerStateChase;
class WalkerStateChaseParam;
class WalkerStateFindPlayer;
class WalkerStateFindPlayerParam;
class WalkerStateJump;
class WalkerStateWander;
class WalkerStateWanderParam;

/**
 * @brief Wind-up enemy that marches in a parade, scatters when disturbed and then wanders
 * around and chases players. It can carry an item and be frozen by touch.
 */
class March : public al::LiveActor {
public:
    /** @brief Item type whose attached item spins above the March instead of following a joint. */
    static constexpr s32 cItemTypeSpin = 30;

    March(const char* pName, const char* pWalkAction, const char* pItemName, s32 itemType);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableDown() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void startClipped() override;
    void endClipped() override;
    void reappear() override;

    void exeParade();
    void rotateItem();
    void rotateZenmai(f32 speed);
    void exeScatter();
    void exePuzzlement();
    void getRandomDir(sead::Vector3f* pDir) const;
    void exeWander();
    bool isActive(f32 distance) const;
    void exeFindPlayer();
    void exeChase();
    void exeAttack();
    void exeWait();
    void exeJumpJumpPanel();
    void exePressDown();
    void exeBlowDown();
    void exeSupportFreezeParade();
    void exeSupportFreeze();

    void startScatter(const sead::Vector3f& rTarget);
    void startFreeze(const al::LiveActor* pTouchActor);
    al::LiveActor* getFreezeTouchActor() const;
    void startParade();
    bool isFreeze() const;
    al::LiveActor* getLinkedActor() override;

private:
    TargetFinder* mTargetFinder = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateFindPlayer* mStateFindPlayer = nullptr;
    WalkerStateWanderParam* mWanderParam = nullptr;
    WalkerStateChaseParam* mChaseParam = nullptr;
    WalkerStateFindPlayerParam* mFindPlayerParam = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    ActorStateSupportFreeze* mStateSupportFreezeParade = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    WalkerStateJump* mStateJumpJumpPanel = nullptr;
    EnemyAttachItem* mAttachItem = nullptr;
    const char* mItemName;
    const char* mWalkAction;
    s32 mItemType;
    f32 mZenmaiAngle = 0.0f;
    sead::Vector3f mPuzzlementDir = sead::Vector3f::zero;
    bool mIsEnableCliffCheck = true;
    sead::Vector3f mItemRotate = sead::Vector3f::zero;
    s32 mAttackInvalidTime = 0;
};

static_assert(sizeof(March) == 0x1e0);

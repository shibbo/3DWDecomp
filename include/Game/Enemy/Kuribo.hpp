#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorMicRumbler;
class ActorStateSupportFreeze;
class BoxKuribo;
class EnemyStateBlowDown;
class TargetFinder;
class WalkerStateChase;
class WalkerStateChaseParam;
class WalkerStateFall;
class WalkerStateFindPlayer;
class WalkerStateJump;
class WalkerStateRouteDokanMove;
class WalkerStateWander;
class WalkerStateWanderParam;

/**
 * @brief Goomba: walks around, chases players, jumps on trampolines and jump panels, can be
 * frozen by touch and can drop a Goomba box when defeated.
 */
class Kuribo : public al::LiveActor {
public:
    explicit Kuribo(const char* pName);

    void control() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void cancelStayBySwitch();
    void killBySwitch();
    void appearBySwitch();
    void reappear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableDown() const;
    bool isEnableAttack() const;
    bool isEnableRouteDokan() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isDown() const;

    void exeWait();
    bool isActive(f32 distance) const;
    void exeFall();
    void exeWander();
    void exeFindPlayer();
    void exeChase();
    void exeAttack();
    void exePressDown();
    void exeBlowDown();
    void exeSupportFreeze();
    void exeRouteDokan();
    void exeRouteDokanDeath();
    void exeJumpTrampoline();
    void exeJumpJumpPanel();

private:
    void* _148 = nullptr;
    TargetFinder* mTargetFinder = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateFindPlayer* mStateFindPlayer = nullptr;
    WalkerStateJump* mStateAttack;  // not initialized by the constructor
    WalkerStateJump* mStateJumpTrampoline = nullptr;
    WalkerStateJump* mStateJumpJumpPanel = nullptr;
    WalkerStateFall* mStateFall = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    WalkerStateRouteDokanMove* mStateRouteDokanMove = nullptr;
    ActorMicRumbler* mMicRumbler = nullptr;
    WalkerStateWanderParam* mWanderParam = nullptr;
    WalkerStateChaseParam* mChaseParam = nullptr;
    s32 mAttackInvalidTime = 0;
    s32 mClippingInvalidTime = -1;
    bool mIsStayBySwitch = false;
    BoxKuribo* mBoxKuribo = nullptr;
    void* _1d8 = nullptr;
    sead::Vector3f _1e0 = sead::Vector3f::zero;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    sead::Vector3f mInitFront = sead::Vector3f::ez;
    bool mIsRequestOnCollide = false;
};

static_assert(sizeof(Kuribo) == 0x208);

#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorJointLookController;
class ActorMicRumbler;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class KuribonStateReverse;
class TargetFinder;
class WalkerStateChase;
class WalkerStateFindPlayer;
class WalkerStateRouteDokanMove;
class WalkerStateWander;

/** @brief Goomba-like walking enemy (Kuribon / KuribonBig) that can be flipped, kicked and frozen. */
class Kuribon : public al::LiveActor {
public:
    explicit Kuribon(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void resetInitTrans();
    void appear() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isPushStrong() const;
    bool isEnablePush() const;
    bool isEnableAttack() const;
    bool isEnableRotueDokan() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableKick() const;
    void startKick(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool isEnableDamage() const;
    bool isEnableReverse() const;
    void startReverse(const al::LiveActor* pAttacker);
    bool isEnableDown() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isAfterKick() const;
    bool isReverseStart() const;
    void startScreenHit();

    void exeWait();
    bool isActive(f32 distance) const;
    void exeWaitGateKeeper();
    void exeWander();
    void exeFindPlayer();
    void exeAttack();
    void exeChase();
    void exeReverse();
    void exeRouteDokan();
    void exeRouteDokanDeath();
    void exePressDown();
    void exeBlowDown();
    void exeSupportFreeze();

private:
    bool isReverseInAir() const;

    TargetFinder* mTargetFinder = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateFindPlayer* mStateFindPlayer = nullptr;
    WalkerStateRouteDokanMove* mStateRouteDokanMove = nullptr;
    KuribonStateReverse* mStateReverse = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    ActorMicRumbler* mMicRumbler = nullptr;
    ActorJointLookController* mJointLookController = nullptr;
    bool mIsBig = false;
    bool mIsGateKeeper = false;
    sead::Vector3f mInitTrans = {0.0f, 0.0f, 0.0f};
    f32 mActiveDistance = 3500.0f;
    f32 mDeactiveDistance = 4000.0f;
};

static_assert(sizeof(Kuribon) == 0x1B0);

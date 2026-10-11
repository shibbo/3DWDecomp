#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class EnemyStateBlowDown;
class TargetFinder;
class WalkerStateChase;
class WalkerStateChaseParam;
class WalkerStateJump;
class WalkerStateRouteDokanMove;
class WalkerStateWander;
class WalkerStateWanderParam;

/**
 * @brief Mini Goomba: a tiny walking enemy that chases players, is blown away by microphone
 * input, touch flicks and nearby hip drops, and can appear from a stage switch.
 */
class KuriboMini : public al::LiveActor {
public:
    explicit KuriboMini(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void startBySwitch();
    void resetInitTrans();
    void control() override;
    bool isEnableDown() const;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeAppearStart();
    void exeAppearLoop();
    void exeAppearEnd();
    void exeSwitchWait();
    void exeWait();
    void exeWander();
    void exeRunStart();
    void exeChase();
    void exeLost();
    void exeAttack();
    void exeRouteDokan();
    void exeRouteDokanDeath();
    void exePressDown();
    void exeBlowDown();
    void exeBlow();
    void exeFlick();
    void exeMicReaction();
    void exeLand();
    void exeRecover();

    void appearStart();
    bool isEnableResetPosition() const;

private:
    TargetFinder* mTargetFinder = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateJump* mStateAttack = nullptr;
    WalkerStateRouteDokanMove* mStateRouteDokanMove = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    WalkerStateWanderParam* mWanderParam = nullptr;
    WalkerStateChaseParam* mChaseParam = nullptr;
    sead::Vector3f mFlickPos = sead::Vector3f::zero;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    s32 mHipDropBlowDelay = 0;
    al::HitSensor* mHipDropSensor;
};

static_assert(sizeof(KuriboMini) == 0x1B0);

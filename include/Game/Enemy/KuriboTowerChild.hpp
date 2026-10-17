#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Enemy/KuriboTowerNode.hpp"

class ActorMicRumbler;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class TargetFinder;
class WalkerStateChase;
class WalkerStateFindPlayer;
class WalkerStateRailMove;
class WalkerStateWander;

/** @brief A Goomba stacked in a Goomba Tower; the bottom one walks around carrying the others. */
class KuriboTowerChild : public KuriboTowerNode {
public:
    explicit KuriboTowerChild(const char* pName);

    void control() override;
    bool isEnableDown() const;
    void init(const al::ActorInitInfo& rInfo) override;
    void reappear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isValidPush() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void endSleep();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void endInit() override;

    void exeStandBy();
    void exeWait();
    bool isActive(f32 distance) const;
    void exeSleep();
    void exeWander();
    void exeRailMove();
    void exeFindPlayer();
    void exeChase();
    void exeAttack();
    void exeFall();
    void exeLand();
    void exePressDown();
    void exeHipDropDown();
    void exeBlowDown();
    void exeSupportFreeze();
    void exeSupportFreezeSync();
    void exeEatDown();
    void exeSwallow();
    void exeSurprise();

    bool isNerveHipDropDown() const override;
    bool isNerveSupportFreeze() const override;
    void requestRootBehavior() override;
    void requestRelease() override;
    bool requestSurprise() override;
    void requestSupportFreezeSync() override;
    void requestEndSupportFreezeSync() override;
    void requestAttackReaction(al::HitSensor* pSensor) override;
    f32 getOffsetY() const override;

private:
    const char* getStandByActionName() const;

    TargetFinder* mTargetFinder = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateFindPlayer* mStateFindPlayer;
    WalkerStateRailMove* mStateRailMove = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    ActorMicRumbler* mMicRumbler = nullptr;
    al::HitSensor* mFloorSensor = nullptr;
    sead::Vector3f mFloorTouchTrans = sead::Vector3f::zero;
    al::HitSensor* mEatSensor = nullptr;
    sead::Vector3f mEatStartTrans;
    al::HitSensor* mHipDropSensor = nullptr;
};

static_assert(sizeof(KuriboTowerChild) == 0x1E0);

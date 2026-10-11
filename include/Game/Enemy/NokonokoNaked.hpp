#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class Koura;
class Nokonoko;
class TargetFinder;
class WalkerStateChase;
class WalkerStateChaseParam;
class WalkerStateFindPlayer;
class WalkerStateRailMove;
class WalkerStateWander;
class WalkerStateWanderParam;

/** @brief Koopa Troopa without its shell: runs away, chases players and goes back to its shell. */
class NokonokoNaked : public al::LiveActor {
public:
    NokonokoNaked(const char* pName, Nokonoko* pNokonoko);

    void control() override;
    void updateKouraChaseState();
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isReceivableAttack();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void endClipped() override;
    void startEject();
    void startEjectUpper();

    void exeWait();
    bool isActive(f32 distance) const;
    bool isKouraChasable() const;
    void exeWander();
    void exeFindPlayer();
    void exeAttack();
    void exeChaseStart();
    void exeChase();
    void exeChaseEnd();
    void exeRailMove();
    void exeTrampleKoura();
    void exeTrampleKouraSlide();
    void exeAfterEject();
    void exeChaseKouraRetry();
    void exeFaceToKoura();
    void exeChaseKouraStart();
    void exeChaseKoura();
    void exeChaseKouraBreak();
    void exeChaseKouraEnd();
    void exeAttachKoura();
    void exeWaitAttach();
    void exeRunAway();
    void exePressDown();
    void exePressDownPress();
    void exePressDownBlow();
    void exeBlowDown();
    void exeSupportFreeze();

private:
    Koura* getKoura() const;

    bool mIsNearKoura = true;
    bool mIsGiveUpChaseKoura = false;
    Nokonoko* mNokonoko;
    s32 mKouraSlideFrame = 0;
    TargetFinder* mTargetFinder = nullptr;
    WalkerStateWander* mStateRunAway = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateFindPlayer* mStateFindPlayer = nullptr;
    WalkerStateRailMove* mStateRailMove = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    EnemyStateBlowDown* mStatePressDownBlow = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    WalkerStateWanderParam* mWanderParam = nullptr;
    WalkerStateWanderParam* mRunAwayParam = nullptr;
    WalkerStateChaseParam* mChaseParam = nullptr;
};

static_assert(sizeof(NokonokoNaked) == 0x1B8);

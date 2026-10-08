#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObjGroup;
}  // namespace al
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class SkipperTrampoline;
class TargetFinder;
class WalkerStateChase;
class WalkerStateWander;

/**
 * @brief Skipper (Skipsqueak): wanders around and chases players inside its chase area. When
 * stomped it flips over and turns into a SkipperTrampoline until it recovers.
 */
class Skipper : public al::LiveActor {
public:
    explicit Skipper(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killSwitch();
    void kill() override;
    bool isEnableKill() const;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isNerveAttackable() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableDown() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeStandByWander();
    void exeWander();
    void exeTurnToPlayer();
    void exeFind();
    void exeChase();
    void exeAngry();
    void exeTrampled();
    void exeFlip();
    void exeAfter();
    void exeRecoverEnd();
    void exeLand();
    void exeFall();
    void exeBlowDown();
    void exeSupportFreeze();
    void exeAttackSuccess();
    bool isNerveDown() const;

private:
    bool isNerveDownOrAfter() const;
    bool isTargetLeftChaseArea(const al::LiveActor* pTarget) const;
    bool isEnableChaseTarget(const al::LiveActor* pTarget) const;

    TargetFinder* mTargetFinder = nullptr;                // 0x148
    EnemyStateBlowDown* mStateBlowDown;                   // 0x150
    WalkerStateChase* mStateChase = nullptr;              // 0x158
    WalkerStateWander* mStateWander = nullptr;            // 0x160
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x168
    al::AreaObjGroup* mChaseAreaGroup = nullptr;          // 0x170
    SkipperTrampoline* mTrampoline;                       // 0x178
};

static_assert(sizeof(Skipper) == 0x180);

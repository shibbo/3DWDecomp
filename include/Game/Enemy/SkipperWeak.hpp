#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class AreaObjGroup; class BreakModel; }
class TargetFinder;
class WalkerStateChase;
class WalkerStateWander;
class ActorStateSupportFreeze;
/** @brief Fragile Skipsqueak that wanders and chases nearby players. */
class SkipperWeak : public al::LiveActor {
public:
    explicit SkipperWeak(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void killSwitch();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeWander();
    void exeTurnToPlayer();
    void exeFind();
    void exeChase();
    void exeLand();
    void updateVelocity();
    void exeSupportFreeze();
    void exeAttackSuccess();
private:
    TargetFinder* mTargetFinder = nullptr;
    WalkerStateChase* mStateChase = nullptr;
    WalkerStateWander* mStateWander = nullptr;
    ActorStateSupportFreeze* mFreeze = nullptr;
    al::AreaObjGroup* mChaseArea = nullptr;
    al::BreakModel* mBreakModel = nullptr;
};
static_assert(sizeof(SkipperWeak) == 0x178);

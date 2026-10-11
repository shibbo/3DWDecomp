#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportFreeze;
class BossBunretsu;
class EnemyStateBlowDownNoCollider;
class TargetFinder;
class WalkerStateChase;

/**
 * @brief One body chunk of the splitting boss (BossBunretsu). Chunks fly apart when the boss splits
 * up, chase the players or walk back to the core, and gather back into the body.
 */
class BossBunretsuChip : public al::LiveActor {
public:
    BossBunretsuChip(BossBunretsu* pParent, s32 index, const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isSplashing() const;
    bool isAttackable() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isCollidedGround() const;
    bool isGathering() const;
    bool isReceivableAttack() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void updateCollider() override;
    bool isOnGround() const;
    void startBreakup();
    void startGather();
    void endGather();
    void onDeath();
    void onDamage();
    bool isGatherd() const;

    void exeWait();
    void movementProc();
    void exeMoveToCore();
    void exeFindPlayer();
    void exeChase();
    void exeSplash();
    void exeSplashLand();
    void exeGatherStart();
    void exeGather();
    void exeGatherEnd();
    void exePressDown();
    void exeBlowDown();
    void exeDisappearPrepare();
    void exeDisappear();
    void exeSupportFreeze();
    void exeDeath();

private:
    BossBunretsu* mParent;                                 // 0x148
    s32 mIndex;                                            // 0x150 delays the gathering
    TargetFinder* mTargetFinder = nullptr;                 // 0x158
    WalkerStateChase* mStateChase = nullptr;               // 0x160
    EnemyStateBlowDownNoCollider* mStateBlowDown = nullptr;  // 0x168
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x170
    bool mIsCollidedGround = false;                        // 0x178
    bool mIsOnGround = false;                              // 0x179
};
static_assert(sizeof(BossBunretsuChip) == 0x180);

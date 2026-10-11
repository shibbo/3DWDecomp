#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class BossBunretsu;

namespace al {
class RumbleCalculator;
}

/**
 * @brief Core of the splitting boss (BossBunretsu): the weak point that walks
 * around while the body is split up, and that has to be stomped to damage the boss.
 */
class BossBunretsuCore : public al::LiveActor {
public:
    BossBunretsuCore(BossBunretsu* pParent, const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isAttackable() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isReceivableAttack() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void startDemo();
    void startBreakup();
    void startGather();
    void endGather();

    void exeDemo();
    void exeWait();
    void exeSwoon();
    void exeSwoonEnd();
    void exeSplashStart();
    void exeSplash();
    void exeSplashLand();
    void exeStandup();
    void exeWalk();
    void exeTurnPrepare();
    void exeFarFromPlayerTurn();
    void exeFarFromPlayer();
    void exeTurn();
    void exeJumpSign();
    void exeJumpStart();
    void exeJump();
    void exeGatherStart();
    void exeGather();
    void exePressDown();
    void exeBlowDown();
    void exeDeath();

private:
    BossBunretsu* mParent;
    sead::Vector3f mTurnDir = {0.0f, 0.0f, 0.0f};
    s32 mMoveTime = 0;
    al::LiveActor* mTargetPlayer;  // not initialized by the constructor
    s32 mFireBallHitNum = 0;
    s32 mSmallGatherNum = 0;
    bool mIsRecover = false;
    al::RumbleCalculator* mRumble = nullptr;
};

static_assert(sizeof(BossBunretsuCore) == 0x180);

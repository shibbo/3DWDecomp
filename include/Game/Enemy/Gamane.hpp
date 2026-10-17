#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorStateSupportFreeze;
class CoinBlow;
class EnemyStateBlowDown;
class GamaneChameleon;

/**
 * @brief Coin-spewing chameleon frog enemy (Gamane). It wanders as an invisible/transparent
 * chameleon model and runs away from the player once hit, dropping coins on every hit.
 */
class Gamane : public al::LiveActor {
public:
    explicit Gamane(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    bool isTransparent();
    void reappear() override;
    void startGamaneAction(const char* pActionName);
    void killComplete(bool isNoReaction) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgAtTransparent(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf);
    bool receiveMsgAtReal(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void kill() override;

    void exeWait();
    bool isInvisible();
    void exeAppear();
    bool isGamaneActionEnd();
    void exeFind();
    void exeRun();
    bool runAway();
    void exeRunInvisible();
    void exeFall();
    void exeLandToDown();
    void exeLandToRecover();
    void exeReactionDamage();
    void exeReactionDamageWait();
    void exeReactionTail();
    void exeTrampled();
    void exeDown();
    void exeRecover();
    void exePressDownBlow();
    void appearCoinDie();
    void exeHipDropSpew();
    void appearCoinNormal();
    void exeBlowDown();
    void exeSupportFreeze();

private:
    s32 mCoinNum;
    sead::PtrArray<CoinBlow> mCoins;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    EnemyStateBlowDown* mStatePressDownBlow = nullptr;
    EnemyStateBlowDown* mStateReactionTail = nullptr;
    sead::Vector3f mAppearVelocity = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mRunDir = {0.0f, 0.0f, 0.0f};
    bool mIsRunStopped = true;
    s32 mDamageCoolTime = 0;
    sead::Vector3f mInitFront = sead::Vector3f::ez;
    GamaneChameleon* mChameleon = nullptr;
};

static_assert(sizeof(Gamane) == 0x1B0);

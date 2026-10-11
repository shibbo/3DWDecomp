#pragma once

#include <math/seadMatrix.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/LiveActor/LiveActor.hpp"

class ActorStateRouteDokanMove;
class BallStateFall;
class BallStateRolling;
class BallStateThrow;
class ItemStatePlayerHold;
class ItemStatePopUpFront;
class TouchCarryItemState;

/// Ball item that can be carried, thrown and kicked at enemies.
class Ball : public al::LiveActor {
public:
    explicit Ball(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableHold(al::HitSensor* pSensor);
    bool isEnableKick();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void kill() override;
    void updateCollider() override;
    void appearPopUpFront();
    void appearAbove();
    void reset();
    bool isPlayerHold() const;
    bool hideActor() override;

    void exeWait();
    void exePlayerHold();
    void exeDRCHold();
    void exeThrow();
    void startEffect();
    void countWallCollide();
    void exeKick();
    void exeFall();
    void exeRolling();
    void exeDamageThrow();
    void exeWaterBottom();
    void exePopUpFront();
    void exeRouteDokan();
    void exeRouteDokanThrow();

private:
    bool mIsRotateOnFall = true;
    s32 mHoldDisableTimer = 0;
    s32 mKnockDownDisableTimer = 0;
    s32 mWallCollideCount = 0;
    s32 mAttackDisableTimer = 0;
    s32 mRouteDokanDisableTimer = 0;
    f32 mColliderRadius = 0.0f;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    al::HitSensor* mHolderSensor = nullptr;
    const al::LiveActor* mTouchPointer = nullptr;
    al::ComboCounter* mComboCounter = new al::ComboCounter();
    ItemStatePopUpFront* mStatePopUpFront = nullptr;
    TouchCarryItemState* mStateTouchCarry = nullptr;
    ItemStatePlayerHold* mStatePlayerHold = nullptr;
    BallStateFall* mStateFall = nullptr;
    BallStateRolling* mStateRolling = nullptr;
    BallStateThrow* mStateThrow = nullptr;
    ActorStateRouteDokanMove* mStateRouteDokan;
    bool mIsSingleMode;
};

static_assert(sizeof(Ball) == 0x1e8);

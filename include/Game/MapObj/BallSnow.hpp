#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/LiveActor/LiveActor.hpp"

class BallStateFall;
class BallStateRolling;
class BallStateThrow;
class ItemStatePlayerHold;
class TouchCarryItemState;

/** @brief Throwable snow ball, e.g. left behind by a defeated snow Pokey. */
class BallSnow : public al::LiveActor {
public:
    explicit BallSnow(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableHold(al::HitSensor* pSensor);
    bool isEnableKick();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void kill() override;
    void control() override;
    void updateCollider() override;
    void reset();
    bool isPlayerHold() const;
    void requestPlayerRelease();

    void exeWait();
    void exePlayerHold();
    void exeDrcHold();
    void exeThrow();
    void startEffect();
    void countWallCollide();
    void exeKick();
    void exeFall();
    void exeRolling();
    void exeDamageThrow();
    void exeHide();
    bool hideActor() override;

private:
    bool mIsRotateOnFall = true;
    bool mIsBreakOnLand = false;
    bool mIsRebirth = false;
    s32 mHoldDisableTimer = 0;
    s32 mKnockDownDisableTimer = 0;
    s32 mHitDisableTimer = 0;
    s32 mClippingInvalidTimer = 0;
    s32 mWallCollideCount = 0;
    f32 mColliderRadius = 0.0f;
    sead::Vector3f mRebirthTrans = sead::Vector3f::zero;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    al::HitSensor* mHolderSensor = nullptr;
    const al::LiveActor* mTouchPointer = nullptr;
    al::ComboCounter* mComboCounter = new al::ComboCounter();
    ItemStatePlayerHold* mStatePlayerHold = nullptr;
    TouchCarryItemState* mStateTouchCarry = nullptr;
    BallStateFall* mStateFall = nullptr;
    BallStateRolling* mStateRolling = nullptr;
    BallStateThrow* mStateThrow = nullptr;
};

static_assert(sizeof(BallSnow) == 0x1e0);

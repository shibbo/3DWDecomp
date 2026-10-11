#pragma once

#include <math/seadMatrix.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class Nerve;
}  // namespace al

class ActorStateRouteDokanMove;
class BallStateFall;
class BallStateRolling;
class BallStateThrow;
class BallStateThrowParam;
class ItemStatePlayerHold;
class ItemStatePopUpFront;
class TouchCarryItemState;

/// Giant ball that can be carried, thrown and kicked around by a giga (mega) player.
class GigaBall : public al::LiveActor {
public:
    explicit GigaBall(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    virtual void attackSensorBody(al::HitSensor* pSelf, al::HitSensor* pOther);
    void requestRelease(al::HitSensor* pOther, BallStateThrowParam* pParam,
                        const al::Nerve* pNerve);
    virtual void attackSensorHold(al::HitSensor* pSelf, al::HitSensor* pOther);
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
    bool isEnablePlayerKnockDown(al::HitSensor* pPlayer, al::HitSensor* pSelf);

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
};

static_assert(sizeof(GigaBall) == 0x1E0);

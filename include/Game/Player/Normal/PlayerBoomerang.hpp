#pragma once

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Player/PlayerDef.hpp"

class IUsePlayerInput;

namespace al {
class ComboCounterWithSe;
struct ActorParamF32;
struct ActorParamS32;
}  // namespace al

/**
 * @brief Tunable parameters of the player's boomerang, read from the actor's parameter file.
 */
class PlayerBoomerangParam {
public:
    PlayerBoomerangParam(al::LiveActor* pActor);

    const al::ActorParamF32* mThrowHeight;        ///< 投げる高さ
    const al::ActorParamF32* mBrakeStrength;      ///< ブレーキ強さ
    const al::ActorParamF32* mReturnStrength;     ///< 戻り強さ
    const al::ActorParamS32* mEndStopTime;        ///< 端点停止時間
    const al::ActorParamF32* mSpeed;              ///< 速度
    const al::ActorParamF32* mRange;              ///< 到達距離
    const al::ActorParamF32* mGravity;            ///< 重力
    const al::ActorParamF32* mMaxFallSpeed;       ///< 落下最高速度
    const al::ActorParamF32* mAboveHeight;        ///< プレイヤーより上とみなす高低差
    const al::ActorParamF32* mTurnLimitDegree;    ///< ターン限界角度
    const al::ActorParamS32* mLostTime1;          ///< 見失い時間[1回目]
    const al::ActorParamS32* mLostTime2;          ///< 見失い時間[2回目]
    const al::ActorParamS32* mLostTime3;          ///< 見失い時間[3回目]
    const al::ActorParamF32* mReflectRate;        ///< 反射率
    const al::ActorParamS32* mBurnTime;           ///< 燃える時間
    const al::ActorParamF32* mBurnAirResistance;  ///< 空気抵抗[燃え時]
    const al::ActorParamF32* mBurnHitBrake;       ///< 衝突減速[燃え時]
    const al::ActorParamS32* mBreakInterval;      ///< 壊れた後のインターバル
};

static_assert(sizeof(PlayerBoomerangParam) == 0x90);

/**
 * @brief Boomerang thrown by Boomerang Mario. It flies forward, brakes, hovers at its end point and
 * then flies back to the player, who catches it. Walls and reflecting sensors bounce it back.
 */
class PlayerBoomerang : public al::LiveActor {
public:
    PlayerBoomerang(EPlayerChara chara, const IUsePlayerInput* pInput);

    void init(const al::ActorInitInfo& rInfo) override;
    void appearBoomerang(const al::LiveActor* pPlayer, const al::LiveActor* pHolder);
    void startThrow(const sead::Quatf& rQuat, const sead::Vector3f& rVelocity);
    bool tryKill();
    bool isMoving() const;
    void doBreak(const char* pReactionName);
    void forceEnd();
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool tryCatch();
    void doReflectSensor(const al::HitSensor* pSelf, const al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void control() override;
    void controlShowHideModel();
    void updateGravity();
    bool tryHitWall();
    void trySendMsgToCollision();
    bool tryReflectWall();
    void caught();
    f32 getRange() const;
    s32 getLostTime() const;
    bool isAbovePlayer() const;
    bool checkCollideFront() const;

    void exeWait();
    void exeMoveGo();
    void exeMoveBrake();
    bool isLostPlayer() const;
    void exeMoveStay();
    void exeMoveBack();
    void exeMoveLost();
    void exeBurn();
    void exeInterval();

private:
    EPlayerChara mChara;
    PlayerBoomerangParam* mParam = nullptr;
    sead::Vector3f mThrowDir = sead::Vector3f::ez;
    f32 mThrowSpeed = 0.0f;
    s32 mGoStep = 0;
    f32 mBackSpeed = 0.0f;
    s32 mLostCount = 0;
    s32 mReflectInterval = -1;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    f32 mColliderRadius = 0.0f;
    bool mIsReflected = false;
    const al::LiveActor* mPlayer = nullptr;
    al::ComboCounterWithSe* mComboCounter;
    bool mIsCatchTrigOn = false;
    const IUsePlayerInput* mInput;
    bool mIsEnableEcho = false;
};

static_assert(sizeof(PlayerBoomerang) == 0x1D0);

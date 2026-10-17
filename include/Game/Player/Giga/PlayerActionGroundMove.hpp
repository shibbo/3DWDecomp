#pragma once

#include <math/seadVector.h>
#include <prim/seadEnum.h>

#include "Player/IUsePlayerDashChecker.hpp"
#include "Player/PlayerAction.hpp"
#include "Player/PlayerActionArg.hpp"

class IUsePlayerDoubleMarioSpeed;
class IUsePlayerFlag;
class IUsePlayerInvincibleDash;
class IUsePlayerMoveSpeedScaler;
class PlayerConstParam;
class PlayerFigureDirector;
class PlayerTrigger;
class PlayerUprightMtxCalc;

/**
 * @brief Adjusts the move speed while an old player demo is replayed.
 * @param speed The speed the action computed.
 * @return The speed to use instead.
 */
f32 OldPlayerDemoAdjustSpeed(f32 speed);

/// The player's walking, running and dashing on the ground.
class PlayerActionGroundMove : public PlayerAction, public IUsePlayerDashChecker {
    SEAD_RTTI_OVERRIDE(PlayerActionGroundMove, PlayerAction)

public:
    /// The animation slots blended while moving (see IUsePlayerAnimator::setWeightSixfold).
    SEAD_ENUM(ESlotIndex, Walk, Run, Dash, SuperDash, InvincibleDash, DashStart)

    /// Which speed class the player is moving in.
    enum class EMoveState : s32 { Walk = 0, Dash = 1, SuperDash = 2 };

    /// Lets a super dash carry over into the next ground move (e.g. after a jump).
    struct SuperDashKeepInfo {
        bool mIsKeep;           // 0x0
        sead::Vector3f mFront;  // 0x4
    };

    PlayerActionGroundMove(PlayerActionArg* pArg,
                           const IUsePlayerDoubleMarioSpeed* pDoubleMarioSpeed,
                           const IUsePlayerInvincibleDash* pInvincibleDash,
                           const IUsePlayerMoveSpeedScaler* pMoveSpeedScaler,
                           const PlayerFigureDirector* pFigureDirector,
                           const IUsePlayerFlag* pPanelDashFlag,
                           const IUsePlayerFlag* pModifiedPanelDashFlag,
                           const IUsePlayerFlag* pFlingPoleDashFlag, const PlayerTrigger* pTrigger,
                           const IUsePlayerFlag* pHoldingFlag);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    bool isDashing() const override;
    bool isDashingFast() const override;
    bool isRunningOnGround() const override;
    bool isGreaterSuperDashMaxSpeed() const override;

    virtual f32 getNormalMaxSpeed() const;
    virtual f32 getDashMaxSpeed() const;
    virtual f32 getSuperDashSpeed() const;
    virtual f32 getInvincibleDashSpeed() const;
    virtual f32 getAnimRateEff() const;
    virtual s32 getAccelFrame() const;
    virtual f32 accelerate(f32 speed, f32 maxSpeed, f32 accel);

    virtual const char* getMoveAnimName() const { return "Move"; }

    virtual f32 calcAnimRate(ESlotIndex index) const;
    virtual void calcTilt(const sead::Vector3f& rPrevFront);
    virtual f32 getDownHillAccelAddRate();
    virtual f32 getClimbUprightBlendRate();
    virtual f32 getRoundLimitDegreeMin() const;
    virtual f32 getRoundLimitDegreeMax() const;

    bool isAnyDashFlagOn() const;
    f32 calcMaxSpeed() const;
    f32 getDashPanelSpeed() const;
    s32 getSuperDashTimer() const;
    s32 getDashPanelStartFrame() const;
    bool isSuperDashSuccess() const;
    void updateMoveState(f32 speed);
    void updateBlendWeight(f32 speed, bool isBrake);
    void updateAnimRate(f32 speed, bool isBrake);
    bool isDashSuccess(f32 speed) const;
    bool isPanelDashOn() const;
    bool isModifiedPanelDashOn() const;
    bool isFlingPoleDashOn() const;

    void setSuperDashKeepInfo(SuperDashKeepInfo* pInfo) { mSuperDashKeepInfo = pInfo; }

private:
    const PlayerConstParam* getConstParam() const { return mArg->mConstParam; }

    PlayerActionArg* mArg;                                  // 0x10
    const IUsePlayerDoubleMarioSpeed* mDoubleMarioSpeed;    // 0x18
    const IUsePlayerInvincibleDash* mInvincibleDash;        // 0x20
    const IUsePlayerMoveSpeedScaler* mMoveSpeedScaler;      // 0x28
    const IUsePlayerFlag* mPanelDashFlag;                   // 0x30
    const IUsePlayerFlag* mModifiedPanelDashFlag;           // 0x38
    const IUsePlayerFlag* mFlingPoleDashFlag;               // 0x40
    const IUsePlayerFlag* mHoldingFlag;                     // 0x48
    SuperDashKeepInfo* mSuperDashKeepInfo = nullptr;        // 0x50
    const PlayerTrigger* mTrigger;                          // 0x58
    bool mIsSuperDashInhibited = false;                     // 0x60
    f32 mBlendLevel = 0.0f;                                 // 0x64
    f32 mDashStartBlendRate = 0.0f;                         // 0x68
    u32 mDashStartFrame = 0;                                // 0x6c
    const PlayerFigureDirector* mFigureDirector;            // 0x70
    u32 mFastFrame = 0;                                     // 0x78
    u32 mSuperDashFrame = 0;                                // 0x7c
    sead::Vector3f mFloorNormal = {0.0f, 0.0f, 0.0f};       // 0x80
    EMoveState mMoveState = EMoveState::Walk;               // 0x8c
    u32 mSuperDashStartAnimFrame = 0;                       // 0x90
    PlayerUprightMtxCalc* mUprightMtxCalc = nullptr;        // 0x98
    bool mIsSpeedScaleEnabled = true;                       // 0xa0
    bool mIsForceDash = false;                              // 0xa1
    bool mIsDashStart = false;                              // 0xa2
};

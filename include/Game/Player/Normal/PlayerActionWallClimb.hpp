#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerActionEnd.hpp"
#include "Player/PlayerAction.hpp"

class IUsePlayerFlag;
class IUsePlayerSnapWallInfo;
class IUsePlayerWallClimbInfo;
class IUsePlayerWallClimbInput;
class PlayerTrigger;
class PlayerWallJumpDirection;
struct PlayerActionArg;

/// The player's wall climb action (cat suit): climbing up, down and sideways along a wall.
class PlayerActionWallClimb : public PlayerAction, public IUsePlayerActionEnd {
    SEAD_RTTI_OVERRIDE(PlayerActionWallClimb, PlayerAction)

public:
    /// What the player did on the wall this frame (also picks the animation).
    enum class EState : u32 {
        None = 0,
        Climb = 1,
        Keep = 2,
        LeftMove = 3,
        RightMove = 4,
    };

    PlayerActionWallClimb(const PlayerActionArg* pArg, const IUsePlayerSnapWallInfo* pSnapWallInfo,
                          PlayerWallJumpDirection* pWallJumpDirection,
                          const IUsePlayerWallClimbInfo* pWallClimbInfo,
                          const IUsePlayerWallClimbInput* pWallClimbInput, PlayerTrigger* pTrigger,
                          const IUsePlayerFlag* pFlingPoleFlag);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;
    bool isEnd() const override;

    void climbUp(f32 speedRate, const sead::Vector3f& rClimbDir);
    void climbDown(f32 speedRate, const sead::Vector3f& rClimbDir);
    void brake(const sead::Vector3f& rClimbDir);
    void controlSlide(const sead::Vector3f& rClimbDir);
    void controlAnim(const sead::Vector3f& rClimbDir);
    f32 getMaxClimbSpeed() const;
    void incrementClimbCount();
    s32 getClimbFrame() const;
    void calcSlideVec(sead::Vector3f* pOut, const sead::Vector3f& rVec,
                      const sead::Vector3f& rClimbDir) const;
    f32 getMaxSideSpeed() const;

private:
    const PlayerActionArg* mArg;                        // 0x10
    const IUsePlayerSnapWallInfo* mSnapWallInfo;        // 0x18
    PlayerWallJumpDirection* mWallJumpDirection;        // 0x20
    const IUsePlayerWallClimbInfo* mWallClimbInfo;      // 0x28
    const IUsePlayerWallClimbInput* mWallClimbInput;    // 0x30
    PlayerTrigger* mTrigger;                            // 0x38
    u32 mClimbCount = 0;                                // 0x40
    u32 mBrakeCount = 0;                                // 0x44
    bool mIsEnd = false;                                // 0x48
    bool _49 = false;                                   // 0x49, set on setup, never read here
    sead::Vector3f mStartFront;                         // 0x4c
    sead::Vector2f mStartStick;                         // 0x58
    EState mLastState = EState::None;                   // 0x60
    EState mState = EState::None;                       // 0x64
    sead::Vector3f mWallNormal;                         // 0x68
    const IUsePlayerFlag* mFlingPoleFlag;               // 0x78
};

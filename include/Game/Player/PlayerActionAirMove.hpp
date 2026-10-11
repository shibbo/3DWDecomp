#pragma once

#include <math/seadVector.h>

#include "Player/PlayerAction.hpp"
#include "Player/PlayerActionArg.hpp"

class IUsePlayerCheckArea;
class IUsePlayerHolded;
class IUsePlayerLongFallCheck;
class IUsePlayerRaccoonDogFallTask;
class PlayerConstParam;
class PlayerJumpExtension;
class PlayerTrigger;

/// The player systems an air move action gets handed.
struct PlayerActionAirMoveArg {
    PlayerActionArg* mActionArg;                        // 0x0
    IUsePlayerLongFallCheck* mLongFallCheck;            // 0x8
    IUsePlayerRaccoonDogFallTask* mRaccoonDogFallTask;  // 0x10
    bool mIsFlightDuration;                             // 0x18
    f32 mFlightDurationRotBlendRate;                    // 0x1c
};

/// The player's movement in the air (base of the jumps and falls).
class PlayerActionAirMove : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionAirMove, PlayerAction)

public:
    PlayerActionAirMove(PlayerActionAirMoveArg* pArg, PlayerTrigger* pTrigger,
                        const IUsePlayerCheckArea* pCheckArea, IUsePlayerHolded* pHolded,
                        bool isIgnoreHoldRelease);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    virtual void controlDirection();
    virtual void controlDirectionForFlightDuration();
    virtual f32 getGravity() const;
    virtual f32 getFallSpeedMax() const;
    virtual f32 getStickOffBrakeRate() const;
    virtual const char* getFallAnimAfterTailAttack() const;
    virtual void calcMoveVec(sead::Vector3f* pOut) const;
    virtual f32 getNormalMaxSpeed() const;
    virtual f32 getJumpCancelBrakeRate() const;
    virtual f32 getJumpCancelMinSpeed() const;
    virtual f32 getSideVelocityRate() const;

    void checkEndTurn();
    void controlHorizontalVelocity(const sead::Vector3f& rMoveDir, f32 inputRate,
                                   const sead::Vector3f& rHorizontalVelocity);
    void controlVerticalVelocity(f32* pSpeed, const sead::Vector3f& rUp);
    void createJumpExtension(u32 maxFrame);
    void resetJumpExtension();
    void replaceJumpKeep(const char* pAnimName);
    void replaceFallAfterJumpKeep(const char* pAnimName);
    bool isSubActionRunning() const;
    void startTurn(const sead::Vector3f& rFront, const sead::Vector3f& rUp);
    void validateSubAction();
    void invalidateSubAction();
    void updateJumpExtensionMaxFrame(u32 maxFrame);

protected:
    void calcFrontFromVelocity();
    const PlayerConstParam* getConstParam() const { return mArg->mConstParam; }

    PlayerActionArg* mArg;                                // 0x8
    PlayerJumpExtension* mJumpExtension = nullptr;        // 0x10
    bool mIsInhibitControl = false;                       // 0x18
    u32 mStickOnFrame = 0;                                // 0x1c
    bool mIsTurning = false;                              // 0x20
    u32 mInhibitControlFrame = 0;                         // 0x24
    sead::Vector3f mFront = {0.0f, 0.0f, 0.0f};           // 0x28
    sead::Vector3f mSide = {0.0f, 0.0f, 0.0f};            // 0x34
    bool mIsFixDirection = false;                         // 0x40
    unsigned char _41[0x50 - 0x41];
    bool mIsBrakeAtFallSpeedMax = true;                   // 0x50
    IUsePlayerLongFallCheck* mLongFallCheck;              // 0x58
    IUsePlayerRaccoonDogFallTask* mRaccoonDogFallTask;    // 0x60
    f32 mSpeedRate = 0.0f;                                // 0x68
    bool mIsInhibitSideAccel = false;                     // 0x6c
    f32 mSideBrakeRate = 1.0f;                            // 0x70
    u32 mFrontWallHitCount = 0;                           // 0x74
    bool mIsOnFrontWall = false;                          // 0x78
    bool mIsBrakeStickOff = false;                        // 0x79
    bool mIsInhibitBackward = false;                      // 0x7a
    bool mIsFlightDuration;                               // 0x7b
    bool mIsFlightDurationUsed = false;                   // 0x7c
    s32 mFlightDurationCount = 0;                         // 0x80
    bool mIsFlightDurationActive = false;                 // 0x84
    f32 mFlightDurationRotBlendRate;                      // 0x88
    const char* mJumpKeepAnim = "JumpKeep";               // 0x90
    const char* mFallAfterJumpKeepAnim = "Fall";          // 0x98
    PlayerTrigger* mTrigger;                              // 0xa0
    IUsePlayerHolded* mHolded;                            // 0xa8
    bool mIsReleasedFromHold = false;                     // 0xb0
    bool mIsIgnoreHoldRelease;                            // 0xb1
    const IUsePlayerCheckArea* mCheckArea;                // 0xb8
};

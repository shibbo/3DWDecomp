#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerActionCancelable.hpp"
#include "Player/IUsePlayerPropellerJumpPhase.hpp"
#include "Player/PlayerActionAirMove.hpp"

class IUsePlayerCheckArea;
class IUsePlayerFlag;
class IUsePlayerFlagControl;
class IUsePlayerPropellerInhibitor;
class PlayerTrigger;

/// The propeller box's flight: the drop, the engine brake, the rise and the glide down.
class PlayerActionPropellerJump : public PlayerActionAirMove,
                                  public IUsePlayerActionCancelable,
                                  public IUsePlayerPropellerJumpPhase {
    SEAD_RTTI_OVERRIDE(PlayerActionPropellerJump, PlayerActionAirMove)

public:
    /// The phase of the flight.
    enum class EState : u32 {
        Drop = 0,         ///< Falling before the engine starts.
        EngineBrake = 1,  ///< The engine brakes the fall.
        Rise = 2,         ///< The engine lifts the player.
        Glide = 3,        ///< Gliding down with the button held.
        Fall = 4,         ///< The button was released while braking.
        Hover = 5,        ///< Started again without engine power left.
    };

    PlayerActionPropellerJump(PlayerActionAirMoveArg* pArg, IUsePlayerFlagControl* pFlagControl,
                              IUsePlayerPropellerInhibitor* pPropellerInhibitor,
                              const IUsePlayerFlag* pGlideFlag, PlayerTrigger* pTrigger,
                              const IUsePlayerCheckArea* pCheckArea);

    void update() override;
    void setup() override;
    void teardown() override;
    bool isPossibleToCancel() const override;
    bool isPropellerJumping() const override;
    bool isPropellerJumpRising() const override;
    bool isPropellerJumpGlide() const override;
    void controlDirection() override;
    f32 getGravity() const override;
    f32 getFallSpeedMax() const override;
    f32 getStickOffBrakeRate() const override;

    void updateSound();
    bool updateEngine();
    f32 calcGravity() const;
    u32 getPropellerPowMaxFrame() const;

private:
    IUsePlayerFlagControl* mFlagControl;                   // 0xd0
    IUsePlayerPropellerInhibitor* mPropellerInhibitor;     // 0xd8
    const IUsePlayerFlag* mGlideFlag;                      // 0xe0
    u32 mPowFrame = 0;                                     // 0xe8
    EState mState = EState::EngineBrake;                   // 0xec
    u32 mSeFrame = 0;                                      // 0xf0
    f32 mGravity = 0.0f;                                   // 0xf4
    bool mIsHoldingButton = false;                         // 0xf8
    bool mIsPropellerJumping = false;                      // 0xf9
};

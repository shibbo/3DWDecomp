#pragma once

#include <basis/seadTypes.h>

#include "Player/PlayerAction.hpp"

struct PlayerActionArg;

/// The player's free swim action (the 3D underwater swim): the stick steers the swim direction
/// and the paddle button kicks the player forward.
class PlayerActionSwim : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionSwim, PlayerAction)

public:
    PlayerActionSwim(const PlayerActionArg* pArg);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    void turnVelocityToFront();

private:
    const PlayerActionArg* mArg;  // 0x8
    u32 mPaddleFrame = 0;         // 0x10, frames the current paddle still accelerates
};

#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's bind jump action.
class PlayerActionBindJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionBindJump, PlayerActionAirMove)

    bool isJumpAction() const;
};

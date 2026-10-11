#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's wall jump action.
class PlayerActionWallJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionWallJump, PlayerActionAirMove)
};

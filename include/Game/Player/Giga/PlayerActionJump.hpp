#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's jump action.
class PlayerActionJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionJump, PlayerActionAirMove)
};

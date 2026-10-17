#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's punched jump action.
class PlayerActionPunchedJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionPunchedJump, PlayerActionAirMove)
};

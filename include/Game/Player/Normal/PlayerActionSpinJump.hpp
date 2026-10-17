#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's spin jump action.
class PlayerActionSpinJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionSpinJump, PlayerActionAirMove)
};

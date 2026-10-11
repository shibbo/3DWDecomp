#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's swim jump action.
class PlayerActionSwimJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionSwimJump, PlayerActionAirMove)
};

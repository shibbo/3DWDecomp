#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's trample jump action.
class PlayerActionTrampleJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionTrampleJump, PlayerActionAirMove)
};

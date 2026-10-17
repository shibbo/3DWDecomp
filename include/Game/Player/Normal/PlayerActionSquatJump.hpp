#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's squat jump action.
class PlayerActionSquatJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionSquatJump, PlayerActionAirMove)
};

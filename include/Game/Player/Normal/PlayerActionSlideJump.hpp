#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's slide jump action.
class PlayerActionSlideJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionSlideJump, PlayerActionAirMove)
};

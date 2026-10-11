#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's slide fall action.
class PlayerActionSlideFall : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionSlideFall, PlayerActionAirMove)
};

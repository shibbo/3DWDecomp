#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's fall action.
class PlayerActionFall : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionFall, PlayerActionAirMove)
};

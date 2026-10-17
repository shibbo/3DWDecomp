#pragma once

#include "Player/PlayerAction.hpp"

/// The player's brake action.
class PlayerActionBrake : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionBrake, PlayerAction)
};

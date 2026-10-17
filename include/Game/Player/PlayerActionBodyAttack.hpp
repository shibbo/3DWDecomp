#pragma once

#include "Player/PlayerAction.hpp"

/// The player's body attack action.
class PlayerActionBodyAttack : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionBodyAttack, PlayerAction)
};

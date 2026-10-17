#pragma once

#include "Player/PlayerAction.hpp"

/// The player's turn action.
class PlayerActionTurn : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionTurn, PlayerAction)
};

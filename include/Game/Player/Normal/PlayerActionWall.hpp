#pragma once

#include "Player/PlayerAction.hpp"

/// The player's wall action.
class PlayerActionWall : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionWall, PlayerAction)
};

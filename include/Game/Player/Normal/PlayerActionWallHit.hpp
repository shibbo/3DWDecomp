#pragma once

#include "Player/PlayerAction.hpp"

/// The player's wall hit action.
class PlayerActionWallHit : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionWallHit, PlayerAction)
};

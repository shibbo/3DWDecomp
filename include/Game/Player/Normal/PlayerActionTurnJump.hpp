#pragma once

#include "Player/PlayerAction.hpp"

/// The player's turn jump action.
class PlayerActionTurnJump : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionTurnJump, PlayerAction)
};

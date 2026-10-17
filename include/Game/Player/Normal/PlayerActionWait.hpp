#pragma once

#include "Player/PlayerAction.hpp"

/// The player's wait action.
class PlayerActionWait : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionWait, PlayerAction)
};

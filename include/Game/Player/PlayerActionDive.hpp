#pragma once

#include "Player/PlayerAction.hpp"

/// The player's dive action.
class PlayerActionDive : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionDive, PlayerAction)
};

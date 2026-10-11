#pragma once

#include "Player/PlayerAction.hpp"

/// The player's normal die action.
class PlayerActionNormalDie : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionNormalDie, PlayerAction)
};

#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's spin attack jump action.
class PlayerActionSpinAttackJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionSpinAttackJump, PlayerActionAirMove)
};

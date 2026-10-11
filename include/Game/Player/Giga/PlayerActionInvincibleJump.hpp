#pragma once

#include "Player/PlayerActionAirMove.hpp"

/// The player's invincible jump action.
class PlayerActionInvincibleJump : public PlayerActionAirMove {
    SEAD_RTTI_OVERRIDE(PlayerActionInvincibleJump, PlayerActionAirMove)
};

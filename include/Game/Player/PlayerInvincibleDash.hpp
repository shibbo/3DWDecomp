#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerInvincibleDash.hpp"

class IUsePlayerInput;
class IUsePlayerInvincibleCheck;
class PlayerActionGraph;
class PlayerConstParam;
struct PlayerProperty;

/// The faster dash while the player is invincible.
class PlayerInvincibleDash : public IUsePlayerInvincibleDash {
public:
    PlayerInvincibleDash();
    void init(const PlayerActionGraph* pActionGraph, const IUsePlayerInvincibleCheck* pInvincible,
              const IUsePlayerInput* pInput, const PlayerProperty* pProperty,
              const PlayerConstParam* pConstParam);
    void update();

    f32 getRate() const override;
    bool isPossibleToPlayDashAnim() const override;

private:
    u8 _8[0x38 - 0x8];
};
static_assert(sizeof(PlayerInvincibleDash) == 0x38);

#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerLongFallCheck.hpp"

class IUsePlayerCollision;
class PlayerActionGraph;
class PlayerConstParam;
struct PlayerProperty;

/// Detects that the player has been falling for long.
class PlayerLongFallCheck : public IUsePlayerLongFallCheck {
public:
    PlayerLongFallCheck();
    void init(const PlayerProperty* pProperty, const PlayerActionGraph* pActionGraph,
              const IUsePlayerCollision* pCollision, const PlayerConstParam* pConstParam);
    void update();

    bool isLongFalling() const override;
    void resetLongFall() override;

private:
    u8 _8[0x38 - 0x8];
};
static_assert(sizeof(PlayerLongFallCheck) == 0x38);

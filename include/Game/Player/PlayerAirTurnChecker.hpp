#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerAirTurnCheck.hpp"
#include "Player/IUsePlayerAirTurnObserver.hpp"

/// Remembers whether the player is turning around in mid-air.
class PlayerAirTurnChecker : public IUsePlayerAirTurnCheck, public IUsePlayerAirTurnObserver {
public:
    PlayerAirTurnChecker();

    bool isAirTurning() const override;
    void startAirTurn() override;
    void endAirTurn() override;

private:
    u8 _10[0x18 - 0x10];
};
static_assert(sizeof(PlayerAirTurnChecker) == 0x18);

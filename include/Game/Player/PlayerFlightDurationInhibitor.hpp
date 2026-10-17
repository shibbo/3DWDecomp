#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerActionInhibitor.hpp"
#include "Player/IUsePlayerLandingObserver.hpp"

class IUsePlayerEquipment;

/// Shortens the flight of the player's equipment (e.g. after a punch).
class PlayerFlightDurationInhibitor : public IUsePlayerActionInhibitor,
                                      public IUsePlayerLandingObserver {
public:
    PlayerFlightDurationInhibitor(const IUsePlayerEquipment* pEquipment);
    void update();
    void inhibitForAShortTime(u32 frame);

    void inhibit() override;
    bool isInhibit() const override;
    void notifyLanding() override;

private:
    u8 _10[0x20 - 0x10];
};
static_assert(sizeof(PlayerFlightDurationInhibitor) == 0x20);

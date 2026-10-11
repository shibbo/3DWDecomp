#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerLandingObserver.hpp"
#include "Player/IUsePlayerPropellerInhibitor.hpp"

class PlayerActionGraph;

/// Blocks the propeller box's flight until the player lands.
class PlayerPropellerInhibitor : public IUsePlayerPropellerInhibitor,
                                 public IUsePlayerLandingObserver {
public:
    PlayerPropellerInhibitor();

    void inhibitPropeller() override;
    bool isInhibit() const override;
    void notifyLanding() override;

    void setActionGraph(const PlayerActionGraph* pActionGraph) { mActionGraph = pActionGraph; }

private:
    const PlayerActionGraph* mActionGraph;  // 0x10
    u8 _18[0x20 - 0x18];
};
static_assert(sizeof(PlayerPropellerInhibitor) == 0x20);

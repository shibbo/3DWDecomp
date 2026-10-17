#pragma once

/// Gets told when the player lands (registered with a PlayerLandingInformer).
class IUsePlayerLandingObserver {
public:
    virtual void notifyLanding() = 0;
};

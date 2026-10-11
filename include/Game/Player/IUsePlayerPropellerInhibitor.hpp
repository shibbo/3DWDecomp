#pragma once

/// Blocks the propeller box's flight until the player lands (implemented by
/// PlayerPropellerInhibitor).
class IUsePlayerPropellerInhibitor {
public:
    virtual void inhibitPropeller() = 0;
    virtual bool isInhibit() const = 0;
};

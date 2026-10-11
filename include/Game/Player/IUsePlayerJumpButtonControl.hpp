#pragma once

/// Turns reading of the player's jump button off and on (implemented by PlayerInput).
class IUsePlayerJumpButtonControl {
public:
    virtual void disableJumpButton() = 0;
    virtual void enableJumpButton() = 0;
};

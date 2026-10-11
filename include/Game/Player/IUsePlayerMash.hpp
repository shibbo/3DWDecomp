#pragma once

/// Toggles the player's mash (squash) joint control (implemented by PlayerModelHolder).
class IUsePlayerMash {
public:
    virtual void validateMash() = 0;
    virtual void invalidateMash() = 0;
    virtual bool isMash() const = 0;
};

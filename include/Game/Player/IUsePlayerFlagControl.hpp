#pragma once

/// Switches a player state flag on and off (handed to the player by its owner).
class IUsePlayerFlagControl {
public:
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
};

#pragma once

/// A player flag that can be switched on and off (e.g. a PlayerSimpleFlag).
class IUsePlayerFlagSwitch {
public:
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
};

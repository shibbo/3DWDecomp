#pragma once

#include "Player/IUsePlayerFlag.hpp"
#include "Player/IUsePlayerFlagSwitch.hpp"

/// A player flag that is simply switched on and off.
class PlayerSimpleFlag : public IUsePlayerFlag, public IUsePlayerFlagSwitch {
public:
    /** @brief Tests whether the flag is on. @return True if on. */
    bool isOn() const override { return mIsOn; }

    /** @brief Switches the flag on. */
    void turnOn() override { mIsOn = true; }

    /** @brief Switches the flag off. */
    void turnOff() override { mIsOn = false; }

private:
    bool mIsOn = false;  // 0x10
};

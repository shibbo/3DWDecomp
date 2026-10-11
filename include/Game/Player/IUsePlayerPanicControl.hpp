#pragma once

/**
 * @brief Interface deciding whether the player's panic behaviour (running around in fear) is on.
 */
class IUsePlayerPanicControl {
public:
    virtual bool isPanic() = 0;
};

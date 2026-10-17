#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Draws the rain splatters on the screen during disaster mode.
 * @note Only the members used by reconstructed code are declared.
 */
class SplatterPlotter : public al::LiveActor {
public:
    explicit SplatterPlotter(const char* pName);

    void run(bool isRun);
    void update();
    void disable(bool isDisable);

private:
    u8 mUnreconstructed[0xe4];
};

static_assert(sizeof(SplatterPlotter) == 0x228);

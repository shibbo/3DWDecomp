#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class PlayerAliveWatcher;

/**
 * @brief Player-count (lives) counter parts layout.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CounterPlayerParts : public al::LayoutActor {
public:
    CounterPlayerParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
                       al::LayoutActor* pParent, const PlayerAliveWatcher* pWatcher,
                       bool isUnknown);

    void startDemo();
    void endDemo();

    /** @brief Show the counter in its Toad Brigade (Captain Toad stage) style. */
    void setKinopioBrigade() { mIsKinopioBrigade = true; }

private:
    unsigned char _padding[0x144 - 0x121];  // members start in al::LayoutActor's tail padding
    bool mIsKinopioBrigade;  // 0x144
    unsigned char _145[0x148 - 0x145];
};

static_assert(sizeof(CounterPlayerParts) == 0x148);

#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class GreenStarKeeper;

/**
 * @brief Counter of the green stars collected in the current stage.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CounterGreenStarParts : public al::LayoutActor {
public:
    CounterGreenStarParts(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPaneName, al::LayoutActor* pParent,
                          const GreenStarKeeper* pGreenStarKeeper);

private:
    u8 _121[0x180 - 0x121];  // members start in al::LayoutActor's tail padding
};

static_assert(sizeof(CounterGreenStarParts) == 0x180);

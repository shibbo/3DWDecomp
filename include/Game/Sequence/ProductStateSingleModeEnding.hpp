#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ProductSequence;
class StageWipeKeeper;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
}  // namespace al

/**
 * @brief Sequence state of the Bowser's Fury ending.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateSingleModeEnding : public al::HostStateBase<ProductSequence> {
public:
    ProductStateSingleModeEnding(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                                 const al::LayoutInitInfo& rLayoutInfo,
                                 StageWipeKeeper* pWipeKeeper,
                                 al::ScreenCaptureExecutor* pScreenCaptureExecutor);

private:
    u8 _20[0x30];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateSingleModeEnding) == 0x50);

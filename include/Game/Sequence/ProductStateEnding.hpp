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
 * @brief Sequence state of the Super Mario 3D World ending.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateEnding : public al::HostStateBase<ProductSequence> {
public:
    ProductStateEnding(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                       const al::LayoutInitInfo& rLayoutInfo, StageWipeKeeper* pWipeKeeper,
                       al::ScreenCaptureExecutor* pScreenCaptureExecutor);

    bool isEndFirstTime();

private:
    u8 _20[0x28];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateEnding) == 0x48);

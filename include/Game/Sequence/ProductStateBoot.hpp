#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ProductAsyncResourceLoader;
class ProductSequence;
class StageWipeKeeper;

namespace al {
class ScreenCaptureExecutor;
struct SequenceInitInfo;
}  // namespace al

/**
 * @brief Sequence state of the boot (loading the stationed resources).
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateBoot : public al::HostStateBase<ProductSequence> {
public:
    ProductStateBoot(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                     StageWipeKeeper* pWipeKeeper, ProductAsyncResourceLoader* pResourceLoader,
                     al::ScreenCaptureExecutor* pScreenCaptureExecutor);

private:
    u8 _20[0x28];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateBoot) == 0x48);

#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ProductSequence;
class ProductStageStartParam;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
}  // namespace al

/**
 * @brief Sequence state of the event shown after the first Super Mario 3D World ending.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateAfterEndingEvent : public al::HostStateBase<ProductSequence> {
public:
    ProductStateAfterEndingEvent(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                                 StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                                 const al::LayoutInitInfo& rLayoutInfo,
                                 al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                                 WipeCurtainResult* pWipeCurtainResult);

    bool isEffectDraw();

private:
    u8 _20[0x38];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateAfterEndingEvent) == 0x58);

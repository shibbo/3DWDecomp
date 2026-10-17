#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
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
 * @brief Sequence state of the Super Mario 3D World course select.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateCourseSelect : public al::HostStateBase<ProductSequence> {
public:
    ProductStateCourseSelect(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                             StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                             const al::LayoutInitInfo& rLayoutInfo,
                             ControllerConnectChecker* pConnectChecker,
                             al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                             WipeCurtainResult* pWipeCurtainResult);

    bool isGotoTitle() const;
    bool isLoadGame() const;
    bool isEffectDraw() const;

private:
    u8 _20[0x48];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateCourseSelect) == 0x68);

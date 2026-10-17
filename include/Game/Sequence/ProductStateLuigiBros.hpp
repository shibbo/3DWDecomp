#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
class GameDataHolder;
class ProductSequence;
class ProductStageStartParam;
class StageWipeKeeper;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
}  // namespace al

/**
 * @brief Sequence state of Luigi Bros.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateLuigiBros : public al::HostStateBase<ProductSequence> {
public:
    ProductStateLuigiBros(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                          const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
                          GameDataHolder* pHolder, StageWipeKeeper* pWipeKeeper,
                          al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                          ControllerConnectChecker* pConnectChecker);

private:
    u8 _20[0x30];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateLuigiBros) == 0x50);

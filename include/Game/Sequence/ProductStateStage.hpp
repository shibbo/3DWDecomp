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
 * @brief Sequence state of a Super Mario 3D World stage.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateStage : public al::HostStateBase<ProductSequence> {
public:
    ProductStateStage(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                      StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                      const al::LayoutInitInfo& rLayoutInfo, GameDataHolder* pHolder,
                      ControllerConnectChecker* pConnectChecker,
                      al::ScreenCaptureExecutor* pScreenCaptureExecutor);

    bool isClear() const;
    bool isClearWithResult() const;
    bool isClearWorldWarp() const;
    bool isGameOver() const;
    bool isGameEnd() const;
    bool isRetire() const;
    bool isLoadGame() const;

private:
    u8 _20[0x50];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateStage) == 0x70);

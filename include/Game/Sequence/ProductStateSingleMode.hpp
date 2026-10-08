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
 * @brief Sequence state of Bowser's Fury.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateSingleMode : public al::HostStateBase<ProductSequence> {
public:
    ProductStateSingleMode(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                           StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                           const al::LayoutInitInfo& rLayoutInfo, GameDataHolder* pHolder,
                           ControllerConnectChecker* pConnectChecker,
                           al::ScreenCaptureExecutor* pScreenCaptureExecutor);

    bool isGameChange() const;
    bool isGameEnd() const;
    bool isLoadGame() const;
    
    /**
     * @brief Mark that the game starts right after the opening demo.
     * @param isAfter True when coming from the opening demo.
     */
    void setAfterOpeningDemo(bool isAfter) { mIsAfterOpeningDemo = isAfter; }

private:
    u8 _20[0x38];  ///< Not reconstructed yet.
    bool mIsAfterOpeningDemo;  // 0x58
    u8 _59[0x17];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateSingleMode) == 0x70);

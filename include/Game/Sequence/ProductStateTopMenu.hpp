#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
class ProductAsyncResourceLoader;
class ProductSequence;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
}  // namespace al

/// What the top menu is entered from, which decides how it opens.
enum class ProductTopMenuEntry : s32 {
    Default = 0,             ///< Boot, title or a game exit.
    SingleMode = 1,          ///< Leaving Bowser's Fury.
    SingleModeEnding = 2,    ///< After the Bowser's Fury ending.
    StageClear = 3,          ///< After clearing a stage.
};

/**
 * @brief Sequence state showing the top menu (game selection).
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateTopMenu : public al::HostStateBase<ProductSequence> {
public:
    ProductStateTopMenu(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                        const al::LayoutInitInfo& rLayoutInfo, StageWipeKeeper* pWipeKeeper,
                        ProductAsyncResourceLoader* pResourceLoader,
                        al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                        ControllerConnectChecker* pConnectChecker,
                        WipeCurtainResult* pWipeCurtainResult);

    bool isEffectDraw() const;
    
    /**
     * @brief Set what the top menu is entered from.
     * @param entry The entry kind.
     */
    void setEntry(ProductTopMenuEntry entry) { mEntry = entry; }

private:
    u8 _20[0x2c];  ///< Not reconstructed yet.
    ProductTopMenuEntry mEntry;  // 0x4c
    u8 _50[0x10];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateTopMenu) == 0x60);

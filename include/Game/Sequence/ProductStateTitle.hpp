#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ProductAsyncResourceLoader;
class ProductSequence;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
}  // namespace al

/**
 * @brief Sequence state of the Super Mario 3D World title screen.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateTitle : public al::HostStateBase<ProductSequence> {
public:
    ProductStateTitle(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                      const al::LayoutInitInfo& rLayoutInfo, StageWipeKeeper* pWipeKeeper,
                      ProductAsyncResourceLoader* pResourceLoader,
                      al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                      WipeCurtainResult* pWipeCurtainResult);

    void resetState();
    
    /**
     * @brief Check whether Luigi Bros. was chosen on the title screen.
     * @return True to start Luigi Bros.
     */
    bool isGotoLuigiBros() const { return mIsGotoLuigiBros; }
    
    /**
     * @brief Check whether the title screen goes back to the top menu.
     * @return True to go back to the top menu.
     */
    bool isGotoTopMenu() const { return mIsGotoTopMenu; }
    
    /**
     * @brief Request loading the save data when the title opens.
     * @param isLoad True to load the save data.
     */
    void setLoadGame(bool isLoad) { mIsLoadGame = isLoad; }

private:
    u8 _20[0x30];  ///< Not reconstructed yet.
    bool mIsGotoLuigiBros;  // 0x50
    u8 _51[0x20];  ///< Not reconstructed yet.
    bool mIsGotoTopMenu;  // 0x71
    bool mIsLoadGame;     // 0x72
    u8 _73[0x5];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateTitle) == 0x78);

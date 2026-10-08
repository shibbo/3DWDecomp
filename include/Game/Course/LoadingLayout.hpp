#pragma once

#include <basis/seadTypes.h>

class GameDataHolder;

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Loading screen shown while the next scene is created.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class LoadingLayout {
public:
    LoadingLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pHolder);

    void startDisplay(bool isFadeIn, bool isShowTips);
    void requestEnd();

    /**
     * @brief Check whether the loading screen is displayed.
     * @return True while displayed.
     */
    bool isDisplay() const { return mIsDisplay; }

private:
    u8 _0[0x120];  ///< Not reconstructed yet.
    bool mIsDisplay;  // 0x120
    u8 _121[0x17];  ///< Not reconstructed yet.
};

static_assert(sizeof(LoadingLayout) == 0x138);

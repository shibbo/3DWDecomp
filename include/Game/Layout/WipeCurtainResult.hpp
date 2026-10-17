#pragma once

#include <basis/seadTypes.h>

class GameDataHolder;

namespace al {
class ErrorViewer;
class HomeButton;
class LayoutInitInfo;
class NetworkSystem;
}  // namespace al

/**
 * @brief Curtain wipe showing the result of a stage.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class WipeCurtainResult {
public:
    WipeCurtainResult(const al::LayoutInitInfo& rInfo, GameDataHolder* pHolder,
                      al::NetworkSystem* pNetworkSystem, al::ErrorViewer* pErrorViewer,
                      al::HomeButton* pHomeButton);

private:
    u8 _0[0x168];  ///< Not reconstructed yet.
};

static_assert(sizeof(WipeCurtainResult) == 0x168);

#pragma once

#include "Library/Scene/ISceneObj.hpp"

namespace al {
class GraphicsSystemInfo;
}  // namespace al

/**
 * @brief Scene object counting the fire balls the players have thrown.
 * @note Only the members used by already-decompiled callers are declared.
 */
class PlayerFireBallAppearWatcher : public al::ISceneObj {
public:
    PlayerFireBallAppearWatcher();

    /**
     * @brief Sets the graphics system info used to check the bound effects.
     * @param pInfo The graphics system info.
     */
    void setGraphicsSystemInfo(al::GraphicsSystemInfo* pInfo) { mGraphicsSystemInfo = pInfo; }

private:
    u8 _8[0x10 - 0x8];
    al::GraphicsSystemInfo* mGraphicsSystemInfo;  // 0x10
};

static_assert(sizeof(PlayerFireBallAppearWatcher) == 0x18);

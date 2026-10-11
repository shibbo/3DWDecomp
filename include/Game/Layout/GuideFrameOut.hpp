#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
}  // namespace al

/**
 * @brief Multiplayer HUD marker pointing at a player that has left the screen.
 * @note Only what reconstructed code needs is declared so far.
 */
class GuideFrameOut : public al::LayoutActor {
public:
    GuideFrameOut(const al::LayoutInitInfo& rInfo, const al::LiveActor* pTarget);

    void startDemo();
    void endDemo();
    void startPause();
    void endPause();
    void setEnable();
    void setEnableBubble(bool isEnable);
    s32 getScreenOutFrame() const;

    /**
     * @brief Get how many frames the target's icon has been out of the screen.
     * @return The icon screen-out frame count.
     */
    s32 getIconOutFrame() const { return mIconOutFrame; }

private:
    u8 _121[0x138 - 0x121];
    s32 mIconOutFrame;  // 0x138
};

static_assert(sizeof(GuideFrameOut) == 0x140);

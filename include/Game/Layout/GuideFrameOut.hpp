#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
} // namespace al

/**
 * @brief Multiplayer HUD marker pointing at a player that has left the screen.
 */
class GuideFrameOut : public al::LayoutActor {
public:
  GuideFrameOut(const al::LayoutInitInfo &rInfo, const al::LiveActor *pTarget);

  void initIcon();
  void appear() override;
  void control() override;
  void updateVisibility(bool isResume);
  void setDisable();
  bool isEnable() const;
  bool isScreenOut() const;
  void exeStartAppear();
  void exeAppear();
  void exeStartDisappear();
  void exeDisappear();
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
  const al::LiveActor *mTarget;
  s32 mCharacterType = -1;
  s32 mScreenOutFrame = 0;
  s32 mIconOutFrame = 0;
  bool mIsEnabled = false;
  bool mIsBubbleEnabled = true;
  bool mIsDemo = false;
  bool mWasAlive = false;
};

static_assert(sizeof(GuideFrameOut) == 0x140);

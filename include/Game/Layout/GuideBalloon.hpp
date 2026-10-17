#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>

namespace al {
class LayoutInitInfo;
class DemoDirector;
class SceneCameraInfo;
} // namespace al

namespace neko {
enum ColorType : s32;
} // namespace neko

/** @brief World-anchored guidance balloon with text, item and controller icons.
 */
class GuideBalloon : public al::LayoutActor {
public:
  /** @brief Creates a balloon following the supplied world position. */
  GuideBalloon(const char *pName, const al::LayoutInitInfo &rInfo,
               const sead::Vector3f *pTrans, const sead::Vector3f &rOffset,
               bool isFollow, al::DemoDirector *pDemoDirector);
  /** @brief Creates a balloon following the supplied world position. */
  GuideBalloon(const char *pName, const char *pLayoutName,
               const al::LayoutInitInfo &rInfo, const sead::Vector3f *pTrans,
               const sead::Vector3f &rOffset, bool isFollow,
               al::DemoDirector *pDemoDirector);

  /** @brief Shows the number of goal items unless a demo suppresses the
   * balloon. */
  void startShowGoalItem(int count);
  /** @brief Shows the touch or assist-controller prompt. */
  void startShowDrcTouch(bool);
  /** @brief Shows the new-item icon. */
  void startShowNew();
  /** @brief Shows the found and total cat counts in a thought balloon. */
  void startShowNeko(neko::ColorType colorType, int foundNum, int maxNum);
  /** @brief Shows a custom text balloon. */
  void startShow(const char16_t *pMessage);
  /** @brief Starts the matching speech or thought balloon exit. */
  void endShow();

  /**
   * @brief Sets the position the balloon follows.
   * @param pTrans Followed position; must outlive the balloon.
   */
  void setTrans(const sead::Vector3f *pTrans) { mTrans = pTrans; }

  /** @brief Unused microphone prompt entry point. */
  void startShowMike();
  /** @brief Checks whether an active demo suppresses this balloon. */
  bool isDemoOn();
  /** @brief Refreshes the prompt for the current assist controller. */
  void updateDRCIcon(bool isForce);
  /** @brief Tracks the owner while the balloon appears. */
  void exeAppear();
  /** @brief Tracks the owner and refreshes controller icons. */
  void exeWait();
  /** @brief Tracks the owner until the exit animation ends. */
  void exeEnd();
  /** @brief Reverses the balloon beak direction. */
  void setUpsideDown();
  /** @brief Selects the standard entrance state. */
  void setAppear();

  al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }

  const sead::Vector3f& getOffset() const { return mOffset; }

private:
  /** @brief Projects the followed position and offset into layout coordinates.
   */
  void updatePosition();
  al::SceneCameraInfo *mSceneCameraInfo;
  const sead::Vector3f *mTrans;
  sead::Vector3f mOffset;
  al::DemoDirector *mDemoDirector;
  bool mIsDemoSensitive;
  bool mIsUpsideDown;
  s32 mDRCIconType = 0;
};

static_assert(sizeof(GuideBalloon) == 0x158);

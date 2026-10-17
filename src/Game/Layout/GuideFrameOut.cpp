#include "Layout/GuideFrameOut.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/PlayerUtil.hpp"
class FrameOutCtrlArea : public al::AreaObj {
public:
  bool mCheckHorizontal;
  bool mCheckVertical;
  bool mShowIcon;
};
namespace {
NERVE_DECL(GuideFrameOut, Disappear);
NERVE_DECL(GuideFrameOut, StartDisappear);
NERVE_DECL(GuideFrameOut, StartAppear);
NERVE_DECL(GuideFrameOut, Appear);
NERVES_MAKE_NOSTRUCT(GuideFrameOut, Disappear, StartDisappear, StartAppear,
                     Appear)
} // namespace
/** @brief Creates the off-screen guide for a target player. */
GuideFrameOut::GuideFrameOut(const al::LayoutInitInfo &rInfo,
                             const al::LiveActor *pTarget)
    : al::LayoutActor("画面外ガイド"), mTarget(pTarget) {
  al::initLayoutActor(this, rInfo, "GuideFrameOut", nullptr);
  initNerve(&NrvGuideFrameOutDisappear, 0);
  initIcon();
  kill();
}
/** @brief Initializes the character animation and picture font. */
void GuideFrameOut::initIcon() {
  if (GameDataFunction::isCurrentStageKinopioBrigade(
          GameDataHolderAccessor(this)) ||
      mCharacterType >= 0)
    return;
  mCharacterType = rc::getPlayerCharaType(mTarget);
  const char *pCharacter = rc::getPlayerCharacterNameTrue(mCharacterType);
  al::startAction(this, pCharacter, "Character");
  al::setPaneString(
      this, "TxtFrameIcon",
      sead::WFormatFixedSafeString<6>(
          u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
          .cstr(),
      0, -1);
}
/** @brief Activates the guide with its pane initially hidden. */
void GuideFrameOut::appear() {
  al::LayoutActor::appear();
  al::hidePaneRootNoRecursive(this);
  al::setNerve(this, &NrvGuideFrameOutDisappear);
}
/** @brief Suppresses the guide during a demo. */
void GuideFrameOut::startDemo() {
  mIsDemo = true;
  updateVisibility(false);
}
/** @brief Applies enable flags, optionally preserving the resumed animation. */
void GuideFrameOut::updateVisibility(bool isResume) {
  if (isEnable()) {
    if (isResume) {
      al::LayoutActor::appear();
      const char *pCharacter = rc::getPlayerCharacterNameTrue(mCharacterType);
      al::startFreezeActionEnd(this, pCharacter, "Character");
    } else if (!isAlive())
      appear();
  } else if (isAlive()) {
    mScreenOutFrame = 0;
    mIconOutFrame = 0;
    kill();
  }
}
/** @brief Restores visibility after a demo. */
void GuideFrameOut::endDemo() {
  mIsDemo = false;
  updateVisibility(false);
}
/** @brief Saves visibility and suppresses the guide while paused. */
void GuideFrameOut::startPause() {
  mIsEnabled = false;
  mWasAlive = isAlive();
  updateVisibility(false);
}
/** @brief Restores the visibility recorded before pausing. */
void GuideFrameOut::endPause() {
  mIsEnabled = true;
  updateVisibility(mWasAlive);
}
/** @brief Enables the guide and resets the out-of-screen counters. */
void GuideFrameOut::setEnable() {
  mScreenOutFrame = 0;
  mIconOutFrame = 0;
  mIsEnabled = true;
  updateVisibility(false);
}
/** @brief Disables the guide. */
void GuideFrameOut::setDisable() {
  mIsEnabled = false;
  updateVisibility(false);
}
/** @brief Updates whether the player bubble permits the guide. */
void GuideFrameOut::setEnableBubble(bool isEnable) {
  if (mIsBubbleEnabled == isEnable)
    return;
  mIsBubbleEnabled = isEnable;
  updateVisibility(false);
}
/** @brief Returns whether the guide is enabled outside a demo. */
bool GuideFrameOut::isEnable() const {
  return mIsEnabled && !mIsDemo && mIsBubbleEnabled;
}
/** @brief Returns whether the target is currently outside the screen. */
bool GuideFrameOut::isScreenOut() const { return mScreenOutFrame > 0; }
/** @brief Returns the consecutive out-of-screen frame count. */
s32 GuideFrameOut::getScreenOutFrame() const { return mScreenOutFrame; }
/** @brief Tracks screen boundaries and points the guide toward the target. */
void GuideFrameOut::control() {
  if (!mTarget) {
    al::setNerve(this, &NrvGuideFrameOutStartDisappear);
    return;
  }
  bool checkHorizontal = true, checkVertical = true, forceIcon = false;
  auto *pArea = static_cast<const FrameOutCtrlArea *>(
      al::tryFindAreaObj(mTarget, "FrameOutCtrlArea", al::getTrans(mTarget)));
  if (pArea) {
    checkHorizontal = pArea->mCheckHorizontal;
    checkVertical = pArea->mCheckVertical;
    forceIcon = pArea->mShowIcon;
  }
  sead::Vector3f worldPos(al::getTrans(mTarget));
  worldPos.y += 70.0f;
  sead::Vector3f pos;
  al::calcLayoutPosFromWorldPosWithClampOutRange(&pos, mTarget, worldPos, 50.0f,
                                                 0);
  float screenRadius =
      al::calcLayoutRadiusFromWorldRadius(worldPos, mTarget, 200.0f);
  float iconRadius = sead::Mathf::abs(
      al::calcLayoutRadiusFromWorldRadius(worldPos, mTarget, 50.0f));
  float halfWidth = al::getLayoutDisplayWidth() * 0.5f;
  float halfHeight = al::getLayoutDisplayHeight() * 0.5f;
  bool outHorizontal =
      pos.x + 200.0f < -halfWidth || halfWidth < pos.x - 200.0f;
  bool outVertical =
      pos.y + 200.0f < -halfHeight || halfHeight < pos.y - 200.0f;
  bool behind = pos.z - sead::Mathf::abs(screenRadius) > 50.0f;
  if ((outHorizontal && checkHorizontal) || (outVertical && checkVertical) ||
      (behind && (checkHorizontal || checkVertical)))
    ++mScreenOutFrame;
  else
    mScreenOutFrame = 0;
  bool iconHorizontal =
      pos.x + iconRadius < -halfWidth || halfWidth < pos.x - iconRadius;
  bool iconVertical =
      pos.y + iconRadius < -halfHeight || halfHeight < pos.y - iconRadius;
  bool horizontalIcon = iconHorizontal && (checkHorizontal || forceIcon);
  bool verticalIcon = iconVertical && (checkVertical || forceIcon);
  bool showIcon = false;
  if (horizontalIcon || verticalIcon) {
    ++mIconOutFrame;
    showIcon = true;
  } else
    mIconOutFrame = 0;
  if (al::isDead(mTarget) || al::getAlivePlayerNum(mTarget) <= 1) {
    showIcon = false;
    mScreenOutFrame = 0;
    mIconOutFrame = 0;
  }
  if (pos.z > 0.0f) {
    pos.x = -pos.x;
    pos.y = -pos.y;
  }
  const sead::Vector2f original(pos.x, pos.y);
  sead::Vector2f clamped(original);
  sead::Vector2f limit(al::getLayoutDisplayWidth() / 2 - 100,
                       al::getLayoutDisplayHeight() / 2 - 100);
  al::clampV2f(&clamped, sead::Vector2f(-limit.x, -limit.y), limit);
  sead::Vector2f direction = original - clamped;
  if (al::isNearZero(direction, 0.001f))
    direction.set(0, 1);
  else
    al::normalize(&direction);
  const sead::Vector2f up(0, 1);
  float angle = sead::Mathf::rad2deg(
      sead::Mathf::acos(sead::Mathf::clamp(direction.dot(up), -1.0f, 1.0f)));
  sead::Vector3f rotate(0, 0, angle);
  if (up.x * direction.y - up.y * direction.x < 0.0f)
    rotate.z = -rotate.z;
  al::setLocalTrans(this, clamped);
  al::setPaneLocalRotate(this, "Arrow", rotate);
  if (showIcon) {
    if (al::isNerve(this, &NrvGuideFrameOutDisappear))
      al::setNerve(this, &NrvGuideFrameOutStartAppear);
  } else if (al::isNerve(this, &NrvGuideFrameOutAppear))
    al::setNerve(this, &NrvGuideFrameOutStartDisappear);
}
/** @brief Shows the icon and starts its appearance animation. */
void GuideFrameOut::exeStartAppear() {
  if (al::isFirstStep(this)) {
    al::showPaneRootNoRecursive(this);
    initIcon();
    al::startAction(this, "Appear", nullptr);
  }
  if (al::isActionEnd(this, nullptr))
    al::setNerve(this, &NrvGuideFrameOutAppear);
}
/** @brief Starts the waiting animation. */
void GuideFrameOut::exeAppear() {
  if (al::isFirstStep(this))
    al::startAction(this, "Wait", nullptr);
}
/** @brief Plays the disappearance animation. */
void GuideFrameOut::exeStartDisappear() {
  if (al::isFirstStep(this))
    al::startAction(this, "End", nullptr);
  if (al::isActionEnd(this, nullptr))
    al::setNerve(this, &NrvGuideFrameOutDisappear);
}
/** @brief Hides the guide pane. */
void GuideFrameOut::exeDisappear() {
  if (al::isFirstStep(this))
    al::hidePaneRootNoRecursive(this);
}

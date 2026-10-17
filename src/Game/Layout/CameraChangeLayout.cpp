#include "Layout/CameraChangeLayout.hpp"
#include "Layout/CameraIcon.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(CameraChangeLayout, NormalCamera);
NERVE_DECL(CameraChangeLayout, KinopioBrigadeCamera);
NERVE_DECL(CameraChangeLayout, Hide);
NERVE_DECL(CameraChangeLayout, SwitchCameraByStick);
NERVE_DECL(CameraChangeLayout, GyroCamera);
NERVE_DECL(CameraChangeLayout, SwitchCameraByButton);
NERVES_MAKE_NOSTRUCT(CameraChangeLayout, NormalCamera, KinopioBrigadeCamera,
                     Hide, SwitchCameraByStick, GyroCamera,
                     SwitchCameraByButton)
} // namespace
/** @brief Creates the camera icon and selects the initial camera state. */
CameraChangeLayout::CameraChangeLayout(const al::LayoutInitInfo &rInfo,
                                       al::CameraDirector *pCameraDirector,
                                       GuideGameWindow *pGuideGameWindow,
                                       bool isKinopioBrigade)
    : al::NerveExecutor("カメラモード変更レイアウト管理"),
      mMessageSystem(rInfo.getMessageSystem()),
      mGuideGameWindow(pGuideGameWindow), mCameraDirector(pCameraDirector),
      mIsKinopioBrigade(isKinopioBrigade) {
  initNerve(&NrvCameraChangeLayoutNormalCamera, 0);
  mCameraIcon = new CameraIcon(rInfo, isKinopioBrigade);
  if (mIsKinopioBrigade)
    al::setNerve(this, &NrvCameraChangeLayoutKinopioBrigadeCamera);
}
/** @brief Hides the icon immediately or enters the animated hidden state. */
void CameraChangeLayout::hide(bool isImmediate) {
  if (isImmediate)
    mCameraIcon->kill();
  else
    al::setNerve(this, &NrvCameraChangeLayoutHide);
}
/** @brief Updates camera state and reflects camera control restrictions. */
void CameraChangeLayout::update() {
  updateNerve();
  if (al::isNerve(this, &NrvCameraChangeLayoutHide) ||
      al::isNerve(this, &NrvCameraChangeLayoutKinopioBrigadeCamera))
    return;
  bool isLimited = mCameraDirector->isUserControlMoveLimited();
  if (isLimited && !mWasMoveLimited &&
      (al::isNerve(this, &NrvCameraChangeLayoutNormalCamera) ||
       al::isNerve(this, &NrvCameraChangeLayoutSwitchCameraByStick)))
    al::startSe(mCameraDirector, "PgInvalid");
  mWasMoveLimited = isLimited;
  if ((al::isNerve(this, &NrvCameraChangeLayoutNormalCamera) ||
       al::isNerve(this, &NrvCameraChangeLayoutSwitchCameraByStick)) &&
      mCameraDirector->isUserControlMoveLimited())
    mCameraIcon->setCameraNoIcon();
  if (!mCameraDirector->isValidGyroMode())
    mGuideGameWindow->startDisappear(this);
  if (mCameraDirector->isInCameraIconDisplayArea() &&
      mCameraDirector->isValidUserCameraControlMode() &&
      !al::isNerve(this, &NrvCameraChangeLayoutGyroCamera))
    mCameraIcon->startAppear();
}
/** @brief Restores the icon state for the current camera mode. */
void CameraChangeLayout::appearCameraChangeLayout() {
  if (!al::isNerve(this, &NrvCameraChangeLayoutHide))
    return;
  if (mIsKinopioBrigade)
    al::setNerve(this, &NrvCameraChangeLayoutKinopioBrigadeCamera);
  else if (mCameraDirector->isSubCameraGyroMode())
    al::setNerve(this, &NrvCameraChangeLayoutGyroCamera);
  else
    al::setNerve(this, &NrvCameraChangeLayoutNormalCamera);
}
/** @brief Starts hiding the camera layout. */
void CameraChangeLayout::disappearCameraChangeLayout() {
  if (!al::isNerve(this, &NrvCameraChangeLayoutHide))
    al::setNerve(this, &NrvCameraChangeLayoutHide);
}
/** @brief Hides the layout and returns a gyro camera to normal mode. */
void CameraChangeLayout::disappearCameraChangeLayoutAndResetCameraMode() {
  disappearCameraChangeLayout();
  if (mCameraDirector->isSubCameraGyroMode()) {
    mCameraIcon->reset();
    mCameraDirector->switchCameraMode();
  }
}
/** @brief Hides the brigade camera icon. */
void CameraChangeLayout::disappearGyroIconInKinopioBrigade() {
  if (al::isNerve(this, &NrvCameraChangeLayoutKinopioBrigadeCamera))
    mCameraIcon->startEnd();
}
/** @brief Ends the guide message and camera icon. */
void CameraChangeLayout::exeHide() {
  if (al::isFirstStep(this))
    mGuideGameWindow->startDisappear(this);
  mCameraIcon->startEnd();
}
/** @brief Handles camera requests and stick input in normal mode. */
void CameraChangeLayout::exeNormalCamera() {
  if (al::isFirstStep(this))
    mIsSwitchRequested = false;
  if (al::isStep(this, 5))
    al::startAction(mCameraIcon, "Normal", "Camera");
  if (mCameraDirector->isRequestGyroMode()) {
    mCameraDirector->resetRequestGyroModeFlag();
    al::setNerve(this, &NrvCameraChangeLayoutSwitchCameraByButton);
    mShowGyroGuide = false;
    mIsSwitchRequested = true;
    return;
  }
  if (mCameraDirector->isRequestOffGyroMode())
    mCameraDirector->resetRequestOffGyroMode();
  if (mCameraDirector->isValidGyroMode() &&
      mCameraDirector->isSubCameraGyroMode()) {
    al::setNerve(this, &NrvCameraChangeLayoutGyroCamera);
    return;
  }
  if (!mCameraDirector->isValidUserCameraControlMode() ||
      !mCameraDirector->isDisplayCameraIcon())
    mCameraIcon->startEnd();
  if (isInputRightStickTrig() &&
      mCameraDirector->isValidUserCameraControlMode())
    al::setNerve(this, &NrvCameraChangeLayoutSwitchCameraByStick);
}
/** @brief Detects a transition from neutral to camera stick input. */
bool CameraChangeLayout::isInputRightStickTrig() {
  sead::Vector2f stick(0, 0);
  for (s32 i = 0; i < rc::getActiveControlUserNum(GameDataHolderAccessor(
                          GameDataFunction::getGameDataHolder(mCameraIcon)));
       ++i) {
    s32 port = rc::getPadPortByUserId(i);
    if (al::isPadEnableRightStick(port))
      stick.set(al::getRightStick(port));
    else if (GameDataFunction::isSingleMode(
                 GameDataHolderAccessor(mCameraIcon)) &&
             al::isPadTypeJoySingle(port) && al::isPadHoldA(port))
      stick.set(al::getLeftStick(port));
  }
  if (al::isNearZero(stick, 0.001f))
    mIsStickNeutral = true;
  else {
    bool wasNeutral = mIsStickNeutral;
    mIsStickNeutral = false;
    if (wasNeutral)
      return true;
  }
  return false;
}
/** @brief Handles gyro mode requests and delayed guide dismissal. */
void CameraChangeLayout::exeGyroCamera() {
  if (al::isFirstStep(this)) {
    mGuideCloseTimer = 0;
    mIsSwitchRequested = false;
  }
  if (mCameraDirector->isRequestOffGyroMode()) {
    mCameraDirector->resetRequestOffGyroMode();
    al::setNerve(this, &NrvCameraChangeLayoutSwitchCameraByButton);
    mShowGyroGuide = false;
    mIsSwitchRequested = true;
    return;
  }
  if (!mCameraDirector->isValidGyroMode()) {
    al::setNerve(this, &NrvCameraChangeLayoutNormalCamera);
    mCameraIcon->startEnd();
    return;
  }
  if (mCameraDirector->isRequestGyroMode())
    mCameraDirector->resetRequestGyroModeFlag();
  if (mGuideGameWindow->isActive()) {
    if (mGuideCloseTimer == 0 && al::isPadTriggerAny(-1))
      mGuideCloseTimer = 1;
    if (mGuideCloseTimer > 0) {
      ++mGuideCloseTimer;
      if (mGuideCloseTimer == 180)
        mGuideGameWindow->startDisappear(this);
    }
  }
  mCameraIcon->startAppear();
}
/** @brief Updates the player shown as the camera owner. */
void CameraChangeLayout::exeKinopioBrigadeCamera() {
  if (al::isFirstStep(this))
    mCameraIcon->startAppear();
  mCameraIcon->setCameraOwnerID(mCameraDirector->getCameraOwnerID());
}
/** @brief Switches camera modes and shows the corresponding guide. */
void CameraChangeLayout::exeSwitchCameraByButton() {
  mCameraDirector->switchCameraMode();
  mCameraIcon->startEnd();
  if (mCameraDirector->isSubCameraGyroMode()) {
    al::setNerve(this, &NrvCameraChangeLayoutGyroCamera);
    if (mShowGyroGuide)
      mGuideGameWindow->startAppear(
          this,
          al::getSystemMessageString(this, "GuideMessage", "CameraGyroMode"),
          600, 0.0f);
  } else {
    al::setNerve(this, &NrvCameraChangeLayoutNormalCamera);
    mGuideGameWindow->startDisappear(this);
  }
}
/** @brief Refreshes the camera icon after stick input. */
void CameraChangeLayout::exeSwitchCameraByStick() {
  if (mCameraIcon->isAlive())
    mCameraIcon->reset();
  else
    mCameraIcon->startAppear();
  al::setNerve(this, &NrvCameraChangeLayoutNormalCamera);
}
/** @brief Returns the scene message system. */
const al::MessageSystem *CameraChangeLayout::getMessageSystem() const {
  return mMessageSystem;
}
/** @brief Returns the scene object name. */
const char *CameraChangeLayout::getSceneObjName() const {
  return "カメラ変更ウィンドウ";
}
namespace rc {
void appearCameraChangeLayout(const al::IUseSceneObjHolder *pHolder) {
  static_cast<CameraChangeLayout *>(al::getSceneObj(pHolder, 27))
      ->appearCameraChangeLayout();
}
void disappearCameraChangeLayout(const al::IUseSceneObjHolder *pHolder) {
  static_cast<CameraChangeLayout *>(al::getSceneObj(pHolder, 27))
      ->disappearCameraChangeLayout();
}
void disappearCameraChangeLayoutAndResetCameraMode(
    const al::IUseSceneObjHolder *pHolder) {
  static_cast<CameraChangeLayout *>(al::getSceneObj(pHolder, 27))
      ->disappearCameraChangeLayoutAndResetCameraMode();
}
void disappearGyroIconInKinopioBrigade(const al::IUseSceneObjHolder *pHolder) {
  static_cast<CameraChangeLayout *>(al::getSceneObj(pHolder, 27))
      ->disappearGyroIconInKinopioBrigade();
}
} // namespace rc

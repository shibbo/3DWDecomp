#pragma once

#include "Library/Message/IUseMessageSystem.hpp"
#include "Library/Nerve/NerveExecutor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class CameraDirector;
class LayoutInitInfo;
} // namespace al

class GuideGameWindow;
class CameraIcon;

/**
 * @brief Scene object showing the camera mode change layout.
 */
class CameraChangeLayout : public al::NerveExecutor,
                           public al::IUseMessageSystem,
                           public al::ISceneObj {
public:
  CameraChangeLayout(const al::LayoutInitInfo &rInfo,
                     al::CameraDirector *pCameraDirector,
                     GuideGameWindow *pGuideGameWindow, bool isKinopioBrigade);

  const al::MessageSystem *getMessageSystem() const override;
  const char *getSceneObjName() const override;
  void hide(bool isImmediate);
  void update();
  void appearCameraChangeLayout();
  void disappearCameraChangeLayoutAndResetCameraMode();
  void disappearGyroIconInKinopioBrigade();
  void exeHide();
  void exeNormalCamera();
  bool isInputRightStickTrig();
  void exeGyroCamera();
  void exeKinopioBrigadeCamera();
  void exeSwitchCameraByButton();
  void exeSwitchCameraByStick();
  void disappearCameraChangeLayout();

private:
  const al::MessageSystem *mMessageSystem;
  CameraIcon *mCameraIcon = nullptr;
  GuideGameWindow *mGuideGameWindow;
  al::CameraDirector *mCameraDirector;
  s32 mGuideCloseTimer = 0;
  bool mIsStickNeutral = true;
  bool mIsKinopioBrigade;
  bool mShowGyroGuide = true;
  bool mIsSwitchRequested = false;
  bool mWasMoveLimited = false;
};

static_assert(sizeof(CameraChangeLayout) == 0x50);

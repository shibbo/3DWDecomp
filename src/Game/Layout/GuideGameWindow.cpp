#include "Layout/GuideGameWindow.hpp"
#include "Layout/GuideMessage.hpp"
#include "Layout/WindowMessage.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

GuideGameWindow::GuideGameWindow(const al::LayoutInitInfo &rInfo,
                                 bool isSingleMode)
    : mIsSingleMode(isSingleMode) {
  mGuideMessage = new GuideMessage(rInfo);
  mWindowMessage = new WindowMessage(rInfo, "WindowMessage",
                                     "メッセージウインドウ", nullptr);
}
bool GuideGameWindow::isActive() const {
  if (!mIsSingleMode)
    return mGuideMessage->isAlive();
  return mGuideMessage->isAlive() || mWindowMessage->isAlive();
}
bool GuideGameWindow::isWindowMessageActive() const {
  return mWindowMessage->isAlive();
}
bool GuideGameWindow::isWaitConfirm() const {
  return mIsWaitAppear || mWindowMessage->isAlive();
}
void GuideGameWindow::startAppear(const void *pUser, const char16_t *pText,
                                  s32 frame, f32 posY) {
  mUser = pUser;
  mIsHidden = false;
  mGuideMessage->startAppear(pText, frame, posY);
}
void GuideGameWindow::startAppear(const void *pUser, const char *pCategory,
                                  const char *pLabel, s32 frame, f32 posY,
                                  bool isForce) {
  mUser = pUser;
  mIsHidden = false;
  mGuideMessage->startAppear(pCategory, pLabel, frame, posY, isForce);
}
void GuideGameWindow::startAppearSplit(const void *pUser, const char16_t *pLeft,
                                       const char16_t *pRight, s32 frame,
                                       f32 posY) {
  mUser = pUser;
  mIsHidden = false;
  mGuideMessage->startAppearSplit(pLeft, pRight, frame, posY);
}
void GuideGameWindow::startAppear(const void *, const char *pLabel,
                                  bool isSingleMode) {
  mWindowMessage->appearWithSystemMessage(
      isSingleMode ? "SingleMode_WindowMessage" : "WindowMessage", pLabel,
      rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(mWindowMessage)));
}
void GuideGameWindow::startAppearCutscene(const void *, const char *pLabel) {
  mIsHidden = false;
  mWindowMessage->appearWithSystemMessage("SingleMode_GenericDialogCutscene",
                                          pLabel, al::getMainControllerPort());
}
void GuideGameWindow::startAppearAssist(const void *pUser) {
  mUser = pUser;
  mIsHidden = false;
  mGuideMessage->startAppearAssist();
}
void GuideGameWindow::startDisappear(const void *pUser) {
  if (mUser == pUser && mGuideMessage->isAlive()) {
    mPriorityLimit = GuideMessagePriority(0);
    mGuideMessage->startEnd();
  }
}
void GuideGameWindow::forceDisappear(const void *pUser) {
  mUser = pUser;
  if (mGuideMessage->isAlive()) {
    mPriorityLimit = GuideMessagePriority(0);
    mGuideMessage->startEnd();
  }
}
void GuideGameWindow::startHide(const void *) { mGuideMessage->hide(); }
void GuideGameWindow::endHide(const void *) { mGuideMessage->unHide(); }
void GuideGameWindow::forceEndWindowMessage() {
  if (mWindowMessage->isAlive())
    mWindowMessage->kill();
}
void GuideGameWindow::setIsWaitAppear(const void *pUser, bool isWait) {
  if (isWait)
    mUser = pUser;
  mIsWaitAppear = isWait;
}
bool GuideGameWindow::isCurrentUser(const void *pUser) const {
  return mUser == pUser;
}
namespace rc {
GuideGameWindow *getGuideGameWindow(const al::IUseSceneObjHolder *pHolder) {
  return static_cast<GuideGameWindow *>(al::tryGetSceneObj(pHolder, 28));
}
void appearGuideGameWindow(const al::IUseSceneObjHolder *pHolder,
                           const char16_t *pText, s32 frame) {
  if (auto *pWindow = getGuideGameWindow(pHolder))
    pWindow->startAppear(pHolder, pText, frame, 0.0f);
}
void appearGuideGameWindow(const al::IUseSceneObjHolder *pHolder,
                           const char *pCategory, const char *pLabel, s32 frame,
                           f32 posY) {
  auto *pWindow = getGuideGameWindow(pHolder);
  if (!pWindow)
    return;
  if (al::isEqualString(pLabel, "RaidonRide") ||
      al::isEqualString(pLabel, "RabbitDash") ||
      al::isEqualString(pLabel, "WorldMapStart") ||
      al::isEqualString(pLabel, "KinopioBrigade") ||
      (!GameDataFunction::isSingleMode(GameDataHolderAccessor(pHolder)) &&
       al::isEqualString(pLabel, "PlayerControlsKiosk"))) {
    al::StringTmp<256> left(pLabel), right(pLabel);
    left.append("DualJoycons");
    right.append("SingleJoycons");
    const char16_t *pLeft = al::getSystemMessageString(
        pWindow->getGuideMessage(), pCategory, left.cstr());
    const char16_t *pRight = al::getSystemMessageString(
        pWindow->getGuideMessage(), pCategory, right.cstr());
    pWindow->startAppearSplit(pHolder, pLeft, pRight, frame, posY);
  } else {
    pWindow->startAppear(pHolder,
                         al::getSystemMessageString(pWindow->getGuideMessage(),
                                                    pCategory, pLabel),
                         frame, posY);
  }
}
bool appearGuideGameWindowWithPriority(const al::IUseSceneObjHolder *pHolder,
                                       const char *pCategory,
                                       const char *pLabel,
                                       GuideMessagePriority priority, s32 frame,
                                       f32 posY) {
  auto *pWindow = getGuideGameWindow(pHolder);
  if (!pWindow || pWindow->isWaitConfirm() ||
      pWindow->getGuideMessage()->isHide())
    return false;
  s32 current = pWindow->getPriorityLimit();
  if (current == 7)
    return false;
  if (current == 5) {
    if (priority <= 5)
      return false;
  } else {
    if (pWindow->getGuideMessage()->isAlive()) {
      if (current >= 2) {
        if (current >= priority)
          return false;
        goto appear;
      }
    } else
      current = 0;
    if (current > priority)
      return false;
  }
appear:
  if (priority == 6) {
    pWindow->startAppearAssist(pHolder);
    pWindow->setPriorityLimit(GuideMessagePriority(6));
  } else {
    pWindow->startAppear(pHolder, pCategory, pLabel, frame, posY, true);
    pWindow->setPriorityLimit(priority);
  }
  return true;
}
void disableGuideGameWindowPriority(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->setPriorityLimit(GuideMessagePriority(7));
}
void enableGuideGameWindowPriority(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->setPriorityLimit(GuideMessagePriority(0));
}
void waitAppearGuideGameWindowWithConfirm(
    const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->setIsWaitAppear(pHolder, true);
}
void appearGuideGameWindowWithConfirm(const al::IUseSceneObjHolder *pHolder,
                                      const char *pLabel, bool isSingleMode) {
  if (auto *p = getGuideGameWindow(pHolder)) {
    p->startAppear(pHolder, pLabel, isSingleMode);
    p->setIsWaitAppear(pHolder, false);
  }
}
void appearCutsceneGuideGameWindow(const al::IUseSceneObjHolder *pHolder,
                                   const char *pLabel) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->startAppearCutscene(pHolder, pLabel);
}
void disappearGuideGameWindow(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->startDisappear(pHolder);
}
void forceDisappearGuideGameWindow(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->forceDisappear(pHolder);
}
void hideGuideGameWindow(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->startHide(pHolder);
}
void unHideGuideGameWindow(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->endHide(pHolder);
}
bool isGuideGameWindowActive(const al::IUseSceneObjHolder *pHolder) {
  auto *p = getGuideGameWindow(pHolder);
  return p && p->isActive();
}
bool isGuideGameWindowWaitConfirm(const al::IUseSceneObjHolder *pHolder) {
  auto *p = getGuideGameWindow(pHolder);
  return p && p->isWaitConfirm();
}
void setGuideGameWindowWaitConfirm(const al::IUseSceneObjHolder *pHolder,
                                   bool) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->setIsWaitAppear(pHolder, false);
}
bool isCurrentGuideGameWindowUser(const al::IUseSceneObjHolder *pHolder) {
  auto *p = getGuideGameWindow(pHolder);
  return p && p->isCurrentUser(pHolder);
}
void forceEndGuideGameWindow(const al::IUseSceneObjHolder *pHolder) {
  if (auto *p = getGuideGameWindow(pHolder))
    p->forceEndWindowMessage();
}
} // namespace rc

#include "Layout/WindowConfirm.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/CursorTarget.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/InputUtil.hpp"
namespace {
const char *sConfirmNames[] = {"確認ウインドウ[自動]",
                               "確認ウインドウ[１ボタン]",
                               "確認ウインドウ[２ボタン]"};
const char *sConfirmLayouts[] = {"WindowConfirmReport", "WindowConfirmSingle",
                                 "WindowConfirmDouble"};
inline const char *getConfirmName(WindowConfirmType type) {
  return sConfirmNames[type];
}
inline const char *getConfirmLayout(WindowConfirmType type) {
  return sConfirmLayouts[type];
}
NERVE_DECL(WindowConfirm, Wait);
NERVE_DECL(WindowConfirm, Appear);
NERVE_DECL(WindowConfirm, AppearAIcon);
NERVE_DECL(WindowConfirm, End);
class WindowConfirmNrvForceEnd : public al::Nerve {
public:
  void execute(al::NerveKeeper *pKeeper) const override {
    pKeeper->getParent<WindowConfirm>()->exeEnd();
  }
};
NERVE_DECL(WindowConfirm, WaitAIcon);
NERVES_MAKE_NOSTRUCT(WindowConfirm, Wait, Appear, AppearAIcon, End, ForceEnd,
                     WaitAIcon)
} // namespace
WindowConfirm::WindowConfirm(WindowConfirmType type,
                             const al::LayoutInitInfo &rInfo,
                             const char *pSuffix, bool isPauseMenu)
    : al::LayoutActor(getConfirmName(type)), mIsPauseMenu(isPauseMenu) {
  al::initLayoutActor(this, rInfo, getConfirmLayout(type), pSuffix);
  initNerve(&NrvWindowConfirmWait, 0);
  mButtons =
      new ButtonGroup(rInfo, this, getConfirmLayout(type), nullptr, false);
  kill();
}
void WindowConfirm::appearWithSystemMessage(const char *pFileName,
                                            const char *pLabel, s32 port,
                                            const char *pArg) {
  al::LayoutActor::appear();
  al::startAction(this, "Appear", nullptr);
  mPort = port;
  mButtons->setPort(port);
  mButtons->reset();
  if (mButtons->getCursorParts())
    mButtons->getCursorParts()->reset();
  al::setPaneSystemMessage(this, "TxtMessage", pFileName, pLabel);
  if (isButtonOne()) {
    al::StringTmp<128> label("%s_Btn", pLabel);
    const char16_t *pMessage =
        al::getSystemMessageString(this, pFileName, label.cstr());
    al::setPaneString(mButtons->getButton("ボタン"), "TxtButton", pMessage, 0,
                      -1);
    if (pArg)
      mButtons->select(pArg);
    else
      mButtons->select("ボタン");
  } else if (isButtonTwo()) {
    al::StringTmp<128> left("%s_Left", pLabel);
    const char16_t *pLeft =
        al::getSystemMessageString(this, pFileName, left.cstr());
    al::setPaneString(mButtons->getButton("Lボタン"), "TxtButton", pLeft, 0,
                      -1);
    al::StringTmp<128> right("%s_Right", pLabel);
    const char16_t *pRight =
        al::getSystemMessageString(this, pFileName, right.cstr());
    al::setPaneString(mButtons->getButton("Rボタン"), "TxtButton", pRight, 0,
                      -1);
    if (pArg)
      mButtons->select(pArg);
    else
      mButtons->select("Lボタン");
  } else {
    if (mIconDelay > 0) {
      al::startAction(this, "HideAIcon", "AIcon");
      al::setNerve(this, &NrvWindowConfirmAppear);
    } else
      al::setNerve(this, &NrvWindowConfirmAppearAIcon);
    return;
  }
  al::setNerve(this, &NrvWindowConfirmAppear);
}
bool WindowConfirm::isButtonOne() const {
  return mButtons->getButtonNum() == 1;
}
bool WindowConfirm::isButtonTwo() const {
  return mButtons->getButtonNum() == 2;
}
void WindowConfirm::appearWithCustomMessage(const char16_t *pMessage,
                                            s32 port) {
  al::LayoutActor::appear();
  al::startAction(this, "Appear", nullptr);
  mPort = port;
  al::setPaneString(this, "TxtMessage", pMessage, 0, -1);
  if (mIconDelay > 0) {
    al::startAction(this, "HideAIcon", "AIcon");
    al::setNerve(this, &NrvWindowConfirmAppear);
  } else
    al::setNerve(this, &NrvWindowConfirmAppearAIcon);
}
bool WindowConfirm::isDecideLeft() const {
  return mButtons->isDecide("Lボタン");
}
bool WindowConfirm::isDecideLeftEnd() const {
  return mButtons->isDecideEnd("Lボタン");
}
bool WindowConfirm::isDecideRight() const {
  return mButtons->isDecide("Rボタン");
}
bool WindowConfirm::isDecideRightEnd() const {
  return mButtons->isDecideEnd("Rボタン");
}
bool WindowConfirm::isDecideOne() const { return mButtons->isDecide("ボタン"); }
bool WindowConfirm::isDecideOneEnd() const {
  return mButtons->isDecideEnd("ボタン");
}
bool WindowConfirm::isEnding() const {
  return al::isNerve(this, &NrvWindowConfirmEnd) ||
         al::isNerve(this, &NrvWindowConfirmForceEnd);
}
void WindowConfirm::forceExit() {
  if (al::isNerve(this, &NrvWindowConfirmWait) ||
      al::isNerve(this, &NrvWindowConfirmWaitAIcon)) {
    if (isExistButton()) {
      mButtons->hideCursor();
      mButtons->invalidate();
    }
    al::setNerve(this, &NrvWindowConfirmForceEnd);
  }
}
bool WindowConfirm::isExistButton() const {
  return mButtons->getButtonNum() > 0;
}
void WindowConfirm::exeAppear() {
  if (al::isFirstStep(this)) {
    al::startAction(this, "Appear", "Main");
    mButtons->invalidate();
  }
  if (al::isActionEnd(this, nullptr)) {
    if (isExistButton()) {
      mButtons->validate();
      mButtons->showCursor();
    }
    al::setNerve(this, &NrvWindowConfirmWait);
  }
}
void WindowConfirm::exeAppearAIcon() {
  if (al::isFirstStep(this)) {
    al::setPaneString(this, "TxtIconA",
                      LayoutFontUtil::getSystemFontBtnDecide(mPort), 0, -1);
    al::startAction(this, "Appear", "AIcon");
    return;
  }
  if (al::isActionEnd(this, "AIcon"))
    al::setNerve(this, &NrvWindowConfirmWaitAIcon);
}
void WindowConfirm::exeWait() {
  if (al::isFirstStep(this))
    al::startAction(this, "Wait", nullptr);
  if (isExistButton()) {
    mButtons->updateAndCursorDefault(mPort);
    if (isButtonTwo() && !mButtons->isDecideAny() &&
        rc::isPadTriggerUiCancelByPort(mPort))
      mButtons->decide("Lボタン");
    if (mButtons->isDecideAny()) {
      mButtons->hideCursor();
      mButtons->invalidate();
      if (isButtonTwo()) {
        if (isDecideLeft())
          al::startSe(this, "Canceled", nullptr);
        else if (isDecideRight())
          al::startSe(this, "Decided", nullptr);
      } else if (isButtonOne()) {
        if (isDecideOne())
          al::startSe(this, "Decided", nullptr);
      }
      al::setNerve(this, &NrvWindowConfirmEnd);
    }
  } else if (!al::isLessStep(this, 60) && !al::isLessStep(this, mIconDelay))
    al::setNerve(this, &NrvWindowConfirmAppearAIcon);
}
void WindowConfirm::exeWaitAIcon() {
  if (al::isFirstStep(this))
    al::startAction(this, "Wait", "AIcon");
  if (rc::isPadTriggerUiDecideByPort(mPort)) {
    if (mIsPauseMenu)
      al::startSe(this, "Decided", nullptr);
    al::setNerve(this, &NrvWindowConfirmEnd);
  }
}
void WindowConfirm::exeEnd() {
  if (al::isNerve(this, &NrvWindowConfirmForceEnd) || !mIsKeepOpen) {
    if (al::isFirstStep(this))
      al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr))
      kill();
  }
}

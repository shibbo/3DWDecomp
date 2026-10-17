#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/InputUtil.hpp"
namespace {
const char16_t *getGuideBarMessage(const al::LayoutActor *pLayout,
                                   RCSControlGuideBar::GuideBarMsgType type,
                                   int port) {
  const char *pLabel = nullptr;
  switch (type) {
  case RCSControlGuideBar::GuideBarMsgType_Default:
    pLabel = al::isPadTypeJoySingle(port) ? "ControlGuideBar_Back_Gen"
                                          : "ControlGuideBar_Back";
    break;
  case RCSControlGuideBar::GuideBarMsgType_ListClearStar:
    pLabel =
        al::isPadTypeJoySingle(port) ? "ListClearStar_Gen" : "ListClearStar";
    break;
  case RCSControlGuideBar::GuideBarMsgType_ListStamp:
    pLabel = al::isPadTypeJoySingle(port) ? "ListStamp_Gen" : "ListStamp";
    break;
  case RCSControlGuideBar::GuideBarMsgType_MapMenu:
    pLabel = al::isPadTypeJoySingle(port) ? "MapMenu_Gen" : "MapMenu";
    break;
  case RCSControlGuideBar::GuideBarMsgType_PauseMenu:
    pLabel = al::isPadTypeJoySingle(port) ? "PauseMenu_Gen" : "PauseMenu";
    break;
  case RCSControlGuideBar::GuideBarMsgType_PauseMenuMap:
    pLabel = al::isPadTypeJoySingle(port) ? "PauseMenuMap_Gen" : "PauseMenuMap";
    break;
  case RCSControlGuideBar::GuideBarMsgType_ControlGuide:
    pLabel = al::isPadTypeJoySingle(port) ? "RCS_ControlGuide_Gen"
                                          : "RCS_ControlGuide";
    break;
  case RCSControlGuideBar::GuideBarMsgType_FileSelect:
    pLabel = al::isPadTypeJoySingle(port) ? "RCS_SaveData_Gen" : "RCS_SaveData";
    break;
  case RCSControlGuideBar::GuideBarMsgType_Title:
    pLabel = al::isPadTypeJoySingle(port) ? "TitleLogo_Gen" : "TitleLogo";
    break;
  case RCSControlGuideBar::GuideBarMsgType_VoiceInfoOption:
    pLabel = al::isPadTypeJoySingle(port)
                 ? "RCS_ControlGuideBar_VoiceInfoOption_SingleJoycon"
                 : "RCS_ControlGuideBar_VoiceInfoOption";
    break;
  case RCSControlGuideBar::GuideBarMsgType_KoopaJrDemo:
    pLabel = al::isPadTypeJoySingle(port) ? "Confirm_Gen" : "Confirm";
    break;
  case RCSControlGuideBar::GuideBarMsgType_NetworkBack:
    pLabel = "ControlGuideBar_NetworkBack";
    break;
  }
  return al::getSystemMessageString(pLayout, "RCS_ControlGuideBar", pLabel);
}
inline const char16_t *
getCurrentGuideBarMessage(const RCSControlGuideBar *pGuide, int port,
                          RCSControlGuideBar::GuideBarMsgType type) {
  return getGuideBarMessage(pGuide, type, port);
}
NERVE_DECL(RCSControlGuideBar, Wait);
NERVE_DECL(RCSControlGuideBar, Appear);
NERVE_DECL(RCSControlGuideBar, End);
NERVE_DECL(RCSControlGuideBar, FadeTextOut);
class RCSControlGuideBarNrvFadeTextOutTitle : public al::Nerve {
public:
  void execute(al::NerveKeeper *pKeeper) const override {
    pKeeper->getParent<RCSControlGuideBar>()->exeFadeTextOut();
  }
};
NERVE_DECL(RCSControlGuideBar, FadeTextIn);
NERVES_MAKE_NOSTRUCT(RCSControlGuideBar, Wait, Appear, End, FadeTextOut,
                     FadeTextOutTitle, FadeTextIn)
} // namespace
RCSControlGuideBar::RCSControlGuideBar(const al::LayoutInitInfo &rInfo)
    : al::LayoutActor("ControlGuideBar") {
  al::initLayoutActor(this, rInfo, "RCS_ControlGuideBar", nullptr);
  mParts = new al::LayoutActor("RCSControlGuideBarParts");
  al::initLayoutPartsActor(mParts, this, rInfo, "ParControlGuideBar", nullptr);
  initNerve(&NrvRCSControlGuideBarWait, 0);
}
void RCSControlGuideBar::appear() {
  al::LayoutActor::appear();
  al::setNerve(this, &NrvRCSControlGuideBarAppear);
}
void RCSControlGuideBar::hide() {
  al::startAction(this, "Hide", nullptr);
  al::setNerve(this, &NrvRCSControlGuideBarWait);
}
void RCSControlGuideBar::show() { al::startAction(this, "Wait", nullptr); }
void RCSControlGuideBar::end() {
  al::setNerve(this, &NrvRCSControlGuideBarEnd);
}
void RCSControlGuideBar::overridePort(s32 port) {
  if (mPort == port)
    return;
  mPort = port;
  al::setPaneString(mParts, "TxtInfo",
                    getGuideBarMessage(this, mMsgType, mPort), 0, -1);
}
void RCSControlGuideBar::appearWithMessage(GuideBarMsgType type, s32 port) {
  mMsgType = type;
  mPort = port;
  al::setPaneString(mParts, "TxtInfo", getGuideBarMessage(this, type, port), 0,
                    -1);
  al::startAction(mParts, "TextInfo_IDLE", "Text");
  appear();
}
void RCSControlGuideBar::changeText(GuideBarMsgType type, s32 port,
                                    bool isForce) {
  mMsgType = type;
  mPort = port;
  if (isForce)
    al::setNerve(this, &NrvRCSControlGuideBarFadeTextOut);
  else
    al::setPaneString(mParts, "TxtInfo", getGuideBarMessage(this, type, port),
                      0, -1);
}
void RCSControlGuideBar::changeTextTitle(GuideBarMsgType type, s32 port) {
  mMsgType = type;
  mPort = port;
  al::setNerve(this, &NrvRCSControlGuideBarFadeTextOutTitle);
}
RCSControlGuideBar::GuideBarMsgType RCSControlGuideBar::getMsgType() const {
  return mMsgType;
}
s32 RCSControlGuideBar::getPort() const { return mPort; }
void RCSControlGuideBar::setCharacter(const char *pCharacterName) {
  al::startAction(
      mParts, al::StringTmp<32>("SetIcon_%s", pCharacterName).cstr(), "Player");
}
void RCSControlGuideBar::setCharacterKinopioBrigade(s32 port) {
  al::setPaneString(mParts, "TxtCharacterCaptainK",
                    LayoutFontUtil::getPictureFontKinopioBrigade(port - 1), 0,
                    -1);
  al::startAction(mParts, "SetIcon_CaptainK", "Player");
}
void RCSControlGuideBar::setCharacterSingleMode(s32 port) {
  if (al::getMainControllerPort() == port)
    al::startAction(mParts, "SetIcon_Mario", "Player");
  else
    al::startAction(mParts, "SetIcon_KoopaJr", "Player");
}
void RCSControlGuideBar::startSave() {
  al::startAction(mParts, "FileOverlay_Save", "FileOverlay");
}
void RCSControlGuideBar::startDelete() {
  al::startAction(mParts, "FileOverlay_Delete", "FileOverlay");
}
void RCSControlGuideBar::appearIcon() {
  al::startAction(mParts, "Appear_Icon", "Player");
}
void RCSControlGuideBar::endIcon() {
  al::startAction(mParts, "End_Icon", "Player");
}
bool RCSControlGuideBar::isOverlayFinished() {
  return al::isActionEnd(mParts, "FileOverlay");
}
void RCSControlGuideBar::exeAppear() {
  if (al::isFirstStep(this))
    al::startAction(this, "Appear", "Main");
  if (al::isActionEnd(this, "Main"))
    al::setNerve(this, &NrvRCSControlGuideBarWait);
}
void RCSControlGuideBar::exeWait() {
  if (rc::isControllerAssignmentChanged()) {
    al::setPaneString(mParts, "TxtInfo",
                      getCurrentGuideBarMessage(this, mPort, mMsgType), 0, -1);
  }
}
void RCSControlGuideBar::exeFadeTextOut() {
  if (al::isFirstStep(this)) {
    if (al::isNerve(this, &NrvRCSControlGuideBarFadeTextOutTitle))
      al::startAction(mParts, "TextInfo_OUTRO_Title", "Text");
    else
      al::startAction(mParts, "TextInfo_OUTRO", "Text");
  }
  if (al::isActionEnd(mParts, "Text")) {
    if (al::isNerve(this, &NrvRCSControlGuideBarFadeTextOut) ||
        al::isNerve(this, &NrvRCSControlGuideBarFadeTextOutTitle)) {
      al::setPaneString(mParts, "TxtInfo",
                        getCurrentGuideBarMessage(this, mPort, mMsgType), 0,
                        -1);
      al::setNerve(this, &NrvRCSControlGuideBarFadeTextIn);
    } else
      al::setNerve(this, &NrvRCSControlGuideBarWait);
  }
}
void RCSControlGuideBar::exeFadeTextIn() {
  if (al::isFirstStep(this))
    al::startAction(mParts, "TextInfo_INTRO", "Text");
  if (al::isActionEnd(mParts, "Text"))
    al::setNerve(this, &NrvRCSControlGuideBarWait);
}
void RCSControlGuideBar::exeEnd() {
  if (al::isFirstStep(this))
    al::startAction(this, "End", "Main");
  if (al::isActionEnd(this, "Main"))
    kill();
}
void RCSControlGuideBar::appearTitle() {
  al::LayoutActor::appear();
  al::startAction(mParts, "AppearTitle", "TitleAppear");
}
void RCSControlGuideBar::showTitle() {
  al::LayoutActor::appear();
  show();
  al::startAction(mParts, "ShowTitle", "TitleAppear");
}
void RCSControlGuideBar::endTitle(bool isAnim) {
  if (isAnim)
    al::startAction(mParts, "EndTitle", "TitleAppear");
}
void RCSControlGuideBar::hideText() { al::hidePane(mParts, "TxtInfo"); }
void RCSControlGuideBar::fadeTextIn() {
  al::startAction(mParts, "TextInfo_INTRO", "Text");
  al::showPane(mParts, "TxtInfo");
}
bool RCSControlGuideBar::isChangingText() {
  return al::isNerve(this, &NrvRCSControlGuideBarFadeTextOutTitle);
}

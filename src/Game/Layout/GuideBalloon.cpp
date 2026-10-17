#include "Layout/GuideBalloon.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Sequence/DemoDirector.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
#define GUIDE_NERVE(State, Func)                                               \
  class GuideBalloonNrv##State : public al::Nerve {                            \
  public:                                                                      \
    void execute(al::NerveKeeper *pKeeper) const override {                    \
      pKeeper->getParent<GuideBalloon>()->exe##Func();                         \
    }                                                                          \
  };
GUIDE_NERVE(Appear, Appear)
GUIDE_NERVE(AppearDRC, Appear)
GUIDE_NERVE(AppearThought, Appear)
GUIDE_NERVE(End, End)
GUIDE_NERVE(EndThought, End)
GUIDE_NERVE(WaitThought, Wait)
GUIDE_NERVE(WaitDRC, Wait)
GUIDE_NERVE(Wait, Wait)
#undef GUIDE_NERVE
NERVES_MAKE_NOSTRUCT(GuideBalloon, Appear, AppearDRC, AppearThought, End,
                     EndThought, WaitThought, WaitDRC, Wait)
} // namespace
GuideBalloon::GuideBalloon(const char *pName, const al::LayoutInitInfo &rInfo,
                           const sead::Vector3f *pTrans,
                           const sead::Vector3f &rOffset, bool isFollow,
                           al::DemoDirector *pDemoDirector)
    : al::LayoutActor(pName), mSceneCameraInfo(rInfo.getSceneCameraInfo()),
      mTrans(pTrans), mOffset(rOffset), mDemoDirector(pDemoDirector),
      mIsDemoSensitive(isFollow), mIsUpsideDown(false) {
  al::initLayoutActor(this, rInfo, "GuideBalloon", nullptr);
  initNerve(&NrvGuideBalloonAppear, 0);
  kill();
}
GuideBalloon::GuideBalloon(const char *pName, const char *pLayoutName,
                           const al::LayoutInitInfo &rInfo,
                           const sead::Vector3f *pTrans,
                           const sead::Vector3f &rOffset, bool isFollow,
                           al::DemoDirector *pDemoDirector)
    : al::LayoutActor(pName), mSceneCameraInfo(rInfo.getSceneCameraInfo()),
      mTrans(pTrans), mOffset(rOffset), mDemoDirector(pDemoDirector),
      mIsDemoSensitive(isFollow) {
  al::initLayoutActor(this, rInfo, pLayoutName, nullptr);
  initNerve(&NrvGuideBalloonAppear, 0);
  kill();
}
void GuideBalloon::startShowMike() {}
void GuideBalloon::startShowDrcTouch(bool isUpdate) {
  if (isAlive() || isDemoOn())
    return;
  al::LayoutActor::appear();
  if (mIsDemoSensitive && isUpdate) {
    updateDRCIcon(true);
    al::setNerve(this, &NrvGuideBalloonAppearDRC);
  } else {
    al::setNerve(this, &NrvGuideBalloonAppear);
    al::startAction(this, "DRCTouch", "Icon");
  }
}
bool GuideBalloon::isDemoOn() {
  if (mIsDemoSensitive && mDemoDirector && mDemoDirector->isAnyActiveDemo())
    return true;
  return false;
}
void GuideBalloon::updateDRCIcon(bool isForce) {
  if (SingleModeDataFunction::getIs2PAssistMode(GameDataHolderAccessor(this))) {
    if (al::isPadTypeJoySingle(rc::getPadPortByUserId(1))) {
      if (!isForce && mDRCIconType == 1)
        return;
      al::setPaneString(this, "TxtIcon",
                        LayoutFontUtil::getSystemFontBtnYSingleJoy(), 0, -1);
      mDRCIconType = 1;
    } else {
      if (!isForce && mDRCIconType == 2)
        return;
      al::setPaneString(this, "TxtIcon", LayoutFontUtil::getSystemFontBtnY(), 0,
                        -1);
      mDRCIconType = 2;
    }
    al::startAction(this, "Text", "Icon");
  } else {
    if (!isForce && mDRCIconType == 0)
      return;
    mDRCIconType = 0;
    al::startAction(this, "DRCTouch", "Icon");
  }
}
void GuideBalloon::startShowGoalItem(int count) {
  if (isAlive() || isDemoOn())
    return;
  al::LayoutActor::appear();
  al::setNerve(this, &NrvGuideBalloonAppear);
  al::setPaneString(this, "TxtGoalNum",
                    sead::WFormatFixedSafeString<6>(u"%d", count).cstr(), 0,
                    -1);
  al::startAction(this, "GoalItem", "Icon");
}
void GuideBalloon::startShowNeko(neko::ColorType colorType, int foundNum,
                                 int maxNum) {
  if (isAlive())
    return;
  al::LayoutActor::appear();
  al::setNerve(this, &NrvGuideBalloonAppearThought);
  al::setPaneString(
      this, "TxtNeko",
      sead::WFormatFixedSafeString<6>(u"%d/%d", foundNum, maxNum).cstr(), 0,
      -1);
  al::startAction(this, "NPC_A", "Icon");
}
inline void GuideBalloon::updatePosition() {
  const sead::Vector3f offset(mOffset);
  sead::Vector3f pos(*mTrans);
  pos += offset;
  rc::updateLayoutPosAtWorldPos(this, this, pos, 100.0f);
}
void GuideBalloon::startShow(const char16_t *pMessage) {
  if (isAlive())
    return;
  al::LayoutActor::appear();
  al::setNerve(this, &NrvGuideBalloonAppear);
  updatePosition();
  al::startAction(this, "Text", "Icon");
  al::setPaneString(this, "TxtIcon", pMessage, 0, -1);
}
void GuideBalloon::startShowNew() {
  if (isAlive())
    return;
  al::LayoutActor::appear();
  al::setNerve(this, &NrvGuideBalloonAppear);
  updatePosition();
  al::startAction(this, "New", "Icon");
}
void GuideBalloon::endShow() {
  if (!isAlive() || al::isNerve(this, &NrvGuideBalloonEnd) ||
      al::isNerve(this, &NrvGuideBalloonEndThought))
    return;
  if (al::isNerve(this, &NrvGuideBalloonAppearThought) ||
      al::isNerve(this, &NrvGuideBalloonWaitThought))
    al::setNerve(this, &NrvGuideBalloonEndThought);
  else
    al::setNerve(this, &NrvGuideBalloonEnd);
}
void GuideBalloon::exeAppear() {
  if (al::isFirstStep(this)) {
    if (al::isNerve(this, &NrvGuideBalloonAppearThought))
      al::startAction(this, "AppearThought", nullptr);
    else
      al::startAction(this, "Appear", nullptr);
  }
  if (isDemoOn()) {
    kill();
    return;
  }
  updatePosition();
  if (al::isNerve(this, &NrvGuideBalloonAppearDRC))
    updateDRCIcon(false);
  if (al::isActionEnd(this, nullptr)) {
    if (al::isNerve(this, &NrvGuideBalloonAppearThought))
      al::setNerve(this, &NrvGuideBalloonWaitThought);
    else if (al::isNerve(this, &NrvGuideBalloonAppearDRC))
      al::setNerve(this, &NrvGuideBalloonWaitDRC);
    else
      al::setNerve(this, &NrvGuideBalloonWait);
  }
}
void GuideBalloon::exeWait() {
  if (al::isFirstStep(this)) {
    if (al::isNerve(this, &NrvGuideBalloonWaitThought))
      al::startAction(this, "WaitThought", nullptr);
    else
      al::startAction(this, "Wait", nullptr);
  }
  if (isDemoOn()) {
    kill();
    return;
  }
  updatePosition();
  if (al::isNerve(this, &NrvGuideBalloonWaitDRC))
    updateDRCIcon(false);
}
void GuideBalloon::exeEnd() {
  if (al::isFirstStep(this)) {
    if (al::isNerve(this, &NrvGuideBalloonEndThought))
      al::startAction(this, "EndThought", nullptr);
    else
      al::startAction(this, "End", nullptr);
  }
  if (isDemoOn()) {
    kill();
    return;
  }
  updatePosition();
  if (al::isActionEnd(this, nullptr))
    kill();
}
void GuideBalloon::setUpsideDown() {
  al::startAction(this, "SetBalloonUpsideDown", "Beaks");
  mIsUpsideDown = true;
}
void GuideBalloon::setAppear() { al::setNerve(this, &NrvGuideBalloonAppear); }

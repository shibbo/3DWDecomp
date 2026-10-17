#include "Layout/ShowCourseSelect.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(ShowCourseSelect, Appear);
NERVE_DECL(ShowCourseSelect, WaitCursor);
NERVE_DECL(ShowCourseSelect, Wait);
class ShowCourseSelectNrvDecide : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<ShowCourseSelect>()->exeEnd(); }
};
class ShowCourseSelectNrvBack : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<ShowCourseSelect>()->exeEnd(); }
};
NERVES_MAKE_NOSTRUCT(ShowCourseSelect, Appear, WaitCursor, Decide, Back, Wait)
}
/** @brief Creates the show menu for the first user's controller. */
ShowCourseSelect::ShowCourseSelect(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("E3コース選択"), mPort(rc::getPadPortByUserId(0)) {
    al::initLayoutActor(this, rInfo, "ShowCourseSelect", nullptr);
    mButtons = new ButtonGroup(rInfo, this, "ShowCourseSelect", nullptr, false);
    mButtons->setPort(mPort);
    initNerve(&NrvShowCourseSelectAppear, 0);
    kill();
}
/** @brief Resets selection to the first course before showing the menu. */
void ShowCourseSelect::appear() {
    al::LayoutActor::appear();
    mButtons->reset();
    mButtons->select("ステージ1");
    mButtons->invalidate();
    mDecideCourseIndex = -1;
    al::setNerve(this, &NrvShowCourseSelectAppear);
}
/** @brief Shows the cursor after the entrance animation. */
void ShowCourseSelect::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr)) {
        mButtons->showCursor();
        al::setNerve(this, &NrvShowCourseSelectWaitCursor);
    }
}
/** @brief Changes the controller used by the menu. */
void ShowCourseSelect::setControllerPort(int port) { mPort = port; mButtons->setPort(port); }
/** @brief Returns the confirmed course index. */
int ShowCourseSelect::getDecideCourseIndex() const { return mDecideCourseIndex; }
/** @brief Reports a confirmed course. */
bool ShowCourseSelect::isDecide() const { return al::isNerve(this, &NrvShowCourseSelectDecide); }
/** @brief Reports the back state. */
bool ShowCourseSelect::isBack() const { return al::isNerve(this, &NrvShowCourseSelectBack); }
/** @brief Enables input once the cursor entrance has finished. */
void ShowCourseSelect::exeWaitCursor() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (!mButtons->isWaitCursorAppear()) {
        mButtons->validate();
        al::setNerve(this, &NrvShowCourseSelectWait);
    }
}
/** @brief Updates selection and completes a confirmed choice. */
void ShowCourseSelect::exeWait() {
    if (al::isFirstStep(this)) mHasPlayedDecideSound = false;
    mButtons->updateAndCursorDefault(mPort);
    lockButtonEachOther();
    if (mButtons->isDecideAny() && !mHasPlayedDecideSound) {
        al::tryStartSe(this, "Decided", nullptr);
        mHasPlayedDecideSound = true;
    }
    if (mButtons->isDecideEndAny()) {
        mDecideCourseIndex = findDecideCourseIndex();
        al::setNerve(this, &NrvShowCourseSelectDecide);
    }
}
/** @brief Locks all buttons once any one is confirmed. */
void ShowCourseSelect::lockButtonEachOther() {
    if (mButtons->isDecideAny()) { mButtons->hideCursor(); mButtons->invalidate(); }
}
/** @brief Finds the completed course choice among the five show stages. */
int ShowCourseSelect::findDecideCourseIndex() const {
    if (mButtons->isDecideEnd("ステージ1")) return 0;
    if (mButtons->isDecideEnd("ステージ2")) return 1;
    if (mButtons->isDecideEnd("ステージ3")) return 2;
    if (mButtons->isDecideEnd("ステージ4")) return 3;
    if (mButtons->isDecideEnd("ステージ5")) return 4;
    return -1;
}
/** @brief Starts the exit animation for either completion state. */
void ShowCourseSelect::exeEnd() { if (al::isFirstStep(this)) al::startAction(this, "End", nullptr); }

#include "Layout/WindowMiiverseDetail.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/CursorTarget.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/InputUtil.hpp"
namespace {
NERVE_DECL(WindowMiiverseDetail, Wait);
NERVE_DECL(WindowMiiverseDetail, Appear);
NERVE_DECL(WindowMiiverseDetail, Disappear);
NERVE_DECL(WindowMiiverseDetail, Decided);
NERVES_MAKE_NOSTRUCT(WindowMiiverseDetail, Wait, Appear, Disappear, Decided)
}
/** @brief Creates the details layout and action buttons. */
WindowMiiverseDetail::WindowMiiverseDetail(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("Miiverse詳細吹き出し"), mLineBreakCallback(this, &WindowMiiverseDetail::lineBreakCallback) {
    al::initLayoutActor(this, rInfo, "WindowMiiverseDetail", nullptr);
    initNerve(&NrvWindowMiiverseDetailWait, 0);
    mButtons = new ButtonGroup(rInfo, this, "WindowMiiverseDetail", nullptr, false);
    mButtons->setPort(mPort);
}
/** @brief Keeps the text renderer's default line-break behavior. */
bool WindowMiiverseDetail::lineBreakCallback(const sead::WSafeString& rText, u32 index) { return false; }
/** @brief Resets the buttons and begins opening the details. */
void WindowMiiverseDetail::startShow() {
    mButtons->reset();
    mButtons->hideCursor();
    mButtons->validate();
    mButtons->resetDestination("とじる", "そうだね", nullptr, nullptr, nullptr, true);
    al::startAction(this, "Appear", nullptr);
    al::setNerve(this, &NrvWindowMiiverseDetailAppear);
    appear();
}
/** @brief Begins closing the details. */
void WindowMiiverseDetail::endShow() { al::setNerve(this, &NrvWindowMiiverseDetailDisappear); }
/** @brief Restores interaction after an action, preserving an existing Yeah. */
void WindowMiiverseDetail::reset() {
    al::setNerve(this, &NrvWindowMiiverseDetailWait);
    mButtons->reset();
    mButtons->validate();
    if (mHasYeah) mButtons->getButton("そうだね")->disable();
}
/** @brief Selects the close button when opening finishes. */
void WindowMiiverseDetail::exeAppear() {
    if (al::isActionEnd(this, nullptr)) {
        mButtons->select("とじる");
        al::setNerve(this, &NrvWindowMiiverseDetailWait);
    }
}
/** @brief Handles navigation and confirmation while the window is open. */
void WindowMiiverseDetail::exeWait() {
    if (al::isFirstStep(this)) {
        mButtons->showCursorAppear();
        al::startAction(this, "Wait", nullptr);
    }
    mButtons->update();
    if (mButtons->isInputLocked()) return;
    if (rc::isPadTriggerUiUpByPort(mPort)) mButtons->tryMove(0);
    if (rc::isPadTriggerUiDownByPort(mPort)) mButtons->tryMove(1);
    if (rc::isPadTriggerUiLeftByPort(mPort)) mButtons->tryMove(2);
    if (rc::isPadTriggerUiRightByPort(mPort)) mButtons->tryMove(3);
    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        if (mButtons->isSelect("とじる")) mButtons->decide("とじる");
        if (mButtons->isSelect("そうだね")) mButtons->decide("そうだね");
        if (mButtons->isSelect("コメント")) mButtons->decide("コメント");
    }
    if (rc::isPadTriggerUiCancelByPort(mPort)) mButtons->decide("とじる");
    if (mButtons->isDecide("とじる") || mButtons->isDecide("そうだね") || mButtons->isDecide("コメント")) {
        mButtons->hideCursor();
        if (mButtons->isDecide("とじる")) al::startSe(this, "PgCancel", nullptr);
        else al::startSe(this, "PgDecided", nullptr);
        al::setNerve(this, &NrvWindowMiiverseDetailDecided);
    }
}
/** @brief Holds the confirmed action for its owner. */
void WindowMiiverseDetail::exeDecided() { al::isFirstStep(this); }
/** @brief Plays the closing animation and deactivates the window. */
void WindowMiiverseDetail::exeDisappear() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Reports any confirmed action. */
bool WindowMiiverseDetail::isDecidedAny() const { return al::isNerve(this, &NrvWindowMiiverseDetailDecided) && mButtons->isDecideAny(); }
/** @brief Reports confirmation of the Close action. */
bool WindowMiiverseDetail::isDecidedClose() const { return al::isNerve(this, &NrvWindowMiiverseDetailDecided) && mButtons->isDecide("とじる"); }
/** @brief Reports confirmation of the Yeah action. */
bool WindowMiiverseDetail::isDecidedYeah() const { return al::isNerve(this, &NrvWindowMiiverseDetailDecided) && mButtons->isDecide("そうだね"); }
/** @brief Reports confirmation of the Comment action. */
bool WindowMiiverseDetail::isDecidedComment() const { return al::isNerve(this, &NrvWindowMiiverseDetailDecided) && mButtons->isDecide("コメント"); }
/** @brief Disables button interaction. */
void WindowMiiverseDetail::invalidateButton() const { mButtons->invalidate(); }

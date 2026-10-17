#include "Layout/SimpleMenuLayout.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/InputUtil.hpp"
namespace {
NERVE_DECL(SimpleMenuLayout, Wait);
NERVE_DECL(SimpleMenuLayout, Appear);
NERVE_DECL(SimpleMenuLayout, Decided);
NERVE_DECL(SimpleMenuLayout, End);
NERVES_MAKE_NOSTRUCT(SimpleMenuLayout, Wait, Appear, Decided, End)
}
/** @brief Creates a hidden menu with an optional button group. */
SimpleMenuLayout::SimpleMenuLayout(const char* pName, const al::LayoutInitInfo& rInfo, u32 buttonNum,
                                 const char* pSuffix) : al::LayoutActor(pName) {
    al::initLayoutActor(this, rInfo, pName, pSuffix);
    initNerve(&NrvSimpleMenuLayoutWait, 0);
    mPort = al::getMainControllerPort();
    if (buttonNum) {
        mButtons = new ButtonGroup(rInfo, this, pName, nullptr, false);
        mButtons->setPort(mPort);
    }
    kill();
}
/** @brief Shows the menu for a controller and resets its input state. */
void SimpleMenuLayout::appear(s32 port) {
    al::LayoutActor::appear();
    mPort = port;
    if (mButtons) {
        mButtons->setPort(mPort);
        mButtons->reset();
        if (mButtons->getCursorParts()) mButtons->getCursorParts()->reset();
    }
    al::setNerve(this, &NrvSimpleMenuLayoutAppear);
}
/** @brief Tests for the first frame of a confirmed selection. */
bool SimpleMenuLayout::isDecided() {
    return al::isNerve(this, &NrvSimpleMenuLayoutDecided) && al::isFirstStep(this);
}
/** @brief Tests whether the selected button has finished its confirmation animation. */
bool SimpleMenuLayout::isDecideEnd() {
    if (!mButtons) return al::isNerve(this, &NrvSimpleMenuLayoutDecided);
    return al::isNerve(this, &NrvSimpleMenuLayoutDecided) && mDecidedButtonName;
}
/** @brief Tests whether the menu is exiting. */
bool SimpleMenuLayout::isEnding() { return al::isNerve(this, &NrvSimpleMenuLayoutEnd); }
/** @brief Starts the exit sequence. */
void SimpleMenuLayout::end() { al::setNerve(this, &NrvSimpleMenuLayoutEnd); }
/** @brief Restores input and optionally selects a named button. */
void SimpleMenuLayout::reset(const char* pButtonName) {
    if (mButtons) {
        mButtons->reset();
        mButtons->validate();
        mButtons->showCursor();
        if (pButtonName) mButtons->select(pButtonName);
    }
    mDecidedButtonName = nullptr;
    al::setNerve(this, &NrvSimpleMenuLayoutWait);
}
/** @brief Plays the entrance or restores the previous selection when reopening. */
void SimpleMenuLayout::exeAppear() {
    if (al::isFirstStep(this)) {
        if (mHasAppeared) {
            if (mButtons) {
                if (mDecidedButtonName) mButtons->select(mDecidedButtonName);
                else mButtons->select(mButtons->getButtonNum() ? mButtons->getButtonUnsafe(0) : nullptr);
                mDecidedButtonName = nullptr;
                mButtons->validate();
                mButtons->showCursor();
            }
            al::setNerve(this, &NrvSimpleMenuLayoutWait);
            return;
        }
        al::startAction(this, "Appear", nullptr);
        if (mButtons) mButtons->invalidate();
        mHasAppeared = true;
    }
    if (al::isActionEnd(this, nullptr)) {
        if (mButtons) {
            mDecidedButtonName = nullptr;
            mButtons->select(mButtons->getButtonNum() ? mButtons->getButtonUnsafe(0) : nullptr);
            mButtons->validate();
            mButtons->showCursor();
        }
        al::setNerve(this, &NrvSimpleMenuLayoutWait);
    }
}
/** @brief Handles cancellation and button selection. */
void SimpleMenuLayout::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (rc::isPadTriggerUiCancelByPort(mPort)) {
        mDecidedButtonName = nullptr;
        al::setNerve(this, &NrvSimpleMenuLayoutEnd);
    }
    if (mButtons) {
        mButtons->updateAndCursorDefault(mPort);
        if (mButtons->isDecideAny()) {
            mButtons->invalidate();
            if (!mButtons->getCursorParts()->isHide()) mButtons->hideCursor();
            al::setNerve(this, &NrvSimpleMenuLayoutDecided);
        }
    }
}
/** @brief Records the button whose confirmation animation has completed. */
void SimpleMenuLayout::exeDecided() { mDecidedButtonName = mButtons->getDecideEndButton(); }
/** @brief Hides the cursor and closes the menu after its exit animation. */
void SimpleMenuLayout::exeEnd() {
    if (al::isFirstStep(this)) {
        if (mButtons) mButtons->getCursorParts()->hide();
        al::startAction(this, "End", nullptr);
        mHasAppeared = false;
    }
    if (al::isActionEnd(this, nullptr)) kill();
}

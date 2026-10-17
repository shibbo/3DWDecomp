#include "Layout/ButtonTouch.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
const ButtonTouchParam sDefaultParam;
NERVE_DECL(ButtonTouch, Wait);
NERVE_DECL(ButtonTouch, Hide);
NERVE_DECL(ButtonTouch, Appear);
NERVE_DECL(ButtonTouch, End);
NERVE_DECL(ButtonTouch, Touch);
NERVE_DECL(ButtonTouch, Decide);
NERVE_DECL(ButtonTouch, DecideEnd);
NERVE_DECL(ButtonTouch, OutsideHold);
NERVES_MAKE_NOSTRUCT(ButtonTouch, Wait, Hide, Appear, End, Touch, Decide, DecideEnd, OutsideHold)
}
/** @brief Uses the standard button hit pane and action slot. */
ButtonTouchParam::ButtonTouchParam() : mHitPane("Hit"), mActionSlot("Main") {}
/** @brief Selects button panes. @param pHitPane Hit-test pane. @param pActionSlot Animation slot. */
ButtonTouchParam::ButtonTouchParam(const char* pHitPane, const char* pActionSlot)
    : mHitPane(pHitPane), mActionSlot(pActionSlot) {}
/** @brief Binds a touch state machine to a layout.
 * @param pName State machine name.
 * @param pActor Layout supplying panes, actions, and sound.
 * @param pParam Optional pane configuration. */
ButtonTouch::ButtonTouch(const char* pName, al::LayoutActor* pActor, const ButtonTouchParam* pParam)
    : al::NerveExecutor(pName), mLayout(pActor), mLayoutAction(pActor), mAudio(pActor), mParam(pParam) {
    if (!mParam) mParam = &sDefaultParam;
    initNerve(&NrvButtonTouchWait, 0);
}
/** @brief Advances the current state. */
void ButtonTouch::update() { updateNerve(); }
/** @brief Hides the layout root and enters the hidden state. */
void ButtonTouch::hidePane() {
    al::hidePaneRoot(mLayout);
    al::setNerve(this, &NrvButtonTouchHide);
}
/** @brief Shows the layout root if hidden. */
void ButtonTouch::showPane() {
    if (al::isHidePaneRoot(mLayout)) al::showPaneRoot(mLayout);
}
/** @brief Starts the appearance animation. */
void ButtonTouch::startAppear() { al::setNerve(this, &NrvButtonTouchAppear); }
/** @brief Starts the end animation. */
void ButtonTouch::startEnd() { al::setNerve(this, &NrvButtonTouchEnd); }
/** @brief Enables touch input. */
void ButtonTouch::setTouchEnable() { mIsValid = true; }
/** @brief Disables new touches. */
void ButtonTouch::setTouchDisable() { mIsValid = false; }
/** @brief Checks the touch-entry frame. @return Whether touch began on the preceding update. */
bool ButtonTouch::isTrigerTouch() const {
    return al::isNerve(this, &NrvButtonTouchTouch) && al::isStep(this, 1);
}
/** @brief Checks the selection-entry frame. @return Whether selection began on the preceding update. */
bool ButtonTouch::isTrigerDecide() const {
    return al::isNerve(this, &NrvButtonTouchDecide) && al::isStep(this, 1);
}
/** @brief Checks selection playback. @return Whether the selected animation is active. */
bool ButtonTouch::isDecide() const { return al::isNerve(this, &NrvButtonTouchDecide); }
/** @brief Checks selection completion. @return Whether the selection animation ended. */
bool ButtonTouch::isDecideEnd() const { return al::isNerve(this, &NrvButtonTouchDecideEnd); }
/** @brief Tests the current touch against the configured pane. @return Whether touch is inside. */
bool ButtonTouch::isTouchPane() const {
    sead::Vector2f pos;
    al::calcTouchLayoutPos(&pos, mTouchPort);
    return al::isContainPointPane(mLayout, mParam->mHitPane, pos);
}
/** @brief Checks an ongoing touch gesture. @return Whether touch is held inside or outside. */
bool ButtonTouch::isTouchHold() const {
    return al::isNerve(this, &NrvButtonTouchTouch) || al::isNerve(this, &NrvButtonTouchOutsideHold);
}
/** @brief Checks idle state. @return Whether the button is waiting. */
bool ButtonTouch::isWait() const { return al::isNerve(this, &NrvButtonTouchWait); }
/** @brief Resets to the waiting state. */
void ButtonTouch::reset() { al::setNerve(this, &NrvButtonTouchWait); }
/** @brief Starts selection playback. */
void ButtonTouch::startDecide() { al::setNerve(this, &NrvButtonTouchDecide); }
/** @brief Assigns controller and touch ports. @param port Controller port. */
void ButtonTouch::setPort(int port) {
    mPort = port;
    mTouchPort = rc::calcTouchPanelPortByPortNum(port);
}
/** @brief Shows the pane and plays its appearance animation. */
void ButtonTouch::exeAppear() {
    if (al::isFirstStep(this)) {
        showPane();
        al::startAction(mLayoutAction, "Appear", mParam->mActionSlot);
    }
    if (al::isActionEnd(mLayoutAction, mParam->mActionSlot)) al::setNerve(this, &NrvButtonTouchWait);
}
/** @brief Waits for a new valid touch. */
void ButtonTouch::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mLayoutAction, "Wait", mParam->mActionSlot);
    if (mIsValid && al::isPadTriggerTouch(mTouchPort) && isTouchPane())
        al::setNerve(this, &NrvButtonTouchTouch);
}
/** @brief Plays the end animation before hiding. */
void ButtonTouch::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(mLayoutAction, "End", mParam->mActionSlot);
    if (al::isActionEnd(mLayoutAction, mParam->mActionSlot)) al::setNerve(this, &NrvButtonTouchHide);
}
/** @brief Sets the hidden animation. */
void ButtonTouch::exeHide() {
    if (al::isFirstStep(this)) al::startAction(mLayoutAction, "Hide", mParam->mActionSlot);
}
/** @brief Tracks touch release and movement outside the pane. */
void ButtonTouch::exeTouch() {
    if (al::isFirstStep(this)) {
        al::startAction(mLayoutAction, "Touch", mParam->mActionSlot);
        if (mAudio) al::tryStartSe(mAudio, "ButtonTouched", nullptr);
    }
    if (al::isPadReleaseTouch(mTouchPort) && isTouchPane()) {
        al::setNerve(this, &NrvButtonTouchDecide);
        return;
    }
    if (al::isPadHoldTouch(mTouchPort)) {
        if (!isTouchPane()) al::setNerve(this, &NrvButtonTouchOutsideHold);
    } else al::setNerve(this, &NrvButtonTouchWait);
}
/** @brief Tracks a gesture outside the pane until reentry or release. */
void ButtonTouch::exeOutsideHold() {
    if (al::isFirstStep(this)) {
        al::startAction(mLayoutAction, "Wait", mParam->mActionSlot);
        if (mAudio) al::tryStartSe(mAudio, "ButtonGoOutside", nullptr);
    }
    if (al::isPadReleaseTouch(mTouchPort) && !isTouchPane()) {
        if (mAudio) al::tryStartSe(mAudio, "ButtonReleaseOutside", nullptr);
        al::setNerve(this, &NrvButtonTouchWait);
        return;
    }
    if (al::isPadHoldTouch(mTouchPort)) {
        if (isTouchPane()) al::setNerve(this, &NrvButtonTouchTouch);
    } else al::setNerve(this, &NrvButtonTouchWait);
}
/** @brief Plays selection feedback and records completion. */
void ButtonTouch::exeDecide() {
    if (al::isFirstStep(this)) {
        al::startAction(mLayoutAction, "Decide", mParam->mActionSlot);
        if (mAudio) al::tryStartSe(mAudio, "ButtonDecided", nullptr);
    }
    if (al::isActionEnd(mLayoutAction, mParam->mActionSlot)) al::setNerve(this, &NrvButtonTouchDecideEnd);
}
/** @brief Holds the completed selection state. */
void ButtonTouch::exeDecideEnd() {}

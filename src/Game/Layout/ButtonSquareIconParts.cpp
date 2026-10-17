#include "Layout/ButtonSquareIconParts.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/InputUtil.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(ButtonSquareIconParts, Wait);
NERVE_DECL(ButtonSquareIconParts, Touch);
NERVE_DECL(ButtonSquareIconParts, Decide);
NERVE_DECL(ButtonSquareIconParts, OutsideHold);
NERVE_DECL(ButtonSquareIconParts, Select);
NERVE_DECL(ButtonSquareIconParts, Disable);
NERVE_DECL(ButtonSquareIconParts, Hide);
NERVES_MAKE_NOSTRUCT(ButtonSquareIconParts, Wait, Touch, Decide, OutsideHold, Select, Disable, Hide)
}
/** @brief Creates an enabled square icon button with optional raw touch input. */
ButtonSquareIconParts::ButtonSquareIconParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                   const char* pPartsName, al::LayoutActor* pParent, bool isRawTouch)
    : CursorTarget(rInfo, pName, pPartsName, pParent), mIsRawTouch(isRawTouch) {
    initNerve(&NrvButtonSquareIconPartsWait, 0);
}
/** @brief Detects a new touch on an enabled button. */
void ButtonSquareIconParts::exeWait() {
    if (al::isFirstStep(this) && mIsWaitAnimationEnabled) al::startAction(this, "Wait", "Main");
    if (!mIsValid) return;
    int port = getTouchPort();
    if (mIsRawTouch ? rc::isRawPadTriggerTouch(5) : al::isPadTriggerTouch(port)) {
        sead::Vector2f pos;
        if (mIsRawTouch) rc::calcRawLayoutTouchPos(&pos, 5);
        else al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonSquareIconPartsTouch);
    }
}
/** @brief Animates cursor selection and detects a touch. */
void ButtonSquareIconParts::exeSelect() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Select", "Main");
        al::tryStartSe(this, "SelectStartPG", nullptr);
    }
    if (!mIsValid) return;
    int port = getTouchPort();
    if (mIsRawTouch ? rc::isRawPadTriggerTouch(5) : al::isPadTriggerTouch(port)) {
        sead::Vector2f pos;
        if (mIsRawTouch) rc::calcRawLayoutTouchPos(&pos, 5);
        else al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonSquareIconPartsTouch);
    }
}
/** @brief Accepts a release inside the button or tracks a drag outside. */
void ButtonSquareIconParts::exeTouch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", "Main");
        al::tryStartSe(this, "TouchStartPG", nullptr);
    }
    int port = getTouchPort();
    if (mIsRawTouch ? rc::isRawPadReleaseTouch(5) : al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        if (mIsRawTouch) rc::calcRawLayoutTouchPos(&pos, 5);
        else al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) {
            al::setNerve(this, &NrvButtonSquareIconPartsDecide);
            return;
        }
    }
    if (mIsRawTouch ? rc::isRawPadHoldTouch(5) : al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        if (mIsRawTouch) rc::calcRawLayoutTouchPos(&pos, 5);
        else al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonSquareIconPartsOutsideHold);
    }
}
/** @brief Cancels an outside release or restores a touch dragged back inside. */
void ButtonSquareIconParts::exeOutsideHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
        al::tryStartSe(this, "OutsideHoldStartPG", nullptr);
    }
    int port = getTouchPort();
    if (mIsRawTouch ? rc::isRawPadReleaseTouch(5) : al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        if (mIsRawTouch) rc::calcRawLayoutTouchPos(&pos, 5);
        else al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "Hit", pos)) {
            al::setNerve(this, &NrvButtonSquareIconPartsWait);
            return;
        }
    }
    if (mIsRawTouch ? rc::isRawPadHoldTouch(5) : al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        if (mIsRawTouch) rc::calcRawLayoutTouchPos(&pos, 5);
        else al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonSquareIconPartsTouch);
    }
}
/** @brief Selects the button unless its choice is already confirmed. */
void ButtonSquareIconParts::select() {
    if (!al::isNerve(this, &NrvButtonSquareIconPartsDecide)) al::setNerve(this, &NrvButtonSquareIconPartsSelect);
}
/** @brief Confirms the button. */
void ButtonSquareIconParts::decide() { al::setNerve(this, &NrvButtonSquareIconPartsDecide); }
/** @brief Returns to idle touch handling. */
void ButtonSquareIconParts::wait() { al::setNerve(this, &NrvButtonSquareIconPartsWait); }
/** @brief Restores a disabled button to idle. */
void ButtonSquareIconParts::enable() { if (isDisable()) al::setNerve(this, &NrvButtonSquareIconPartsWait); }
/** @brief Starts the disabled state when needed. */
void ButtonSquareIconParts::disable() { if (!al::isNerve(this, &NrvButtonSquareIconPartsDisable)) al::setNerve(this, &NrvButtonSquareIconPartsDisable); }
/** @brief Reports the disabled state. */
bool ButtonSquareIconParts::isDisable() const { return al::isNerve(this, &NrvButtonSquareIconPartsDisable) || al::isNerve(this, &NrvButtonSquareIconPartsHide); }
/** @brief Starts the confirmation animation. */
void ButtonSquareIconParts::exeDecide() { if (al::isFirstStep(this)) {
    al::startAction(this, "Decide", "Main");
    al::tryStartSe(this, "DecideStartPG", nullptr);
}  }
/** @brief Starts the disabled animation. */
void ButtonSquareIconParts::exeDisable() { if (al::isFirstStep(this)) al::startAction(this, "Disable", "Main"); }
/** @brief Starts the hidden animation. */
void ButtonSquareIconParts::exeHide() { if (al::isFirstStep(this)) {
    al::startAction(this, "Hide", "Main");
    al::tryStartSe(this, "HideStartPG", nullptr);
}  }
/** @brief Reports confirmation. */
bool ButtonSquareIconParts::isDecide() const { return al::isNerve(this, &NrvButtonSquareIconPartsDecide); }
/** @brief Reports completion after the confirmation animation has begun. */
bool ButtonSquareIconParts::isDecideEnd() const { return al::isNerve(this, &NrvButtonSquareIconPartsDecide) && al::isGreaterStep(this, 1) && al::isActionEnd(this, nullptr); }
/** @brief Reports an active touch, including a drag outside the button. */
bool ButtonSquareIconParts::isTouch() const { return al::isNerve(this, &NrvButtonSquareIconPartsTouch) || al::isNerve(this, &NrvButtonSquareIconPartsOutsideHold); }
/** @brief Reports whether idle touch input is accepted. */
bool ButtonSquareIconParts::isValid() const { return mIsValid; }
/** @brief Stops idle touch input. */
void ButtonSquareIconParts::invalidate() { mIsValid = false; }
/** @brief Enables idle touch input. */
void ButtonSquareIconParts::validate() { mIsValid = true; }
/** @brief This button does not consume upward navigation. */
bool ButtonSquareIconParts::up() { return false; }
/** @brief This button does not consume downward navigation. */
bool ButtonSquareIconParts::down() { return false; }
/** @brief This button does not consume leftward navigation. */
bool ButtonSquareIconParts::left() { return false; }
/** @brief This button does not consume rightward navigation. */
bool ButtonSquareIconParts::right() { return false; }

/** @brief Selects the hidden state. */
void ButtonSquareIconParts::hide() { al::setNerve(this, &NrvButtonSquareIconPartsHide); }

#include "Layout/ButtonConfigParts.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(ButtonConfigParts, Wait);
NERVE_DECL(ButtonConfigParts, Touch);
NERVE_DECL(ButtonConfigParts, Decide);
NERVE_DECL(ButtonConfigParts, OutsideHold);
NERVE_DECL(ButtonConfigParts, Select);
NERVE_DECL(ButtonConfigParts, Disable);
NERVES_MAKE_NOSTRUCT(ButtonConfigParts, Wait, Touch, Decide, OutsideHold, Select, Disable)
}
/** @brief Creates an enabled configuration button. */
ButtonConfigParts::ButtonConfigParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                   const char* pPartsName, al::LayoutActor* pParent)
    : CursorTarget(rInfo, pName, pPartsName, pParent) {
    initNerve(&NrvButtonConfigPartsWait, 0);
}
/** @brief Detects a new touch on an enabled button. */
void ButtonConfigParts::exeWait() {
    if (!mIsValid) return;
    int port = getTouchPort();
    if (al::isPadTriggerTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonConfigPartsTouch);
    }
}
/** @brief Animates cursor selection and detects a touch. */
void ButtonConfigParts::exeSelect() {
    if (al::isFirstStep(this)) al::startAction(this, "Select", "Main");
    int port = getTouchPort();
    if (al::isPadTriggerTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonConfigPartsTouch);
    }
}
/** @brief Accepts a release inside the button or tracks a drag outside. */
void ButtonConfigParts::exeTouch() {
    if (al::isFirstStep(this)) al::startAction(this, "Touch", "Main");
    int port = getTouchPort();
    if (al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) {
            al::setNerve(this, &NrvButtonConfigPartsDecide);
            return;
        }
    }
    if (al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonConfigPartsOutsideHold);
    }
}
/** @brief Cancels an outside release or restores a touch dragged back inside. */
void ButtonConfigParts::exeOutsideHold() {
    int port = getTouchPort();
    if (al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "Hit", pos)) {
            al::setNerve(this, &NrvButtonConfigPartsWait);
            return;
        }
    }
    if (al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "Hit", pos)) al::setNerve(this, &NrvButtonConfigPartsTouch);
    }
}
/** @brief Selects the button unless its choice is already confirmed. */
void ButtonConfigParts::select() {
    if (!al::isNerve(this, &NrvButtonConfigPartsDecide)) al::setNerve(this, &NrvButtonConfigPartsSelect);
}
/** @brief Confirms the button. */
void ButtonConfigParts::decide() { al::setNerve(this, &NrvButtonConfigPartsDecide); }
/** @brief Returns to idle touch handling. */
void ButtonConfigParts::wait() { al::setNerve(this, &NrvButtonConfigPartsWait); }
/** @brief Restores a disabled button to idle. */
void ButtonConfigParts::enable() { if (al::isNerve(this, &NrvButtonConfigPartsDisable)) al::setNerve(this, &NrvButtonConfigPartsWait); }
/** @brief Starts the disabled state when needed. */
void ButtonConfigParts::disable() { if (!al::isNerve(this, &NrvButtonConfigPartsDisable)) al::setNerve(this, &NrvButtonConfigPartsDisable); }
/** @brief Reports the disabled state. */
bool ButtonConfigParts::isDisable() const { return al::isNerve(this, &NrvButtonConfigPartsDisable); }
/** @brief Starts the confirmation animation. */
void ButtonConfigParts::exeDecide() { if (al::isFirstStep(this)) al::startAction(this, "Decide", "Main"); }
/** @brief Starts the disabled animation. */
void ButtonConfigParts::exeDisable() { if (al::isFirstStep(this)) al::startAction(this, "Disable", "Main"); }
/** @brief Starts the hidden animation. */
void ButtonConfigParts::exeHide() { if (al::isFirstStep(this)) al::startAction(this, "Hide", "Main"); }
/** @brief Reports confirmation. */
bool ButtonConfigParts::isDecide() const { return al::isNerve(this, &NrvButtonConfigPartsDecide); }
/** @brief Reports completion after the confirmation animation has begun. */
bool ButtonConfigParts::isDecideEnd() const { return al::isNerve(this, &NrvButtonConfigPartsDecide) && al::isGreaterStep(this, 1) && al::isActionEnd(this, nullptr); }
/** @brief Reports an active touch, including a drag outside the button. */
bool ButtonConfigParts::isTouch() const { return al::isNerve(this, &NrvButtonConfigPartsTouch) || al::isNerve(this, &NrvButtonConfigPartsOutsideHold); }
/** @brief Reports whether idle touch input is accepted. */
bool ButtonConfigParts::isValid() const { return mIsValid; }
/** @brief Stops idle touch input. */
void ButtonConfigParts::invalidate() { mIsValid = false; }
/** @brief Enables idle touch input. */
void ButtonConfigParts::validate() { mIsValid = true; }
/** @brief This button does not consume upward navigation. */
bool ButtonConfigParts::up() { return false; }
/** @brief This button does not consume downward navigation. */
bool ButtonConfigParts::down() { return false; }
/** @brief This button does not consume leftward navigation. */
bool ButtonConfigParts::left() { return false; }
/** @brief This button does not consume rightward navigation. */
bool ButtonConfigParts::right() { return false; }

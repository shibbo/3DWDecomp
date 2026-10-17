#include "Layout/ButtonShortCutParts.hpp"

#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ButtonShortCutParts, Wait);
NERVE_DECL(ButtonShortCutParts, Appear);
NERVE_DECL(ButtonShortCutParts, End);
NERVE_DECL(ButtonShortCutParts, Hide);
NERVE_DECL(ButtonShortCutParts, Decide);
NERVES_MAKE_NOSTRUCT(ButtonShortCutParts, Wait, Appear, End, Hide, Decide)
}

/** @brief Creates a shortcut layout and begins accepting its assigned button.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pLayoutName Layout archive name.
 * @param pParent Parent supplied by the parts interface.
 * @param port Controller port.
 * @param type Button assigned to the shortcut. */
ButtonShortCutParts::ButtonShortCutParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                       const char* pLayoutName, al::LayoutActor* pParent,
                                       int port, ShortCutType type)
    : al::LayoutActor(pName), mType(type), mPort(port) {
    al::initLayoutActor(this, rInfo, pLayoutName, nullptr);
    initNerve(&NrvButtonShortCutPartsWait, 0);
}

/** @brief Shows the shortcut with its appearance animation. */
void ButtonShortCutParts::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvButtonShortCutPartsAppear);
}
/** @brief Starts the end animation unless it is already active. */
void ButtonShortCutParts::end() {
    if (!al::isNerve(this, &NrvButtonShortCutPartsEnd))
        al::setNerve(this, &NrvButtonShortCutPartsEnd);
}
/** @brief Selects the hidden state. */
void ButtonShortCutParts::setHide() { al::setNerve(this, &NrvButtonShortCutPartsHide); }
/** @brief Checks whether the shortcut was activated.
 * @return Whether the decision state is active. */
bool ButtonShortCutParts::isDecide() const { return al::isNerve(this, &NrvButtonShortCutPartsDecide); }
/** @brief Checks whether the shortcut accepts input.
 * @return Whether the wait state is active. */
bool ButtonShortCutParts::isWait() const { return al::isNerve(this, &NrvButtonShortCutPartsWait); }
/** @brief Checks whether the end animation has finished.
 * @return Whether the end state and its animation are complete. */
bool ButtonShortCutParts::isEndAndActionEnd() const {
    return al::isNerve(this, &NrvButtonShortCutPartsEnd) && al::isActionEnd(this, nullptr);
}
/** @brief Checks whether the shortcut is hidden.
 * @return Whether the hidden state is active. */
bool ButtonShortCutParts::isHide() const { return al::isNerve(this, &NrvButtonShortCutPartsHide); }

/** @brief Plays the appearance animation before accepting input. */
void ButtonShortCutParts::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvButtonShortCutPartsWait);
}
/** @brief Checks the assigned shortcut button when input is enabled. */
void ButtonShortCutParts::exeWait() {
    if (!mIsInputEnable)
        return;
    switch (mType) {
    case Minus:
        if (al::isPadTriggerMinus(mPort))
            al::setNerve(this, &NrvButtonShortCutPartsDecide);
        break;
    case Plus:
        if (al::isPadTriggerPlus(mPort))
            al::setNerve(this, &NrvButtonShortCutPartsDecide);
        break;
    }
}
/** @brief Holds the selected state until its caller reacts. */
void ButtonShortCutParts::exeDecide() {}
/** @brief Starts the hidden animation on entry. */
void ButtonShortCutParts::exeHide() {
    if (al::isFirstStep(this))
        al::startAction(this, "Hide", nullptr);
}
/** @brief Starts the end animation on entry. */
void ButtonShortCutParts::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", nullptr);
}

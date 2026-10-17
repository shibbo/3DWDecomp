#include "Layout/ButtonArrowParts.hpp"

#include "Layout/ButtonTouch.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ButtonArrowParts, Hide);
NERVE_DECL(ButtonArrowParts, Wait);
NERVE_DECL(ButtonArrowParts, Decide);
NERVES_MAKE_NOSTRUCT(ButtonArrowParts, Hide, Wait, Decide)
ButtonTouchParam sTouchParam("Hit", "Button");
}  // namespace

/**
 * @brief Creates a hidden page-navigation button with touch input and layout audio.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout.
 */
ButtonArrowParts::ButtonArrowParts(const al::LayoutInitInfo& rInfo, const char* pName,
    const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvButtonArrowPartsHide, 0);
    mTouch = new ButtonTouch("ページ送りボタン", this, &sTouchParam);
    al::initLayoutPartsAudioKeeper(this, rInfo, "ButtonArrowParts");
}

/** @brief Updates touch input and resets the controller after a completed decision. */
void ButtonArrowParts::exeWait() {
    if (al::isFirstStep(this)) {
        mTouch->reset();
    }
    mTouch->update();
    if (mTouch->isDecideEnd()) {
        mTouch->reset();
    }
}

/** @brief Plays the decision action and returns to waiting when it finishes. */
void ButtonArrowParts::exeDecide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Decide", "Button");
    }
    if (al::isActionEnd(this, "Button")) {
        al::setNerve(this, &NrvButtonArrowPartsWait);
    }
}

/** @brief Starts the hidden action on the button slot. */
void ButtonArrowParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Button");
    }
}

/** @brief Checks for a newly confirmed touch decision.
 * @return Whether the touch controller reports a decision trigger. */
bool ButtonArrowParts::isTrigerDecide() const { return mTouch->isTrigerDecide(); }

/** @brief Starts the decision animation. */
void ButtonArrowParts::startDecide() { al::setNerve(this, &NrvButtonArrowPartsDecide); }

/** @brief Checks whether the button is in its hidden state.
 * @return Whether the Hide state is active. */
bool ButtonArrowParts::isHide() const { return al::isNerve(this, &NrvButtonArrowPartsHide); }

/** @brief Switches to the hidden state. */
void ButtonArrowParts::setHide() { al::setNerve(this, &NrvButtonArrowPartsHide); }

/** @brief Enables the waiting state. */
void ButtonArrowParts::setShow() { al::setNerve(this, &NrvButtonArrowPartsWait); }

/** @brief Assigns a controller to the touch-input handler.
 * @param port Controller port. */
void ButtonArrowParts::setPort(int port) { mTouch->setPort(port); }

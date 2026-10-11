#include "Layout/ButtonTextScrollParts.hpp"

#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
NERVE_DECL(ButtonTextScrollParts, Wait);
NERVE_DECL(ButtonTextScrollParts, TouchRight);
NERVE_DECL(ButtonTextScrollParts, TouchLeft);
NERVE_DECL(ButtonTextScrollParts, TouchCenter);
NERVE_DECL(ButtonTextScrollParts, TextScrollOut);
NERVE_DECL(ButtonTextScrollParts, OutsideHold);
NERVE_DECL(ButtonTextScrollParts, TextScrollIn);
NERVE_DECL(ButtonTextScrollParts, Select);
NERVE_DECL(ButtonTextScrollParts, Decide);
NERVE_DECL(ButtonTextScrollParts, Disable);
NERVE_DECL(ButtonTextScrollParts, Hide);
NERVES_MAKE_NOSTRUCT(ButtonTextScrollParts, Wait, TouchRight, TouchLeft, TouchCenter,
                     TextScrollOut, OutsideHold, TextScrollIn, Select, Decide, Disable, Hide)
}  // namespace

/**
 * @brief Creates the scroll button as parts of its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor; also used to look up the label messages.
 * @param pMessageArchive System message archive holding the labels.
 * @param pLabels Message labels to scroll through.
 * @param labelNum Number of entries in pLabels.
 */
ButtonTextScrollParts::ButtonTextScrollParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                             const char* pPartsName, al::LayoutActor* pParent,
                                             const char* pMessageArchive, const char** pLabels,
                                             s32 labelNum)
    : CursorTarget(rInfo, pName, pPartsName, pParent), mMessageLayout(pParent),
      mMessageArchive(pMessageArchive), mLabels(pLabels), mLabelNum(labelNum) {
    initNerve(&NrvButtonTextScrollPartsWait, 0);
    al::initLayoutPartsAudioKeeper(this, rInfo, "ButtonTextScrollParts");
}

/** @brief Per-frame update; all logic lives in the nerves. */
void ButtonTextScrollParts::control() {}

/** @brief Idle state; a touch on one of the hit panes starts the matching touch state. */
void ButtonTextScrollParts::exeWait() {
    if (al::isFirstStep(this)) {
        mIsSelectStarted = false;
        al::startAction(this, "Wait", "Main");
    }

    if (!mIsValid) {
        return;
    }

    s32 port = getTouchPort();
    if (!al::isPadTriggerTouch(port)) {
        return;
    }

    sead::Vector2f pos;
    al::calcTouchLayoutPos(&pos, port);
    if (al::isContainPointPane(this, "HitR", pos)) {
        al::setNerve(this, &NrvButtonTextScrollPartsTouchRight);
    } else if (al::isContainPointPane(this, "HitL", pos)) {
        al::setNerve(this, &NrvButtonTextScrollPartsTouchLeft);
    } else if (al::isContainPointPane(this, "HitC", pos)) {
        al::setNerve(this, &NrvButtonTextScrollPartsTouchCenter);
    }
}

/** @brief Selected state; plays the select animation once and accepts touches. */
void ButtonTextScrollParts::exeSelect() {
    if (al::isFirstStep(this) && !mIsSelectStarted) {
        al::startAction(this, "Select", "Main");
        mIsSelectStarted = true;
    }

    s32 port = getTouchPort();
    if (!al::isPadTriggerTouch(port)) {
        return;
    }

    sead::Vector2f pos;
    al::calcTouchLayoutPos(&pos, port);
    if (al::isContainPointPane(this, "HitR", pos)) {
        al::setNerve(this, &NrvButtonTextScrollPartsTouchRight);
    } else if (al::isContainPointPane(this, "HitL", pos)) {
        al::setNerve(this, &NrvButtonTextScrollPartsTouchLeft);
    } else if (al::isContainPointPane(this, "HitC", pos)) {
        al::setNerve(this, &NrvButtonTextScrollPartsTouchCenter);
    }
}

/**
 * @brief Center pane touched; releasing on an arrow scrolls, releasing on the center decides
 * and dragging off every hit pane switches to the outside-hold state.
 */
void ButtonTextScrollParts::exeTouchCenter() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", "Main");
        mIsSelectStarted = true;
    }

    s32 port = getTouchPort();
    if (al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "HitR", pos)) {
            mIsScrollRight = true;
            al::startAction(this, "Selected", "Main");
            al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
            return;
        }

        if (al::isContainPointPane(this, "HitL", pos)) {
            mIsScrollRight = false;
            al::startAction(this, "Selected", "Main");
            al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
            return;
        }

        if (al::isContainPointPane(this, "HitC", pos)) {
            al::startAction(this, "Selected", "Main");
            select();
            return;
        }
    }

    if (al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "HitC", pos) &&
            !al::isContainPointPane(this, "HitL", pos) &&
            !al::isContainPointPane(this, "HitR", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsOutsideHold);
        }
    }
}

/**
 * @brief Right arrow touched; releasing on an arrow scrolls, releasing on the center decides
 * and dragging off every hit pane switches to the outside-hold state.
 */
void ButtonTextScrollParts::exeTouchRight() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", "ButtonR");
        al::startAction(this, "Touch", "Main");
        mIsSelectStarted = true;
    }

    s32 port = getTouchPort();
    if (al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "HitR", pos)) {
            mIsScrollRight = true;
            al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
            al::startAction(this, "Selected", "Main");
            return;
        }

        if (al::isContainPointPane(this, "HitL", pos)) {
            mIsScrollRight = false;
            al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
            al::startAction(this, "Selected", "Main");
            return;
        }

        if (al::isContainPointPane(this, "HitC", pos)) {
            select();
            al::startAction(this, "Selected", "Main");
            return;
        }
    }

    if (al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "HitC", pos) &&
            !al::isContainPointPane(this, "HitL", pos) &&
            !al::isContainPointPane(this, "HitR", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsOutsideHold);
        }
    }
}

/**
 * @brief Left arrow touched; releasing on an arrow scrolls, releasing on the center decides
 * and dragging off every hit pane switches to the outside-hold state.
 */
void ButtonTextScrollParts::exeTouchLeft() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", "ButtonL");
        al::startAction(this, "Touch", "Main");
        mIsSelectStarted = true;
    }

    s32 port = getTouchPort();
    if (al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "HitL", pos)) {
            mIsScrollRight = false;
            al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
            al::startAction(this, "Selected", "Main");
            return;
        }

        if (al::isContainPointPane(this, "HitR", pos)) {
            mIsScrollRight = true;
            al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
            al::startAction(this, "Selected", "Main");
            return;
        }

        if (al::isContainPointPane(this, "HitC", pos)) {
            select();
            al::startAction(this, "Selected", "Main");
            return;
        }
    }

    if (al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "HitC", pos) &&
            !al::isContainPointPane(this, "HitL", pos) &&
            !al::isContainPointPane(this, "HitR", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsOutsideHold);
        }
    }
}

/**
 * @brief Touch held outside every hit pane; releasing cancels, moving back onto a pane resumes
 * the matching touch state.
 */
void ButtonTextScrollParts::exeOutsideHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
        mIsSelectStarted = false;
    }

    s32 port = getTouchPort();
    if (al::isPadReleaseTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (!al::isContainPointPane(this, "HitC", pos) &&
            !al::isContainPointPane(this, "HitR", pos) &&
            !al::isContainPointPane(this, "HitL", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsWait);
            return;
        }
    }

    if (al::isPadHoldTouch(port)) {
        sead::Vector2f pos;
        al::calcTouchLayoutPos(&pos, port);
        if (al::isContainPointPane(this, "HitC", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsTouchCenter);
        } else if (al::isContainPointPane(this, "HitR", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsTouchRight);
        } else if (al::isContainPointPane(this, "HitL", pos)) {
            al::setNerve(this, &NrvButtonTextScrollPartsTouchLeft);
        }
    }
}

/** @brief Advances the label index and plays the text scroll-out animation. */
void ButtonTextScrollParts::exeTextScrollOut() {
    if (al::isFirstStep(this)) {
        if (mIsScrollRight) {
            mLabelIdx = mLabelIdx + 1 < mLabelNum ? mLabelIdx + 1 : 0;
            al::startAction(this, "RightOut", "Text");
            al::startAction(this, "Decide", "ButtonR");
        } else {
            mLabelIdx--;
            if (mLabelIdx < 0) {
                mLabelIdx = mLabelNum - 1;
            }

            al::startAction(this, "LeftOut", "Text");
            al::startAction(this, "Decide", "ButtonL");
        }

        al::tryStartSe(this, "MenuSrcoll");
    }

    if (al::isActionEnd(this, "Text")) {
        al::setNerve(this, &NrvButtonTextScrollPartsTextScrollIn);
    }
}

/** @brief Shows the new label and plays the text scroll-in animation. */
void ButtonTextScrollParts::exeTextScrollIn() {
    if (al::isFirstStep(this)) {
        if (mIsScrollRight) {
            al::startAction(this, "RightIn", "Text");
        } else {
            al::startAction(this, "LeftIn", "Text");
        }

        const char16_t* pText =
            al::getSystemMessageString(mMessageLayout, mMessageArchive, mLabels[mLabelIdx]);
        al::setPaneString(this, "TxtSettingType", pText);
    }

    if (al::isActionEnd(this, "Text")) {
        al::setNerve(this, &NrvButtonTextScrollPartsSelect);
    }
}

/** @brief Selects the button unless it is decided or currently scrolling. */
void ButtonTextScrollParts::select() {
    if (al::isNerve(this, &NrvButtonTextScrollPartsDecide) ||
        al::isNerve(this, &NrvButtonTextScrollPartsTextScrollOut) ||
        al::isNerve(this, &NrvButtonTextScrollPartsTextScrollIn)) {
        return;
    }

    al::setNerve(this, &NrvButtonTextScrollPartsSelect);
}

/**
 * @brief Checks whether the button accepts cursor input.
 * @return False while decided or scrolling, true otherwise.
 */
bool ButtonTextScrollParts::isEnableControl() const {
    return !al::isNerve(this, &NrvButtonTextScrollPartsDecide) &&
           !al::isNerve(this, &NrvButtonTextScrollPartsTextScrollOut) &&
           !al::isNerve(this, &NrvButtonTextScrollPartsTextScrollIn);
}

/** @brief Decides the button. */
void ButtonTextScrollParts::decide() {
    al::setNerve(this, &NrvButtonTextScrollPartsDecide);
}

/** @brief Returns the button to its idle state. */
void ButtonTextScrollParts::wait() {
    al::setNerve(this, &NrvButtonTextScrollPartsWait);
}

/** @brief Re-enables a disabled button. */
void ButtonTextScrollParts::enable() {
    if (al::isNerve(this, &NrvButtonTextScrollPartsDisable)) {
        al::setNerve(this, &NrvButtonTextScrollPartsWait);
    }
}

/** @brief Disables the button. */
void ButtonTextScrollParts::disable() {
    if (al::isNerve(this, &NrvButtonTextScrollPartsDisable)) {
        return;
    }

    al::setNerve(this, &NrvButtonTextScrollPartsDisable);
}

/**
 * @brief Checks whether the button is disabled.
 * @return True while in the disable state.
 */
bool ButtonTextScrollParts::isDisable() const {
    return al::isNerve(this, &NrvButtonTextScrollPartsDisable);
}

/** @brief Decided state; plays the decide animation on the center button. */
void ButtonTextScrollParts::exeDecide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Decide", "ButtonC");
    }
}

/** @brief Disabled state; plays the disable animation. */
void ButtonTextScrollParts::exeDisable() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disable", "Main");
    }
}

/** @brief Hidden state; plays the hide animation. */
void ButtonTextScrollParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
    }
}

/**
 * @brief Checks whether the button has been decided.
 * @return True while in the decide state.
 */
bool ButtonTextScrollParts::isDecide() const {
    return al::isNerve(this, &NrvButtonTextScrollPartsDecide);
}

/**
 * @brief Checks whether the decide animation has finished.
 * @return True once the decide action has ended.
 */
bool ButtonTextScrollParts::isDecideEnd() const {
    return al::isNerve(this, &NrvButtonTextScrollPartsDecide) && al::isGreaterStep(this, 1) &&
           al::isActionEnd(this);
}

/**
 * @brief Checks whether the button is currently being touched.
 * @return True while any of the touch states is active.
 */
bool ButtonTextScrollParts::isTouch() const {
    return al::isNerve(this, &NrvButtonTextScrollPartsTouchCenter) ||
           al::isNerve(this, &NrvButtonTextScrollPartsTouchRight) ||
           al::isNerve(this, &NrvButtonTextScrollPartsTouchLeft) ||
           al::isNerve(this, &NrvButtonTextScrollPartsOutsideHold);
}

/**
 * @brief Scrolls to the previous label.
 * @return True if the scroll was started.
 */
bool ButtonTextScrollParts::left() {
    if (!isEnableControl()) {
        return false;
    }

    mIsScrollRight = false;
    al::tryStartSe(this, "SelectArrow");
    al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
    return true;
}

/**
 * @brief Scrolls to the next label.
 * @return True if the scroll was started.
 */
bool ButtonTextScrollParts::right() {
    if (!isEnableControl()) {
        return false;
    }

    mIsScrollRight = true;
    al::tryStartSe(this, "SelectArrow");
    al::setNerve(this, &NrvButtonTextScrollPartsTextScrollOut);
    return true;
}

/**
 * @brief Shows the label at the given index without animating.
 * @param index Index into the label list.
 */
void ButtonTextScrollParts::setLabelIdx(s32 index) {
    mLabelIdx = index;
    const char16_t* pText =
        al::getSystemMessageString(mMessageLayout, mMessageArchive, mLabels[mLabelIdx]);
    al::setPaneString(this, "TxtSettingType", pText);
}

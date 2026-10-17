#include "Layout/Switch/ButtonRCSFileParts.hpp"

#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/InputUtil.hpp"

namespace {
NERVE_DECL(ButtonRCSFileParts, Wait);
NERVE_DECL(ButtonRCSFileParts, Decide);
NERVE_DECL(ButtonRCSFileParts, Select);
NERVE_DECL(ButtonRCSFileParts, Disable);
NERVE_DECL(ButtonRCSFileParts, Hide);
NERVE_DECL(ButtonRCSFileParts, Touch);
NERVE_DECL(ButtonRCSFileParts, OutsideHold);
NERVES_MAKE_NOSTRUCT(ButtonRCSFileParts, Wait, Decide, Select, Disable, Hide, Touch, OutsideHold)

/** Port used for raw touch-panel input. */
constexpr s32 cTouchPort = 5;
}  // namespace

/**
 * @brief Checks whether the current touch position lies inside the button's hit pane.
 * @return True if the touch is inside the "Hit" pane.
 */
inline bool ButtonRCSFileParts::isTouchInHitPane() const {
    sead::Vector2f pos;
    al::calcTouchLayoutPos(&pos, cTouchPort);
    return al::isContainPointPane(this, "Hit", pos);
}

/**
 * @brief Creates the button as parts of its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor.
 */
ButtonRCSFileParts::ButtonRCSFileParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                       const char* pPartsName, al::LayoutActor* pParent)
    : CursorTarget(rInfo, pName, pPartsName, pParent) {
    initNerve(&NrvButtonRCSFilePartsWait, 0);
    al::initLayoutPartsAudioKeeper(this, rInfo, "ButtonRCSFileParts");
}

/** @brief Decides the button, or plays the invalid sound while it is disabled. */
void ButtonRCSFileParts::decide() {
    if (mIsDisabled) {
        al::tryStartSe(this, "Invalid");
    } else {
        al::setNerve(this, &NrvButtonRCSFilePartsDecide);
    }
}

/** @brief Selects the button unless it has already been decided. */
void ButtonRCSFileParts::select() {
    if (al::isNerve(this, &NrvButtonRCSFilePartsDecide)) {
        return;
    }

    al::setNerve(this, &NrvButtonRCSFilePartsSelect);
}

/** @brief Returns the button to its idle state (or the disabled state if disabled). */
void ButtonRCSFileParts::wait() {
    if (mIsDisabled) {
        al::setNerve(this, &NrvButtonRCSFilePartsDisable);
    } else {
        al::setNerve(this, &NrvButtonRCSFilePartsWait);
    }
}

/** @brief Re-enables a disabled or hidden button. */
void ButtonRCSFileParts::enable() {
    if (al::isNerve(this, &NrvButtonRCSFilePartsDisable) ||
        al::isNerve(this, &NrvButtonRCSFilePartsHide) || mIsDisabled) {
        mIsDisabled = false;
        al::setNerve(this, &NrvButtonRCSFilePartsWait);
    }
}

/** @brief Disables the button. */
void ButtonRCSFileParts::disable() {
    if (al::isNerve(this, &NrvButtonRCSFilePartsDisable)) {
        return;
    }

    al::setNerve(this, &NrvButtonRCSFilePartsDisable);
}

/**
 * @brief Checks whether the button is hidden.
 * @return True while in the hide state.
 */
bool ButtonRCSFileParts::isDisable() const {
    return al::isNerve(this, &NrvButtonRCSFilePartsHide);
}

/** @brief Suppresses the select sound for the next selection. */
void ButtonRCSFileParts::setNoSoundSelect() {
    mIsPlaySelectSe = false;
}

/**
 * @brief Checks whether the button has been decided.
 * @return True while in the decide state.
 */
bool ButtonRCSFileParts::isDecide() const {
    return al::isNerve(this, &NrvButtonRCSFilePartsDecide);
}

/**
 * @brief Checks whether the decide animation has finished.
 * @return True once the decide action has ended.
 */
bool ButtonRCSFileParts::isDecideEnd() const {
    return al::isNerve(this, &NrvButtonRCSFilePartsDecide) && al::isGreaterStep(this, 1) &&
           al::isActionEnd(this);
}

/**
 * @brief Checks whether the button is currently being touched.
 * @return True while touched (inside or outside the hit pane).
 */
bool ButtonRCSFileParts::isTouch() const {
    return al::isNerve(this, &NrvButtonRCSFilePartsTouch) ||
           al::isNerve(this, &NrvButtonRCSFilePartsOutsideHold);
}

/** @brief Idle state; a touch on the hit pane starts the touch state. */
void ButtonRCSFileParts::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
    }

    if (mIsValid && rc::isRawPadTriggerTouch(cTouchPort) && isTouchInHitPane()) {
        al::setNerve(this, &NrvButtonRCSFilePartsTouch);
    }
}

/** @brief Selected state; plays the select sound and accepts touches. */
void ButtonRCSFileParts::exeSelect() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Select", "Main");
        if (mIsPlaySelectSe) {
            al::tryStartSe(this, "Select");
        } else {
            mIsPlaySelectSe = true;
        }
    }

    if (mIsValid && rc::isRawPadTriggerTouch(cTouchPort) && isTouchInHitPane()) {
        al::setNerve(this, &NrvButtonRCSFilePartsTouch);
    }
}

/** @brief Touched state; releasing inside decides, dragging outside leaves the hit pane. */
void ButtonRCSFileParts::exeTouch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", "Main");
        al::tryStartSe(this, "ButtonTouched");
    }

    if (rc::isRawPadReleaseTouch(cTouchPort) && isTouchInHitPane()) {
        al::setNerve(this, &NrvButtonRCSFilePartsDecide);
        return;
    }

    if (rc::isRawPadHoldTouch(cTouchPort) && !isTouchInHitPane()) {
        al::setNerve(this, &NrvButtonRCSFilePartsOutsideHold);
    }
}

/** @brief Touch held outside the hit pane; releasing cancels, moving back resumes touching. */
void ButtonRCSFileParts::exeOutsideHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
        al::tryStartSe(this, "OutsideHold");
    }

    if (rc::isRawPadReleaseTouch(cTouchPort) && !isTouchInHitPane()) {
        al::setNerve(this, &NrvButtonRCSFilePartsWait);
        return;
    }

    if (rc::isRawPadHoldTouch(cTouchPort) && isTouchInHitPane()) {
        al::setNerve(this, &NrvButtonRCSFilePartsTouch);
    }
}

/** @brief Decided state; plays the decide animation and sound. */
void ButtonRCSFileParts::exeDecide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Decide", "Main");
        if (mCustomSE != nullptr) {
            al::tryStartSe(this, mCustomSE);
        } else {
            al::tryStartSe(this, "Decide");
        }
    }
}

/** @brief Disabled state; plays the disable animation. */
void ButtonRCSFileParts::exeDisable() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disable", "Main");
        mIsDisabled = true;
    }
}

/** @brief Hidden state; plays the hide animation. */
void ButtonRCSFileParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
    }
}

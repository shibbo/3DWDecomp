#include "Layout/Switch/ChallengeTimerParts.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ChallengeTimerParts, End);
NERVE_DECL(ChallengeTimerParts, CountDown);
NERVE_DECL(ChallengeTimerParts, Hide);
NERVE_DECL(ChallengeTimerParts, Appear);
NERVE_DECL(ChallengeTimerParts, Display);
NERVES_MAKE_NOSTRUCT(ChallengeTimerParts, End, CountDown, Hide, Appear, Display)

/** Frames per displayed second. */
constexpr s32 cFramesPerSecond = 60;
/** Up to this many frames left, the timer only needs a single digit. */
constexpr s32 cSingleDigitFrameMax = 600;
/** Up to this count, DisplayCounter only needs a single digit. */
constexpr s32 cSingleDigitCountMax = 9;
}  // namespace

/**
 * @brief Creates the timer as parts of its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor.
 */
ChallengeTimerParts::ChallengeTimerParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                         const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvChallengeTimerPartsEnd, 0);
    mTimerTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtTimer", 2);
    mTimerShadowTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtTimer_ds", 2);
}

/** @brief Re-applies the text box font fix-ups every frame. */
void ChallengeTimerParts::control() {
    mTimerTextInfo.applyFix();
    mTimerShadowTextInfo.applyFix();
}

/**
 * @brief Writes the remaining seconds into the timer panes.
 * @param frames Remaining time in frames.
 */
inline void ChallengeTimerParts::updateTimerPanes(s32 frames) {
    s32 seconds = (frames - 1) / cFramesPerSecond;
    if (frames <= cSingleDigitFrameMax) {
        al::setPaneCounterDigit1(this, "TxtTimer", seconds, 0);
        al::setPaneCounterDigit1(this, "TxtTimer_ds", seconds, 0);
    } else {
        al::setPaneCounterDigit2(this, "TxtTimer", seconds, 0);
        al::setPaneCounterDigit2(this, "TxtTimer_ds", seconds, 0);
    }
}

/**
 * @brief Sets the remaining time and updates the displayed seconds.
 * @param frames Remaining time in frames.
 */
void ChallengeTimerParts::setTimer(s32 frames) {
    mTimeFrames = frames;
    updateTimerPanes(frames);
}

/**
 * @brief Shows a plain number in the timer panes.
 * @param count Number to show.
 */
void ChallengeTimerParts::DisplayCounter(s32 count) {
    if (count <= cSingleDigitCountMax) {
        al::setPaneCounterDigit1(this, "TxtTimer", count, 0);
        al::setPaneCounterDigit1(this, "TxtTimer_ds", count, 0);
    } else {
        al::setPaneCounterDigit2(this, "TxtTimer", count, 0);
        al::setPaneCounterDigit2(this, "TxtTimer_ds", count, 0);
    }
}

/** @brief Starts counting down. */
void ChallengeTimerParts::startTimer() {
    mIsPaused = false;
    al::setNerve(this, &NrvChallengeTimerPartsCountDown);
}

/** @brief Stops counting down and freezes the red blinking. */
void ChallengeTimerParts::pauseTimer() {
    if (al::isAnyActionPlaying(this, nullptr) && al::isActionPlaying(this, "ColorRed", nullptr)) {
        al::pauseAction(this, "Main");
    }

    mIsPaused = true;
}

/** @brief Resumes counting down and the red blinking. */
void ChallengeTimerParts::unpauseTimer() {
    if (al::isPausedAction(this, "ColorRed", nullptr)) {
        al::unpauseAction(this, "Main");
    }

    mIsPaused = false;
}

/** @brief Switches the timer to its red (running out) look once. */
void ChallengeTimerParts::setTimerRed() {
    if (mIsRed) {
        return;
    }

    al::startAction(this, "ColorRed", nullptr);
    mIsRed = true;
}

/** @brief Activates the parts and keeps them hidden until shown. */
void ChallengeTimerParts::appear() {
    al::LayoutActor::appear();
    if (al::isNerve(this, &NrvChallengeTimerPartsHide)) {
        return;
    }

    al::startAction(this, "ForceHide", nullptr);
    al::setNerve(this, &NrvChallengeTimerPartsHide);
}

/**
 * @brief Hides the timer.
 * @param isForce Whether to hide immediately instead of playing the hide animation.
 */
void ChallengeTimerParts::hide(bool isForce) {
    if (al::isNerve(this, &NrvChallengeTimerPartsHide)) {
        return;
    }

    if (isForce) {
        al::startAction(this, "ForceHide", nullptr);
    } else {
        al::startAction(this, "Hide", nullptr);
    }

    al::setNerve(this, &NrvChallengeTimerPartsHide);
}

/** @brief Shows the timer again (in white) if it is hidden or finished. */
void ChallengeTimerParts::show() {
    if (al::isNerve(this, &NrvChallengeTimerPartsHide) ||
        al::isNerve(this, &NrvChallengeTimerPartsEnd)) {
        mIsRed = false;
        al::startAction(this, "ColorWhite", nullptr);
        al::setNerve(this, &NrvChallengeTimerPartsAppear);
    }
}

/**
 * @brief Checks whether the timer is counting down.
 * @return Whether the count down state is active.
 */
bool ChallengeTimerParts::isCountingDown() const {
    return al::isNerve(this, &NrvChallengeTimerPartsCountDown);
}

/**
 * @brief Checks whether the timer is on screen.
 * @return Whether the timer is neither hidden nor finished.
 */
bool ChallengeTimerParts::isVisible() const {
    return !al::isNerve(this, &NrvChallengeTimerPartsHide) &&
           !al::isNerve(this, &NrvChallengeTimerPartsEnd);
}

/** @brief Shows the remaining time without counting down. */
void ChallengeTimerParts::displayTime() {
    al::setNerve(this, &NrvChallengeTimerPartsDisplay);
}

/** @brief Holds the count down while a demo plays. */
void ChallengeTimerParts::startDemo() {
    mIsDemo = true;
}

/** @brief Lets the count down continue after a demo. */
void ChallengeTimerParts::endDemo() {
    mIsDemo = false;
}

/** @brief Plays the show animation. */
void ChallengeTimerParts::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Show", nullptr);
    }
}

/** @brief Counts the remaining time down and finishes once it runs out. */
void ChallengeTimerParts::exeCountDown() {
    if (mTimeFrames <= 0) {
        al::setNerve(this, &NrvChallengeTimerPartsEnd);
    }

    if (!mIsPaused && !mIsDemo) {
        mTimeFrames--;
    }

    updateTimerPanes(mTimeFrames);
}

/** @brief Idles after the time ran out. */
void ChallengeTimerParts::exeEnd() {}

/** @brief Resets the timer panes to zero. */
void ChallengeTimerParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::setPaneCounterDigit1(this, "TxtTimer", 0, 0);
        al::setPaneCounterDigit1(this, "TxtTimer_ds", 0, 0);
    }
}

/** @brief Shows the timer and the remaining time without counting down. */
void ChallengeTimerParts::exeDisplay() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Show", nullptr);
    }

    updateTimerPanes(mTimeFrames);
}

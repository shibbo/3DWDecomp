#include "Layout/Switch/CounterTimerGate.hpp"
#include "MapObj/TimerGate.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
NERVE_DECL(CounterTimerGate, End);
NERVE_DECL(CounterTimerGate, CountDown);
NERVE_DECL(CounterTimerGate, Hide);
NERVE_DECL(CounterTimerGate, Appear);
NERVES_MAKE_NOSTRUCT(CounterTimerGate, End, CountDown, Hide, Appear)
}
/** @brief Creates the initially paused timer parts. */
CounterTimerGate::CounterTimerGate(const al::LayoutInitInfo& rInfo, const char* pName,
                                 const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvCounterTimerGateEnd, 0);
}
/** @brief Sets the remaining time and the red-warning threshold. */
void CounterTimerGate::setTimer(s32 frames, s32 warningFrames) {
    mFrames = frames;
    mWarningFrames = warningFrames;
    int seconds = (frames - 1) / 60;
    al::setPaneCounterDigit2(this, "TxtTimer", seconds, 0);
    al::setPaneCounterDigit2(this, "TxtTimer_ds", seconds, 0);
}
/** @brief Starts counting down. */
void CounterTimerGate::startTimer() {
    mIsPaused = false;
    al::setNerve(this, &NrvCounterTimerGateCountDown);
}
/** @brief Pauses the countdown and any running warning animation. */
void CounterTimerGate::pauseTimer() {
    if (al::isAnyActionPlaying(this, nullptr) && al::isActionPlaying(this, "ColorRed", nullptr))
        al::pauseAction(this, "Main");
    mIsPaused = true;
}
/** @brief Resumes the countdown and warning animation. */
void CounterTimerGate::unpauseTimer() {
    if (al::isPausedAction(this, "ColorRed", nullptr)) al::unpauseAction(this, "Main");
    mIsPaused = false;
}
/** @brief Starts the warning color animation once. */
void CounterTimerGate::setTimerRed() {
    if (mIsRed) return;
    al::startAction(this, "ColorRed", nullptr);
    mIsRed = true;
}
/** @brief Activates the layout in its hidden state. */
void CounterTimerGate::appear() {
    al::LayoutActor::appear();
    hide(true);
}
/** @brief Hides the countdown and resets the associated gate switch. */
void CounterTimerGate::hide(bool isForce) {
    if (al::isNerve(this, &NrvCounterTimerGateHide)) return;
    if (isForce) al::startAction(this, "ForceHide", nullptr);
    else al::startAction(this, "Hide", nullptr);
    if (mTimerGate) mTimerGate->resetSwitch();
    mTimerGate = nullptr;
    al::setNerve(this, &NrvCounterTimerGateHide);
}
/** @brief Restores white text and begins showing an inactive countdown. */
void CounterTimerGate::show() {
    if (al::isNerve(this, &NrvCounterTimerGateHide) || al::isNerve(this, &NrvCounterTimerGateEnd)) {
        mIsRed = false;
        al::startAction(this, "ColorWhite", nullptr);
        al::setNerve(this, &NrvCounterTimerGateAppear);
    }
}
/** @brief Clears the associated timer gate. */
void CounterTimerGate::resetTimerGate() { mTimerGate = nullptr; }
/** @brief Reports whether the countdown state is active. */
bool CounterTimerGate::isCountingDown() const { return al::isNerve(this, &NrvCounterTimerGateCountDown); }
/** @brief Reports whether the timer is in a visible state. */
bool CounterTimerGate::isVisible() const {
    if (al::isNerve(this, &NrvCounterTimerGateHide) || al::isNerve(this, &NrvCounterTimerGateEnd))
        return false;
    return true;
}
/** @brief Starts the showing animation. */
void CounterTimerGate::exeAppear() { if (al::isFirstStep(this)) al::startAction(this, "Show", nullptr); }
/** @brief Advances the countdown and handles warning and timeout sounds. */
void CounterTimerGate::exeCountDown() {
    if (mFrames <= 0) {
        if (mTimerGate) mTimerGate->resetSwitch();
        mTimerGate = nullptr;
        hide(false);
    }
    if (!mIsPaused) --mFrames;
    int seconds = (mFrames - 1) / 60;
    al::setPaneCounterDigit2(this, "TxtTimer", seconds, 0);
    al::setPaneCounterDigit2(this, "TxtTimer_ds", seconds, 0);
    if (mFrames <= 0) {
        if (mTimerGate) mTimerGate->resetSwitch();
        mTimerGate = nullptr;
        hide(false);
        al::stopAllSeFromUser(this, 0);
        al::startSe(this, "PgTimeUp", nullptr);
    } else if (mFrames == mWarningFrames) {
        setTimerRed();
        al::holdSe(this, "PgTimerFast", nullptr);
    } else al::holdSe(this, "PgTimerNormal", nullptr);
}
/** @brief Holds the initial inactive state. */
void CounterTimerGate::exeEnd() {}
/** @brief Holds the hidden state. */
void CounterTimerGate::exeHide() {}
/** @brief Associates the countdown with its timer gate. */
void CounterTimerGate::setTimerGate(TimerGate* pTimerGate) { mTimerGate = pTimerGate; }

#include "Layout/LockOnButton.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include <math/seadMathCalcCommon.h>
namespace {
NERVE_DECL(LockOnButton, End);
NERVE_DECL(LockOnButton, Appear);
class LockOnButtonNrvFinish : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<LockOnButton>()->exeEnd(); }
};
NERVE_DECL(LockOnButton, Wait);
NERVES_MAKE_NOSTRUCT(LockOnButton, End, Appear, Finish, Wait)
/** @brief Builds a twenty-segment charge meter.
 * @param pText Output buffer.
 * @param rate Filled proportion.
 * @return The formatted meter text. */
const char16_t* formatHoldMeter(sead::WBufferedSafeString* pText, float rate) {
    int count = sead::Mathi::min(static_cast<int>(rate * 20.0f), 20);
    pText->clear();
    int i = 0;
    for (; i < count; ++i) pText->append(u'Y');
    for (; i < 20; ++i) pText->append(u'Z');
    return pText->cstr();
}
}
/** @brief Creates the lock-on prompt and an empty meter. @param rInfo Layout initialization context. */
LockOnButton::LockOnButton(const al::LayoutInitInfo& rInfo) : al::LayoutActor("LockOnButton"), mSceneCameraInfo(rInfo.getSceneCameraInfo()) {
    al::initLayoutActor(this, rInfo, "LockOnButton", nullptr);
    al::setPaneString(this, "TxtCharge", formatHoldMeter(&mMeterText, 0.0f), 0, -1);
    initNerve(&NrvLockOnButtonEnd, 0);
}
/** @brief Shows the prompt and resets charge tracking. */
void LockOnButton::startAppear() {
    appear();
    mDisplayedHold = 0.0f;
    mHoldTime = 0.0f;
    al::setNerve(this, &NrvLockOnButtonAppear);
}
/** @brief Starts the ordinary exit. */
void LockOnButton::end() { al::setNerve(this, &NrvLockOnButtonEnd); }
/** @brief Starts the successful lock-on exit. */
void LockOnButton::startFinish() { al::setNerve(this, &NrvLockOnButtonFinish); }
/** @brief Updates the requested charge and immediately shows increases. @param time Hold proportion. */
void LockOnButton::setHoldTime(float time) {
    if (mDisplayedHold < time) mDisplayedHold = time;
    mHoldTime = time;
    al::setPaneString(this, "TxtCharge", formatHoldMeter(&mMeterText, mDisplayedHold), 0, -1);
}
/** @brief Smoothly decreases the displayed meter toward the requested charge. */
void LockOnButton::updateHoldMeter() {
    if (mDisplayedHold > mHoldTime) {
        mDisplayedHold -= 0.05f;
        if (mDisplayedHold < mHoldTime) mDisplayedHold = mHoldTime;
        al::setPaneString(this, "TxtCharge", formatHoldMeter(&mMeterText, mDisplayedHold), 0, -1);
    }
}
/** @brief Hides the lock-on prompt. */
void LockOnButton::kill() { al::LayoutActor::kill(); }
/** @brief Plays appearance while updating the meter. */
void LockOnButton::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    updateHoldMeter();
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvLockOnButtonWait);
}
/** @brief Updates the meter and starts the waiting animation when needed. */
void LockOnButton::exeWait() {
    updateHoldMeter();
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "Wait", nullptr))
        al::startAction(this, "Wait", nullptr);
}
/** @brief Plays the appropriate exit animation before hiding. */
void LockOnButton::exeEnd() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvLockOnButtonEnd)) al::startAction(this, "End", nullptr);
        else al::startAction(this, "Decide", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Checks completion of either exit. @return Whether an exit animation has finished. */
bool LockOnButton::isEnd() const {
    return (al::isNerve(this, &NrvLockOnButtonFinish) || al::isNerve(this, &NrvLockOnButtonEnd)) &&
        al::isGreaterEqualStep(this, 1) && al::isActionEnd(this, nullptr);
}
/** @brief Returns the camera context used by the prompt. */
al::SceneCameraInfo* LockOnButton::getSceneCameraInfo() const { return mSceneCameraInfo; }

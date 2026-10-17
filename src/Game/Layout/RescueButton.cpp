#include "Layout/RescueButton.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include <math/seadMathCalcCommon.h>
namespace {
NERVE_DECL(RescueButton, End);
NERVE_DECL(RescueButton, Appear);
class RescueButtonNrvFinish : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<RescueButton>()->exeEnd(); }
};
NERVE_DECL(RescueButton, Wait);
NERVES_MAKE_NOSTRUCT(RescueButton, End, Appear, Finish, Wait)
/** @brief Builds a twenty-segment charge meter.
 * @param pText Output buffer.
 * @param rate Filled proportion.
 * @return The formatted meter text. */
const char16_t* formatRescueHoldMeter(sead::WBufferedSafeString* pText, float rate) {
    int count = sead::Mathi::min(static_cast<int>(rate * 20.0f), 20);
    pText->clear();
    int i = 0;
    for (; i < count; ++i) pText->append(u'Y');
    for (; i < 20; ++i) pText->append(u'Z');
    return pText->cstr();
}
}
/** @brief Creates the rescue prompt and an empty meter. @param rInfo Layout initialization context. */
RescueButton::RescueButton(const al::LayoutInitInfo& rInfo) : al::LayoutActor("RescueButton") {
    al::initLayoutActor(this, rInfo, "RescueButton", nullptr);
    al::setPaneString(this, "TxtCharge", formatRescueHoldMeter(&mMeterText, 0.0f), 0, -1);
    initNerve(&NrvRescueButtonEnd, 0);
}
/** @brief Shows the prompt and resets charge tracking. */
void RescueButton::startAppear() {
    appear();
    mDisplayedHold = 0.0f;
    mHoldTime = 0.0f;
    al::setNerve(this, &NrvRescueButtonAppear);
}
/** @brief Starts the ordinary exit. */
void RescueButton::end() { al::setNerve(this, &NrvRescueButtonEnd); }
/** @brief Starts the successful rescue exit. */
void RescueButton::startFinish() { al::setNerve(this, &NrvRescueButtonFinish); }
/** @brief Updates the requested charge and immediately shows increases. @param time Hold proportion. */
void RescueButton::setHoldTime(float time) {
    if (mDisplayedHold < time) mDisplayedHold = time;
    mHoldTime = time;
    al::setPaneString(this, "TxtCharge", formatRescueHoldMeter(&mMeterText, mDisplayedHold), 0, -1);
}
/** @brief Smoothly decreases the displayed meter toward the requested charge. */
void RescueButton::updateHoldMeter() {
    if (mDisplayedHold > mHoldTime) {
        mDisplayedHold -= 0.05f;
        if (mDisplayedHold < mHoldTime) mDisplayedHold = mHoldTime;
        al::setPaneString(this, "TxtCharge", formatRescueHoldMeter(&mMeterText, mDisplayedHold), 0, -1);
    }
}
/** @brief Hides the rescue prompt. */
void RescueButton::kill() { al::LayoutActor::kill(); }
/** @brief Plays appearance while updating the meter. */
void RescueButton::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    updateHoldMeter();
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvRescueButtonWait);
}
/** @brief Updates the meter and starts the waiting animation when needed. */
void RescueButton::exeWait() {
    updateHoldMeter();
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "Wait", nullptr))
        al::startAction(this, "Wait", nullptr);
}
/** @brief Plays the appropriate exit animation before hiding. */
void RescueButton::exeEnd() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvRescueButtonEnd)) al::startAction(this, "End", nullptr);
        else al::startAction(this, "Decide", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Checks completion of either exit. @return Whether an exit animation has finished. */
bool RescueButton::isEnd() const {
    return (al::isNerve(this, &NrvRescueButtonFinish) || al::isNerve(this, &NrvRescueButtonEnd)) &&
        al::isGreaterEqualStep(this, 1) && al::isActionEnd(this, nullptr);
}
/** @brief Returns the camera context used by the prompt. */
al::SceneCameraInfo* RescueButton::getSceneCameraInfo() const { return mSceneCameraInfo; }

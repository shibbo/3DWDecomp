#include "Layout/CounterGreenStarAppend.hpp"
#include <prim/seadSafeString.h>
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(CounterGreenStarAppend, Appear);
NERVE_DECL(CounterGreenStarAppend, Wait);
NERVE_DECL(CounterGreenStarAppend, Add);
NERVE_DECL(CounterGreenStarAppend, End);
NERVES_MAKE_NOSTRUCT(CounterGreenStarAppend, Appear, Wait, Add, End)
}
/** @brief Creates the counter and prepares its text color correction. */
CounterGreenStarAppend::CounterGreenStarAppend(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("取得グリーンスターカウントレイアウト") {
    al::initLayoutActor(this, rInfo, "CounterGreenStarAppend", nullptr);
    initNerve(&NrvCounterGreenStarAppendAppear, 0);
    kill();
    mTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCounter", 2);
}
/** @brief Applies the text color correction each frame. */
void CounterGreenStarAppend::control() { mTextInfo.applyFix(); }
/** @brief Starts counting from the previous total to the new total. */
void CounterGreenStarAppend::show(s32 oldNum, s32 newNum) {
    mCount = oldNum;
    mTargetCount = newNum;
    al::setPaneString(this, "TxtCounter", sead::WFormatFixedSafeString<8>(u"%d", mCount).cstr(), 0, -1);
    al::startAction(this, "Appear", "Main");
    al::LayoutActor::appear();
    al::setNerve(this, &NrvCounterGreenStarAppendAppear);
}
/** @brief Waits for the entrance animation. */
void CounterGreenStarAppend::exeAppear() {
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterGreenStarAppendWait);
}
/** @brief Pauses before counting the newly collected stars. */
void CounterGreenStarAppend::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", "Main");
    if (al::isActionEnd(this, nullptr)) {
        if (mCount == mTargetCount) al::setNerve(this, &NrvCounterGreenStarAppendEnd);
        else al::setNerve(this, &NrvCounterGreenStarAppendAdd);
    }
}
/** @brief Adds one star per animation until the new total is reached. */
void CounterGreenStarAppend::exeAdd() {
    if (al::isFirstStep(this)) {
        ++mCount;
        al::setPaneString(this, "TxtCounter", sead::WFormatFixedSafeString<8>(u"%d", mCount).cstr(), 0, -1);
        al::startAction(this, "Add", "Main");
    }
    if (al::isActionEnd(this, nullptr)) {
        if (mCount < mTargetCount) al::setNerve(this, &NrvCounterGreenStarAppendAdd);
        else al::setNerve(this, &NrvCounterGreenStarAppendEnd);
    }
}
/** @brief Hides the counter after its exit animation. */
void CounterGreenStarAppend::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", "Main");
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Reports completion after the counter has disappeared. */
bool CounterGreenStarAppend::isEnd() const {
    return al::isNerve(this, &NrvCounterGreenStarAppendEnd) && !isAlive();
}

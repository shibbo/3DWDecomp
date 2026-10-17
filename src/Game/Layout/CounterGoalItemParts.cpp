#include "Layout/CounterGoalItemParts.hpp"
#include <prim/seadSafeString.h>
#include "Layout/SingleModeSceneLayout.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
namespace {
NERVE_DECL(CounterGoalItemParts, Wait);
NERVE_DECL(CounterGoalItemParts, Demo);
NERVE_DECL(CounterGoalItemParts, DemoWaitEnd);
NERVES_MAKE_NOSTRUCT(CounterGoalItemParts, Wait, Demo, DemoWaitEnd)
}
/** @brief Creates the shine counter and prepares its text color correction. */
CounterGoalItemParts::CounterGoalItemParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                         const char* pPartsName, SingleModeSceneLayout* pParent)
    : al::LayoutActor(pName), mParent(pParent) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvCounterGoalItemPartsWait, 0);
    mCount = SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(mParent));
    sead::WFormatFixedSafeString<6> text(u"%d", mCount);
    al::setPaneString(this, "TxtCounter", text.cstr(), 0, -1);
    al::setPaneString(this, "TxtCounter_ds", text.cstr(), 0, -1);
    mTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCounter", 2);
}
/** @brief Applies the text color correction each frame. */
void CounterGoalItemParts::control() { mTextInfo.applyFix(); }
/** @brief Defers counter updates during a demo. */
void CounterGoalItemParts::startDemo() { al::setNerve(this, &NrvCounterGoalItemPartsDemo); }
/** @brief Resumes the counter, optionally displaying the new total immediately. */
void CounterGoalItemParts::endDemo(bool isUpdate) {
    if (!al::isNerve(this, &NrvCounterGoalItemPartsDemo)) return;
    int count = SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(mParent));
    if (isUpdate) {
        mCount = count;
        sead::WFormatFixedSafeString<6> text(u"%d", mCount);
        al::setPaneString(this, "TxtCounter", text.cstr(), 0, -1);
        al::setPaneString(this, "TxtCounter_ds", text.cstr(), 0, -1);
        al::setNerve(this, &NrvCounterGoalItemPartsWait);
    } else {
        if (mCount != count) {
            mHasPendingAdd = true;
            mCount = count;
        }
        if (mHasPendingAdd) al::setNerve(this, &NrvCounterGoalItemPartsDemoWaitEnd);
        else al::setNerve(this, &NrvCounterGoalItemPartsWait);
    }
}
/** @brief Refreshes the displayed total on entry and detects collected shines. */
void CounterGoalItemParts::exeWait() {
    int count = SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(mParent));
    if (al::isFirstStep(this)) {
        mCount = count;
        sead::WFormatFixedSafeString<6> text(u"%d", mCount);
        al::setPaneString(this, "TxtCounter", text.cstr(), 0, -1);
        al::setPaneString(this, "TxtCounter_ds", text.cstr(), 0, -1);
    }
    if (mCount != count) mHasPendingAdd = true;
}
/** @brief Holds the displayed count during a demo. */
void CounterGoalItemParts::exeDemo() {}
/** @brief Delays the shine collection effect until the demo has ended. */
void CounterGoalItemParts::exeDemoWaitEnd() {
    if (al::isFirstStep(this)) ++mCount;
    if (al::isGreaterEqualStep(this, 30)) {
        if (mHasPendingAdd) {
            al::startAction(this, "Add", "Main");
            al::emitEffect(mParent, "GoalItemAdd", nullptr);
            mParent->addShine();
            mHasPendingAdd = false;
        }
        al::setNerve(this, &NrvCounterGoalItemPartsWait);
    }
}


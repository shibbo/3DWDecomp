#include "Layout/CounterCoinParts.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include <prim/seadSafeString.h>
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/SingleModeData.hpp"
namespace {
NERVE_DECL(CounterCoinParts, Wait);
NERVE_DECL(CounterCoinParts, Miss);
NERVES_MAKE_NOSTRUCT(CounterCoinParts, Wait, Miss)
}
/** @brief Creates the coin counter and corrects both text panes. */
CounterCoinParts::CounterCoinParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                 const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName), mParent(pParent) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvCounterCoinPartsWait, 0);
    al::setPaneString(this, "TxtCounter", sead::WFormatFixedSafeString<6>(u"%s%02d", LayoutFontUtil::getIconFontCoin(), mCount).cstr(), 0, -1);
    al::setPaneString(this, "TxtCounter_ds", sead::WFormatFixedSafeString<6>(u"%s%02d", LayoutFontUtil::getIconFontCoin(), mCount).cstr(), 0, -1);
    mTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCounter", 2);
    mShadowTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCounter_ds", 2);
}
/** @brief Applies the text color corrections. */
void CounterCoinParts::control() {
    mTextInfo.applyFix();
    mShadowTextInfo.applyFix();
}
/** @brief Updates the total and animates changes after the first frame. */
void CounterCoinParts::exeWait() {
    int count = GameDataFunction::getCoinNum(GameDataHolderAccessor(mParent));
    if (mCount != count) {
        mCount = count;
        setCoinCount(count);
        if (!al::isFirstStep(this)) {
            al::startAction(this, "Add", "Main");
            al::emitEffect(mParent, "CoinAdd", nullptr);
        }
    }
}
/** @brief Writes the coin icon and count to both text panes. */
void CounterCoinParts::setCoinCount(s32 count) {
    al::setPaneString(this, "TxtCounter", sead::WFormatFixedSafeString<6>(u"%s%02d", LayoutFontUtil::getIconFontCoin(), count).cstr(), 0, -1);
    al::setPaneString(this, "TxtCounter_ds", sead::WFormatFixedSafeString<6>(u"%s%02d", LayoutFontUtil::getIconFontCoin(), count).cstr(), 0, -1);
    mCount = count;
}
/** @brief Holds the counter during the loss display. */
void CounterCoinParts::exeMiss() {}
/** @brief Stops automatic coin updates before displaying a loss. */
void CounterCoinParts::waitMiss() { al::setNerve(this, &NrvCounterCoinPartsMiss); }
/** @brief Deducts up to ten coins after a miss in Bowser's Fury. */
void CounterCoinParts::showMiss(const GameDataHolder* pHolder) {
    if (pHolder->isSingleMode()) {
        int count = pHolder->getSingleFile()->getCoinNum();
        int loss = count < 10 ? count : 10;
        mCount = count < 10 ? 0 : count - 10;
        pHolder->getSingleFile()->subtractCoin(loss);
        setCoinCount(mCount);
        al::startAction(this, "Miss", nullptr);
    }
}
/** @brief Removes the coin effect before a Bowser's Fury demo. */
void CounterCoinParts::startDemo() {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && mParent->getEffectKeeper())
        al::tryDeleteEffectAndParticle(mParent, "CoinAdd");
}
/** @brief No restoration is required after the demo. */
void CounterCoinParts::endDemo() {}

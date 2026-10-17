#include "Layout/CounterGreenStarParts.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include <prim/seadSafeString.h>
namespace {
NERVE_DECL(CounterGreenStarParts, Wait);
NERVES_MAKE_NOSTRUCT(CounterGreenStarParts, Wait)
}
/** @brief Creates the green-star counter and prepares both text layers.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPaneName Parts pane name.
 * @param pParent Parent HUD and source of game data.
 * @param pGreenStarKeeper Stage green-star collection. */
CounterGreenStarParts::CounterGreenStarParts(const al::LayoutInitInfo& rInfo, const char* pName,
    const char* pPaneName, al::LayoutActor* pParent, const GreenStarKeeper* pGreenStarKeeper)
    : al::LayoutActor(pName), mParent(pParent), mGreenStarKeeper(pGreenStarKeeper) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPaneName, nullptr);
    initNerve(&NrvCounterGreenStarPartsWait, 0);
    mTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCounter", 2);
    mBaseTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCounterBase", 2);
}
/** @brief Applies font correction to both counter layers. */
void CounterGreenStarParts::control() {
    mTextInfo.applyFix();
    mBaseTextInfo.applyFix();
}
/** @brief Initializes the icon rows and reacts to newly collected stars. */
void CounterGreenStarParts::exeWait() {
    if (al::isFirstStep(this)) {
        mAcquiredCount = mGreenStarKeeper->countAcquiredNum();
        sead::WFixedSafeString<256> text;
        for (int i = 0; i < mGreenStarKeeper->getStarCount(); ++i) {
            if (i != 0 && i % 5 == 0) text.append(sead::WSafeString::cLineBreakChar);
            text.append(LayoutFontUtil::getIconFontGreenStarBase());
        }
        al::setPaneString(this, "TxtCounterBase", text.cstr(), 0, 0);
        setPaneAcquiredGreenStar();
    }
    int count = mGreenStarKeeper->countAcquiredNum();
    int previousCount = mAcquiredCount;
    mAcquiredCount = count;
    if (previousCount < count) {
        al::startSeSetSeqLoacalVariableByName(mParent, "GreenStarGot", 0, count - 1);
        setPaneAcquiredGreenStar();
        if (mAcquiredCount == mGreenStarKeeper->getStarCount()) {
            if (!GameDataFunction::isStageGateKeeper(GameDataHolderAccessor(mParent),
                GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(mParent))))
                al::startSeSetSeqLoacalVariableByName(mParent, "GreenStarComplete", 0, 0);
            al::startAction(this, "AddComplete", "Main");
        } else al::startAction(this, "Add", "Main");
    }
}
/** @brief Updates filled and empty star icons from the current save data. */
void CounterGreenStarParts::setPaneAcquiredGreenStar() {
    sead::WFixedSafeString<256> text;
    for (int i = 0; i < mGreenStarKeeper->getStarCount(); ++i) {
        if (i != 0 && i % 5 == 0) text.append(sead::WSafeString::cLineBreakChar);
        text.append(GameDataFunction::isAcquireGreenStar(GameDataHolderAccessor(mParent), i)
            ? LayoutFontUtil::getIconFontGreenStar() : LayoutFontUtil::getIconFontGreenStarEmpty());
    }
    al::setPaneString(this, "TxtCounter", text.cstr(), 0, 0);
}

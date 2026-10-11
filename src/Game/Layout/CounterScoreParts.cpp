#include "Layout/CounterScoreParts.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "System/GameDataFunction.hpp"

namespace {
NERVE_DECL(CounterScoreParts, Wait);
NERVES_MAKE_NOSTRUCT(CounterScoreParts, Wait)
}  // namespace

/**
 * @brief Creates a score counter within its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor.
 * @param pGameDataHolder Game data supplying the total score.
 */
CounterScoreParts::CounterScoreParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                     const char* pPartsName, al::LayoutActor* pParent,
                                     const GameDataHolder* pGameDataHolder)
    : al::LayoutActor(pName), mGameDataHolder(pGameDataHolder) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvCounterScorePartsWait, 0);
    al::setPaneCounterDigit6(this, "TxtScore", 0, 0);
}

/** @brief Performs no additional work during appearance. */
void CounterScoreParts::exeAppear() {}

/** @brief Refreshes the score text and plays the add action when the score changes. */
void CounterScoreParts::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
        mScore = 0;
    }

    s32 score = GameDataFunction::getTotalScore(
        GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder)));
    if (mScore != score) {
        mScore = score;
        al::startAction(this, "Add", "Main");
        al::setPaneCounterDigit6(this, "TxtScore", mScore, 0);
    }
}

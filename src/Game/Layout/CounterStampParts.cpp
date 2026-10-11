#include "Layout/CounterStampParts.hpp"

#include "MapObj/IllustItemKeeper.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(CounterStampParts, Wait);
NERVE_DECL(CounterStampParts, Hide);
NERVE_DECL(CounterStampParts, Get);
NERVES_MAKE_NOSTRUCT(CounterStampParts, Wait, Hide, Get)
}  // namespace

/**
 * @brief Creates the stage stamp indicator in its current collection state.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout and game-data context.
 * @param pKeeper Stage stamp availability.
 */
CounterStampParts::CounterStampParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                     const char* pPartsName, al::LayoutActor* pParent,
                                     const IllustItemKeeper* pKeeper)
    : al::LayoutActor(pName), mParent(pParent), mKeeper(pKeeper) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    if (mKeeper->isDeclared() && GameDataFunction::isAcquireIllustItem(mParent)) {
        initNerve(&NrvCounterStampPartsWait, 0);
    } else {
        initNerve(&NrvCounterStampPartsHide, 0);
    }
}

/** @brief Hides the indicator until the stage stamp is collected. */
void CounterStampParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
    }
    if (mKeeper->isDeclared() && GameDataFunction::isAcquireIllustItem(mParent)) {
        al::setNerve(this, &NrvCounterStampPartsGet);
    }
}

/** @brief Plays the collection animation before entering the collected state. */
void CounterStampParts::exeGet() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Get", "Main");
    }
    if (al::isActionEnd(this, "Main")) {
        al::setNerve(this, &NrvCounterStampPartsWait);
    }
}

/** @brief Starts the collected stamp's wait animation. */
void CounterStampParts::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
    }
}

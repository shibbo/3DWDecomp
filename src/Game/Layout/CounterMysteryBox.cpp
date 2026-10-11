#include "Layout/CounterMysteryBox.hpp"

#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(CounterMysteryBox, Wait);
NERVES_MAKE_NOSTRUCT(CounterMysteryBox, Wait)
}  // namespace

/**
 * @brief Initializes the mystery box timer to ten.
 * @param rInfo Layout initialization context.
 */
CounterMysteryBox::CounterMysteryBox(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("ミステリーボックスカウンタ") {
    al::initLayoutActor(this, rInfo, "CounterMysteryBox", nullptr);
    initNerve(&NrvCounterMysteryBoxWait, 0);
    al::setPaneNumberDigit2(this, "TxtTimer", 10, 0);
}

/**
 * @brief Updates the two-digit timer text.
 * @param count Timer value to display.
 */
void CounterMysteryBox::setCount(s32 count) {
    al::setPaneNumberDigit2(this, "TxtTimer", count, 0);
}

/** @brief Waits for the owner to update the timer. */
void CounterMysteryBox::exeWait() {}

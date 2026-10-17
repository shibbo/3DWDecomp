#include "Course/ScenarioShineCounterParts.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "System/IslandDataList.hpp"

namespace {
NERVE_DECL(SingleModeCounterBase, Appear);
NERVES_MAKE_NOSTRUCT(SingleModeCounterBase, Appear)
}

/** @brief Binds both shine text layers to their layout-fix helpers. */
ScenarioShineCounterParts::ScenarioShineCounterParts(const al::LayoutInitInfo& info, const char* name,
                                                   const char* pane, al::LayoutActor* parent)
    : SingleModeCounterBase(info, name, pane, parent) {
    mShineText.setTextBox(getLayoutKeeper()->getLayout(), "TxtShine", 2);
    mBaseText.setTextBox(getLayoutKeeper()->getLayout(), "TxtShine_empty", 2);
}

/** @brief Applies the two shine text-layer layout fixes. */
void ScenarioShineCounterParts::control() {
    mShineText.applyFix();
    mBaseText.applyFix();
}

/** @brief Shows the counter, including the empty-counter appearance state. */
void ScenarioShineCounterParts::show() {
    if (mIsland == -1) al::setNerve(this, &NrvSingleModeCounterBaseAppear);
    else SingleModeCounterBase::show();
}

/** @brief Clears both text layers and requests the hide transition. */
void ScenarioShineCounterParts::forceHide() {
    al::setPaneString(this, "TxtShine", u"");
    al::setPaneString(this, "TxtShine_empty", u"");
    hide();
}

/** @brief Sets the island and freezes its counter at the end of the show action. */
void ScenarioShineCounterParts::forceEndAppear(int island) {
    mIsland = island;
    if (island < 0) { kill(); return; }
    appear();
    updateString(island);
    al::startFreezeActionEnd(this, "Show");
}

/** @brief Builds the shine and background glyph strings for the current scenario progress. */
void ScenarioShineCounterParts::updateString(int island) {
    int phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (SingleModeDataFunction::isPhase0(this) || phase == 7 || phase == 10 ||
        IslandDataFunction::isGigaBellIsland(island + 1)) {
        al::setPaneString(this, "TxtShine", u"");
        al::setPaneString(this, "TxtShine_empty", u"");
        return;
    }
    u64 flags = SingleModeDataFunction::getScenarioFlag(this, island);
    sead::WFixedSafeString<16> shine;
    sead::WFixedSafeString<16> base;
    int count = SingleModeDataFunction::getScenarioNum(this, island);
    int active = SingleModeDataFunction::getCurActiveScenarioIndex(this, island);
    for (int i = 0; i <= sead::Mathi::min(active, 2); ++i) {
        if (flags & (1ULL << i)) {
            shine.append(LayoutFontUtil::getIconFontCatShine());
            base.append(LayoutFontUtil::getIconFontCatShineBase());
        } else {
            shine.append(LayoutFontUtil::getIconFontCatShineEmpty());
            base.append(LayoutFontUtil::getIconFontCatShineBase());
            break;
        }
    }
    for (int i = 3; i < count; ++i) {
        if (flags & (1ULL << i)) {
            shine.append(LayoutFontUtil::getIconFontCatShine());
            base.append(LayoutFontUtil::getIconFontCatShineBase());
        } else if (i == active) {
            shine.append(LayoutFontUtil::getIconFontCatShineEmpty());
            base.append(LayoutFontUtil::getIconFontCatShineBase());
        }
    }
    al::setPaneString(this, "TxtShine", shine.cstr());
    al::setPaneString(this, "TxtShine_empty", base.cstr());
}

/** @brief Restores the counter after a demo, clearing text when no island is selected. */
void ScenarioShineCounterParts::endDemo() {
    appear();
    if (mIsland < 0) {
        al::setPaneString(this, "TxtShine", u"");
        al::setPaneString(this, "TxtShine_empty", u"");
    }
    show();
}
/** @brief Resets the selected island and kills the counter. */
void ScenarioShineCounterParts::handleIslandWarp() { mIsland = -1; kill(); }
/** @brief Refreshes the selected island or hides the counter when it is invalid. */
void ScenarioShineCounterParts::updateCount(int island) {
    mIsland = island;
    if (island >= 0) updateString(island);
    else hide();
}
/** @brief Refreshes a valid island counter and plays its add animation. */
void ScenarioShineCounterParts::addShine() {
    if (!isAlive()) appear();
    if (mIsland >= 0) {
        updateString(mIsland);
        al::startAction(this, "Add");
    }
}

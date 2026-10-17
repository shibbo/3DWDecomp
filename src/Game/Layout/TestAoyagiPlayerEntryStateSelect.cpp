#include "Layout/TestAoyagiPlayerEntryStateSelect.hpp"
#include "Layout/TestAoyagiPlayerEntryItem.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, AppearFromHide);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, AppearFromShow);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, Wait);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, Change);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, Decide);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, End);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, Hide);
NERVE_DECL(TestAoyagiPlayerEntryStateSelect, DecideEnd);
NERVES_MAKE_NOSTRUCT(TestAoyagiPlayerEntryStateSelect, AppearFromHide, AppearFromShow, Wait, Change, Decide, End, Hide, DecideEnd)
}
/** @brief Creates the character-selection state for its player panel. */
TestAoyagiPlayerEntryStateSelect::TestAoyagiPlayerEntryStateSelect(const char* pName,
        TestAoyagiPlayerEntryItem* pHost) : al::NerveStateBase(pName), mHost(pHost) {
    initNerve(&NrvTestAoyagiPlayerEntryStateSelectAppearFromHide, 0);
}
/** @brief Selects the entrance sequence for the panel's current visibility. */
void TestAoyagiPlayerEntryStateSelect::appear() {
    mHost->setColorEntry();
    if (mIsHidden) {
        al::startAction(mHost, "Hide", "Main");
        al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectAppearFromHide);
    } else al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectAppearFromShow);
    al::NerveStateBase::appear();
}
/** @brief Shows character selection from a hidden panel. */
void TestAoyagiPlayerEntryStateSelect::exeAppearFromHide() {
    if (al::isFirstStep(this)) al::startAction(mHost, "SelectAppear", "Main");
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectWait);
}
/** @brief Transitions the entry prompt into character selection. */
void TestAoyagiPlayerEntryStateSelect::exeAppearFromShow() {
    if (al::isFirstStep(this)) al::startAction(mHost, "EntryToSelect", "Main");
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectWait);
}
/** @brief Handles character changes, confirmation, and idle timeout. */
void TestAoyagiPlayerEntryStateSelect::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mHost, "SelectWait", "Main");
    if (al::isPadTriggerLeft(5)) {
        al::startAction(mHost, "CharacterLeft", "Main");
        mHost->decrementCharactorIndex();
        al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectChange);
    } else if (al::isPadTriggerRight(5)) {
        al::startAction(mHost, "CharacterRight", "Main");
        mHost->incrementCharactorIndex();
        al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectChange);
    } else if (al::isPadTriggerA(5)) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectDecide);
    else if (al::isGreaterEqualStep(this, 180)) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectEnd);
}
/** @brief Hides character selection after its exit animation. */
void TestAoyagiPlayerEntryStateSelect::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(mHost, "SelectEnd", "Main");
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectHide);
}
/** @brief Waits for confirmation input to show selection again. */
void TestAoyagiPlayerEntryStateSelect::exeHide() {
    if (al::isFirstStep(this)) al::startAction(mHost, "Hide", "Main");
    if (al::isPadTriggerA(5)) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectAppearFromHide);
}
/** @brief Waits for a character-change animation to finish. */
void TestAoyagiPlayerEntryStateSelect::exeChange() {
    al::isFirstStep(this);
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectWait);
}
/** @brief Displays the selected character before completing selection. */
void TestAoyagiPlayerEntryStateSelect::exeDecide() {
    if (al::isFirstStep(this)) {
        mHost->setColorCharactorModelCurrent();
        al::startAction(mHost, "CharacterDecide", "Main");
    }
    if (al::isGreaterEqualStep(this, 180)) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateSelectDecideEnd);
}
/** @brief Finishes the selection state after the play exit animation. */
void TestAoyagiPlayerEntryStateSelect::exeDecideEnd() {
    if (al::isFirstStep(this)) al::startAction(mHost, "PlayEnd", "Main");
    if (al::isActionEnd(mHost, "Main")) kill();
}

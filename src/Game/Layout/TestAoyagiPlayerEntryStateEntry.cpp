#include "Layout/TestAoyagiPlayerEntryStateEntry.hpp"
#include "Layout/TestAoyagiPlayerEntryItem.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestAoyagiPlayerEntryStateEntry, Appear);
NERVE_DECL(TestAoyagiPlayerEntryStateEntry, Wait);
NERVE_DECL(TestAoyagiPlayerEntryStateEntry, End);
NERVE_DECL(TestAoyagiPlayerEntryStateEntry, Hide);
NERVES_MAKE_NOSTRUCT(TestAoyagiPlayerEntryStateEntry, Appear, Wait, End, Hide)
}
/** @brief Creates the entry sequence for one player panel. */
TestAoyagiPlayerEntryStateEntry::TestAoyagiPlayerEntryStateEntry(const char* pName, TestAoyagiPlayerEntryItem* pHost)
    : al::NerveStateBase(pName), mHost(pHost) {
    initNerve(&NrvTestAoyagiPlayerEntryStateEntryAppear, 0);
}
/** @brief Sets the panel's entry color and begins the prompt sequence. */
void TestAoyagiPlayerEntryStateEntry::appear() {
    mHost->setColorEntry();
    al::startAction(mHost, "Hide", "Main");
    al::setNerve(this, &NrvTestAoyagiPlayerEntryStateEntryAppear);
    al::NerveStateBase::appear();
}
/** @brief Plays the entrance before starting the idle phase. */
void TestAoyagiPlayerEntryStateEntry::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(mHost, "EntryAppear", "Main");
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateEntryWait);
}
/** @brief Displays the idle phase for 180 frames. */
void TestAoyagiPlayerEntryStateEntry::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mHost, "EntryWait", "Main");
    if (tryEnd(false)) return;
    if (al::isGreaterEqualStep(this, 180)) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateEntryEnd);
}
/** @brief Plays the exit while still accepting a join input. */
void TestAoyagiPlayerEntryStateEntry::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(mHost, "EntryEnd", "Main");
    if (tryEnd(true)) return;
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStateEntryHide);
}

/** @brief Finishes the state if controller port 5 confirms entry. */
bool TestAoyagiPlayerEntryStateEntry::tryEnd(bool isHidden) {
    if (al::isPadTriggerA(5)) {
        mIsHidden = isHidden;
        kill();
        return true;
    }
    return false;
}
/** @brief Accepts a join input after the prompt has disappeared. */
void TestAoyagiPlayerEntryStateEntry::exeHide() { tryEnd(true); }

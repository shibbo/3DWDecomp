#include "Layout/TestAoyagiPlayerEntryStatePlay.hpp"
#include "Layout/TestAoyagiPlayerEntryItem.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestAoyagiPlayerEntryStatePlay, Appear);
NERVE_DECL(TestAoyagiPlayerEntryStatePlay, Wait);
NERVE_DECL(TestAoyagiPlayerEntryStatePlay, End);
NERVES_MAKE_NOSTRUCT(TestAoyagiPlayerEntryStatePlay, Appear, Wait, End)
}
/** @brief Creates the play sequence for one player panel. */
TestAoyagiPlayerEntryStatePlay::TestAoyagiPlayerEntryStatePlay(const char* pName, TestAoyagiPlayerEntryItem* pHost)
    : al::NerveStateBase(pName), mHost(pHost) {
    initNerve(&NrvTestAoyagiPlayerEntryStatePlayAppear, 0);
}
/** @brief Refreshes the panel's character styling and begins the play sequence. */
void TestAoyagiPlayerEntryStatePlay::appear() {
    mHost->setModelFontCurrentCurrent();
    mHost->setColorCharactorModelCurrent();
    al::startAction(mHost, "Hide", "Main");
    al::setNerve(this, &NrvTestAoyagiPlayerEntryStatePlayAppear);
    al::NerveStateBase::appear();
}
/** @brief Plays the entrance before starting the idle phase. */
void TestAoyagiPlayerEntryStatePlay::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(mHost, "PlayAppear", "Main");
    if (al::isActionEnd(mHost, "Main")) al::setNerve(this, &NrvTestAoyagiPlayerEntryStatePlayWait);
}
/** @brief Displays the idle phase for 180 frames. */
void TestAoyagiPlayerEntryStatePlay::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mHost, "PlayWait", "Main");
    if (al::isGreaterEqualStep(this, 180)) al::setNerve(this, &NrvTestAoyagiPlayerEntryStatePlayEnd);
}
/** @brief Plays the exit and finishes the state. */
void TestAoyagiPlayerEntryStatePlay::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(mHost, "PlayEnd", "Main");
    if (al::isActionEnd(mHost, "Main")) kill();
}

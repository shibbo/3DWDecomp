#include "Layout/TestAoyagiPlayerEntry.hpp"
#include "Layout/TestAoyagiPlayerEntryItem.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
const char* const sPartsNames[] = {"ParPlayerDRC", "ParPlayer1", "ParPlayer2", "ParPlayer3"};
const char* const sControllerNames[] = {"RemoconNum5", "RemoconNum1", "RemoconNum2", "RemoconNum3"};
NERVE_DECL(TestAoyagiPlayerEntry, Wait);
NERVES_MAKE_NOSTRUCT(TestAoyagiPlayerEntry, Wait)
}
/** @brief Creates four player-entry panels and assigns their controller animations.
 * @param rInfo Layout initialization context. */
TestAoyagiPlayerEntry::TestAoyagiPlayerEntry(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("プレーヤーエントリーテスト") {
    al::initLayoutActor(this, rInfo, "TestPlayerEntryMini", nullptr);
    mItemCount = 4;
    mItems = new TestAoyagiPlayerEntryItem*[mItemCount];
    for (int i = 0; i < mItemCount; i++) {
        mItems[i] = new TestAoyagiPlayerEntryItem(rInfo, "プレーヤーエントリー", sPartsNames[i], this);
        al::startAction(mItems[i], sControllerNames[i], "Controller");
        mItems[i]->setCharacterIndex(i);
    }
    initNerve(&NrvTestAoyagiPlayerEntryWait, 0);
}
/** @brief Appears using the standard layout behavior. */
void TestAoyagiPlayerEntry::appear() { al::LayoutActor::appear(); }
/** @brief Keeps the entry test idle. */
void TestAoyagiPlayerEntry::exeWait() {}

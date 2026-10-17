#include "Layout/TestAoyagiPlayerEntryItem.hpp"
#include "Layout/TestAoyagiPlayerEntryStatePlay.hpp"
#include "Layout/TestAoyagiPlayerEntryStateEntry.hpp"
#include "Layout/TestAoyagiPlayerEntryStateSelect.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataConst.hpp"
namespace {
NERVE_DECL(TestAoyagiPlayerEntryItem, Play);
NERVE_DECL(TestAoyagiPlayerEntryItem, Entry);
NERVE_DECL(TestAoyagiPlayerEntryItem, Select);
NERVES_MAKE_STRUCT(TestAoyagiPlayerEntryItem, Play, Entry, Select)
}
/** @brief Creates the panel and connects its play, entry, and selection states. */
TestAoyagiPlayerEntryItem::TestAoyagiPlayerEntryItem(const al::LayoutInitInfo& rInfo,
        const char* pName, const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvTestAoyagiPlayerEntryItem.Play, 8);
    mPlayState = new TestAoyagiPlayerEntryStatePlay("プレイ状態", this);
    mEntryState = new TestAoyagiPlayerEntryStateEntry("エントリー状態", this);
    mSelectState = new TestAoyagiPlayerEntryStateSelect("キャラ選択状態", this);
    al::initNerveState(this, mPlayState, &NrvTestAoyagiPlayerEntryItem.Play, "[state]プレイ");
    al::initNerveState(this, mEntryState, &NrvTestAoyagiPlayerEntryItem.Entry, "[state]エントリー");
    al::initNerveState(this, mSelectState, &NrvTestAoyagiPlayerEntryItem.Select, "[state]選択");
}
/** @brief Holds the unused idle state. */
void TestAoyagiPlayerEntryItem::exeWait() {}
/** @brief Starts the entry prompt when the play sequence ends. */
void TestAoyagiPlayerEntryItem::exePlay() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvTestAoyagiPlayerEntryItem.Entry);
}
/** @brief Passes the panel visibility to character selection. */
void TestAoyagiPlayerEntryItem::exeEntry() {
    if (al::updateNerveState(this)) {
        mSelectState->setHidden(mEntryState->isHidden());
        al::setNerve(this, &NrvTestAoyagiPlayerEntryItem.Select);
    }
}
/** @brief Returns to play when selection ends. */
void TestAoyagiPlayerEntryItem::exeSelect() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvTestAoyagiPlayerEntryItem.Play);
}
/** @brief Writes the selected character's icon to the current pane. */
void TestAoyagiPlayerEntryItem::setModelFontCurrent(int character) {
    al::setPaneString(this, "TxtCharacterL", sead::WFormatFixedSafeString<6>(u"%s", LayoutFontUtil::getPictureFontPlayer(character)).cstr(), 0, -1);
}
/** @brief Writes the previous character's icon to the outgoing pane. */
void TestAoyagiPlayerEntryItem::setModelFontPrev(int character) {
    al::setPaneString(this, "TxtCharacterR", sead::WFormatFixedSafeString<6>(u"%s", LayoutFontUtil::getPictureFontPlayer(character)).cstr(), 0, -1);
}
/** @brief Selects the color animation associated with a character. */
void TestAoyagiPlayerEntryItem::setColorCharactorModel(int character) {
    al::startAction(this, al::StringTmp<32>("Color%s", GameDataConst::getPlayerCharacterName(character)).cstr(), "Color");
}
/** @brief Selects the entry prompt's color. */
void TestAoyagiPlayerEntryItem::setColorEntry() { al::startAction(this, "ColorEntry", "Color"); }
/** @brief Moves to the previous character, wrapping through all five choices. */
void TestAoyagiPlayerEntryItem::decrementCharactorIndex() {
    int previous = mCharacterIndex;
    --mCharacterIndex;
    if (mCharacterIndex < 0) mCharacterIndex = 4;
    setModelFontPrev(previous);
    setModelFontCurrent(mCharacterIndex);
}
/** @brief Moves to the next character, wrapping through all five choices. */
void TestAoyagiPlayerEntryItem::incrementCharactorIndex() {
    int previous = mCharacterIndex;
    ++mCharacterIndex;
    if (mCharacterIndex >= 5) mCharacterIndex = 0;
    setModelFontPrev(previous);
    setModelFontCurrent(mCharacterIndex);
}
/** @brief Refreshes the icon for the selected character. */
void TestAoyagiPlayerEntryItem::setModelFontCurrentCurrent() { setModelFontCurrent(mCharacterIndex); }
/** @brief Refreshes the color for the selected character. */
void TestAoyagiPlayerEntryItem::setColorCharactorModelCurrent() { setColorCharactorModel(mCharacterIndex); }

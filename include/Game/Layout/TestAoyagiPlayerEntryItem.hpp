#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class TestAoyagiPlayerEntryStatePlay;
class TestAoyagiPlayerEntryStateEntry;
class TestAoyagiPlayerEntryStateSelect;
/** @brief Individual player panel in the original entry test. */
class TestAoyagiPlayerEntryItem : public al::LayoutActor {
public:
    TestAoyagiPlayerEntryItem(const al::LayoutInitInfo& rInfo, const char* pName,
                             const char* pPartsName, al::LayoutActor* pParent);
    void exeWait();
    void exePlay();
    void exeEntry();
    void exeSelect();
    void setModelFontCurrent(int character);
    void setModelFontPrev(int character);
    void setColorCharactorModel(int character);
    void setColorEntry();
    void decrementCharactorIndex();
    void incrementCharactorIndex();
    void setModelFontCurrentCurrent();
    void setColorCharactorModelCurrent();
    /** @brief Assigns the initial character selection. */
    void setCharacterIndex(int character) { mCharacterIndex = character; }
    /** @brief Selects the controller port for this panel. */
    void setPort(int port) { mPort = port; }
    /** @brief Returns the controller port assigned to this panel. */
    int getPort() const { return mPort; }
private:
    TestAoyagiPlayerEntryStatePlay* mPlayState = nullptr;
    TestAoyagiPlayerEntryStateEntry* mEntryState = nullptr;
    TestAoyagiPlayerEntryStateSelect* mSelectState = nullptr;
    int mCharacterIndex = 0;
    int mPort;
};
static_assert(sizeof(TestAoyagiPlayerEntryItem) == 0x148);

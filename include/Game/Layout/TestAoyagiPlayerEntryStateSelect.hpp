#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class TestAoyagiPlayerEntryItem;
/** @brief Handles character selection in the player-entry test. */
class TestAoyagiPlayerEntryStateSelect : public al::NerveStateBase {
public:
    TestAoyagiPlayerEntryStateSelect(const char* pName, TestAoyagiPlayerEntryItem* pHost);
    ~TestAoyagiPlayerEntryStateSelect() override = default;
    void appear() override;
    void exeAppearFromHide();
    void exeAppearFromShow();
    void exeWait();
    void exeEnd();
    void exeHide();
    void exeChange();
    void exeDecide();
    void exeDecideEnd();
    /** @brief Selects the entrance animation for a hidden or visible panel. */
    void setHidden(bool isHidden) { mIsHidden = isHidden; }
private:
    TestAoyagiPlayerEntryItem* mHost;
    bool mIsHidden = false;
};
static_assert(sizeof(TestAoyagiPlayerEntryStateSelect) == 0x28);

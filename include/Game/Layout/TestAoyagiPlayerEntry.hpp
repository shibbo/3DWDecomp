#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class TestAoyagiPlayerEntryItem;
/** @brief Four-player entry layout used by the original entry test. */
class TestAoyagiPlayerEntry : public al::LayoutActor {
public:
    explicit TestAoyagiPlayerEntry(const al::LayoutInitInfo& rInfo);
    void appear() override;
    void exeWait();
private:
    TestAoyagiPlayerEntryItem** mItems;
    int mItemCount;
};
static_assert(sizeof(TestAoyagiPlayerEntry) == 0x138);

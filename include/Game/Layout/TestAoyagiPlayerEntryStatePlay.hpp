#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class TestAoyagiPlayerEntryItem;
/** @brief Runs the play animation sequence of an entry test panel. */
class TestAoyagiPlayerEntryStatePlay : public al::NerveStateBase {
public:
    TestAoyagiPlayerEntryStatePlay(const char* pName, TestAoyagiPlayerEntryItem* pHost);
    ~TestAoyagiPlayerEntryStatePlay() override = default;
    void appear() override;
    void exeAppear();
    void exeWait();
    void exeEnd();
private:
    TestAoyagiPlayerEntryItem* mHost;
};
static_assert(sizeof(TestAoyagiPlayerEntryStatePlay) == 0x20);

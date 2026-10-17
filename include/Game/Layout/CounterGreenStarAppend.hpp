#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include "Layout/TextBoxTextInfo.hpp"

namespace al { class LayoutInitInfo; }
/** @brief Animates the green stars added after completing a course. */
class CounterGreenStarAppend : public al::LayoutActor {
public:
    explicit CounterGreenStarAppend(const al::LayoutInitInfo& rInfo);
    void control() override;
    void show(s32 oldNum, s32 newNum);
    bool isEnd() const;
    void exeAppear();
    void exeWait();
    void exeAdd();
    void exeEnd();
private:
    s32 mCount = 0;
    s32 mTargetCount = 0;
    fix::TextBoxTextInfo mTextInfo;
};
static_assert(sizeof(CounterGreenStarAppend) == 0x150);

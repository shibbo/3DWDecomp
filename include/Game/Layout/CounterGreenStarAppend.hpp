#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Counter showing the green stars newly collected in the last course.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CounterGreenStarAppend {
public:
    explicit CounterGreenStarAppend(const al::LayoutInitInfo& rInfo);

    void show(s32 oldNum, s32 newNum);
    bool isEnd() const;

private:
    u8 _0[0x150];
};

static_assert(sizeof(CounterGreenStarAppend) == 0x150);

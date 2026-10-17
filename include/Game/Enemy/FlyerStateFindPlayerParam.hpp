#pragma once
#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

/** @brief Turning and discovery animation settings for a flying enemy. */
struct FlyerStateFindPlayerParam {
    FlyerStateFindPlayerParam();
    FlyerStateFindPlayerParam(f32 turnDegree, char* pAction);
    f32 _0;
    sead::FixedSafeString<32> mAction;
};
static_assert(sizeof(FlyerStateFindPlayerParam) == 0x40);

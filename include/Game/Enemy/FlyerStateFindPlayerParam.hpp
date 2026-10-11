#pragma once

#include <basis/seadTypes.h>

struct FlyerStateFindPlayerParam {
    FlyerStateFindPlayerParam(f32, char* pAction);

    f32 _0;
    u32 _4;
    u64 _8[(0x40 - 0x8) / 8];
};

static_assert(sizeof(FlyerStateFindPlayerParam) == 0x40);

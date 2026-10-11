#pragma once

#include <basis/seadTypes.h>

struct FlyerStateReturnAreaParam {
    FlyerStateReturnAreaParam(f32, s32, f32, f32, f32, s32);

    f32 _0;
    s32 _4;
    f32 _8;
    f32 _C;
    f32 _10;
    s32 _14;
};

static_assert(sizeof(FlyerStateReturnAreaParam) == 0x18);

#pragma once

#include <basis/seadTypes.h>

struct FlyerStateChaseParam {
    FlyerStateChaseParam(f32, s32, f32, f32, s32, s32, const char* pChaseAction,
                         const char* pWaitAction);

    u64 _0[0x88 / 8];  // two FixedSafeString<32> (chase and wait actions) at 0x18 and 0x50
};

static_assert(sizeof(FlyerStateChaseParam) == 0x88);

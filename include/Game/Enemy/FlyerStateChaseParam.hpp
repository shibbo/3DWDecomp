#pragma once

#include <basis/seadTypes.h>

struct FlyerStateChaseParam {
    FlyerStateChaseParam(f32, s32, f32, f32, s32, s32, const char* pChaseAction,
                         const char* pWaitAction);

    u64 _0[0x48 / 8];
};

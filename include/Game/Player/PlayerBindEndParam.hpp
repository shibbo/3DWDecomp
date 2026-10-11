#pragma once

#include <basis/seadTypes.h>

/// How a bind ended, as the binding object reported it.
class PlayerBindEndParam {
public:
    bool isInhibitWall() const { return mIsInhibitWall; }

    unsigned char _0[0x20];
    s32 _20;
    s32 _24;
    bool _28;
    bool _29;
    bool _2a;
    s32 _2c;
    f32 _30;
    s32 _34;
    bool mIsInhibitWall;  // 0x38
    unsigned char _39[0x44 - 0x39];
    f32 _44;
};

static_assert(sizeof(PlayerBindEndParam) == 0x48);

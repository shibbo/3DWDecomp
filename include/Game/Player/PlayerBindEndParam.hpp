#pragma once

#include <basis/seadTypes.h>
#include <prim/seadRuntimeTypeInfo.h>
#include <array>

/// How a bind ended, as the binding object reported it.
class PlayerBindEndParam {
public:
    PlayerBindEndParam() = default;
    constexpr PlayerBindEndParam(const char* action, s32 value20, s32 value24,
                                 bool value28, bool value29, bool value2a,
                                 s32 value2c, f32 value30, s32 value34,
                                 bool inhibitWall = false,
                                 std::array<unsigned char, 11> reserved39 = {}, f32 value44 = 0)
        : mActionName(action), _10{}, _20(value20), _24(value24), _28(value28),
          _29(value29), _2a(value2a), _2c(value2c), _30(value30), _34(value34),
          mIsInhibitWall(inhibitWall), _39(reserved39), _44(value44) {}

    SEAD_RTTI_BASE(PlayerBindEndParam)

    bool isInhibitWall() const { return mIsInhibitWall; }

    const char* mActionName;  // 0x8
    unsigned char _10[0x10];
    s32 _20;
    s32 _24;
    bool _28;
    bool _29;
    bool _2a;
    s32 _2c;
    f32 _30;
    s32 _34;
    bool mIsInhibitWall;  // 0x38
    std::array<unsigned char, 0x44 - 0x39> _39;
    f32 _44;
};

static_assert(sizeof(PlayerBindEndParam) == 0x48);

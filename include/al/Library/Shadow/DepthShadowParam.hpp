#pragma once

#include <basis/seadTypes.h>
#include "utility/aglParameter.h"

namespace al {

/**
 * @brief Parameters of the depth shadow, interpolated by the shadow director.
 * @note Minimal declaration: only the parameters the reconstructed code uses are named so far.
 */
class DepthShadowParam {
public:
    DepthShadowParam();

    void init();
    bool operator==(const DepthShadowParam& rOther) const;
    DepthShadowParam& operator=(const DepthShadowParam& rOther);
    void interp(const DepthShadowParam& rA, const DepthShadowParam& rB, f32 rate);

    u8 _0[0x30];
    agl::utl::Parameter<bool> _30;
    agl::utl::Parameter<bool> _50;
    u8 _70[0x90 - 0x70];
    agl::utl::Parameter<bool> _90;
    u8 _b0[0x130 - 0xb0];
    agl::utl::Parameter<s32> _130;
    u8 _150[0x270 - 0x150];
    agl::utl::Parameter<f32> _270;
    agl::utl::Parameter<f32> _290;
    u8 _2b0[0x540 - 0x2b0];
};

static_assert(sizeof(DepthShadowParam) == 0x540);

}  // namespace al

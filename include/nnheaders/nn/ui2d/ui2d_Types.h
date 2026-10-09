#pragma once

#include "nn/gfx/gfx_Sampler.h"
#include <nn/gfx/gfx_Types.h>

namespace nn::gfx {
class DescriptorSlot;
}

namespace nn {
namespace ui2d {

typedef bool (*RegisterSamplerSlot)(nn::gfx::DescriptorSlot*, const nn::gfx::Sampler&, void*);
typedef bool (*AcquireSamplerSlot)(nn::gfx::DescriptorSlot*, const nn::gfx::SamplerInfo&, void*);
typedef void (*UnregisterSamplerSlot)(nn::gfx::DescriptorSlot*, const nn::gfx::Sampler&, void*);
typedef void (*ReleaseSamplerSlot)(nn::gfx::DescriptorSlot*, const nn::gfx::SamplerInfo&, void*);

enum TexWrap { TexWrap_Clamp, TexWrap_Repeat, TexWrap_Mirror, TexWrap_MaxTexWrap };
enum TexFilter { TexFilter_Near, TexFilter_Linear, TexFilter_MaxTexFilter };
/** @brief Horizontal base position of a pane or text. */
enum HorizontalPosition {
    HorizontalPosition_Center,
    HorizontalPosition_Left,
    HorizontalPosition_Right,
    HorizontalPosition_MaxHorizontalPosition
};
/** @brief Vertical base position of a pane or text. */
enum VerticalPosition {
    VerticalPosition_Center,
    VerticalPosition_Top,
    VerticalPosition_Bottom,
    VerticalPosition_MaxVerticalPosition
};

enum BlendOp {
    BlendOp_Disable,
    BlendOp_Add,
    BlendOp_Subtract,
    BlendOp_ReverseSubtract,
    BlendOp_SelectMin,
    BlendOp_SelectMax,
    BlendOp_MaxBlendOp
};

enum LogicOp {
    LogicOp_Disable,
    LogicOp_Noop,
    LogicOp_Clear,
    LogicOp_Set,
    LogicOp_Copy,
    LogicOp_InvCopy,
    LogicOp_Inv,
    LogicOp_And,
    LogicOp_Nand,
    LogicOp_Or,
    LogicOp_Nor,
    LogicOp_Xor,
    LogicOp_Equiv,
    LogicOp_RevAnd,
    LogicOp_InvAnd,
    LogicOp_RevOr,
    LogicOp_InvOr,
    LogicOp_MaxLogicOp
};

enum AlphaTest {
    AlphaTest_Never,
    AlphaTest_Less,
    AlphaTest_LessEqual,
    AlphaTest_Equal,
    AlphaTest_NotEqual,
    AlphaTest_GreaterEqual,
    AlphaTest_Greater,
    AlphaTest_Always,
    AlphaTest_MaxAlphaTest
};

enum BlendFactor {
    BlendFactor_0,
    BlendFactor_1,
    BlendFactor_DstColor,
    BlendFactor_InvDstColor,
    BlendFactor_SrcAlpha,
    BlendFactor_InvSrcAlpha,
    BlendFactor_DstAlpha,
    BlendFactor_InvDstAlpha,
    BlendFactor_SrcColor,
    BlendFactor_InvSrcColor,
    BlendFactor_MaxBlendFactor
};

namespace detail {

template <typename T>
inline T SetBits(T bits, int pos, int len, T val) {
    const uint32_t MaxValue = 0xFFFFFFFFU >> (32 - len);
    const T mask = T(~(MaxValue << pos));
    bits &= mask;
    bits |= val << pos;

    return bits;
}

template <typename T>
inline void SetBits(T* pBits, int pos, int len, T val) {
    const uint32_t MaxValue = 0xFFFFFFFFU >> (32 - len);
    const T mask = T(~(MaxValue << pos));
    *pBits &= mask;
    *pBits |= val << pos;
}

template <typename T>
inline T GetBits(T bits, int pos, int len) {
    const uint32_t mask = ~(0xFFFFFFFFU << len);
    return T((bits >> pos) & mask);
}
};  // namespace detail
};  // namespace ui2d
};  // namespace nn

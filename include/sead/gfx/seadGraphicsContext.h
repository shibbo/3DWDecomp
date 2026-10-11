#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadColor.h>
#include <nvn/nvn.h>

namespace sead
{
class DrawContext;

class GraphicsContext
{
public:
    struct BlendTarget
    {
        u8 mBlendFactorSrcRGB;
        u8 mBlendFactorSrcA;
        u8 mBlendFactorDstRGB;
        u8 mBlendFactorDstA;
        u8 mBlendEquationRGB;
        u8 mBlendEquationA;
    };
    static_assert(sizeof(BlendTarget) == 6);

    static constexpr s32 cRenderTargetNum = 8;

    GraphicsContext();

    void apply(DrawContext* pDrawContext) const;
    void applyAlphaTest(DrawContext* pDrawContext) const;
    void applyDepthAndStencilTest(DrawContext* pDrawContext) const;
    void applyColorMask(DrawContext* pDrawContext) const;
    void applyBlendAndFastZ(DrawContext* pDrawContext) const;
    void applyBlendConstantColor(DrawContext* pDrawContext) const;
    void applyCullingAndPolygonModeAndPolygonOffset(DrawContext* pDrawContext) const;

    void setDepthTestEnable(bool enable) { mDepthTestEnable = enable; }
    void setDepthWriteEnable(bool enable) { mDepthWriteEnable = enable; }
    void setAlphaTestEnable(bool enable) { mAlphaTestEnable = enable; }
    void setCullingMode(u8 mode) { mCullingMode = mode; }
    void setColorMask(u32 mask) { mColorMask = mask; }
    void setColorMask(bool r, bool g, bool b, bool a)
    {
        r ? mColorMask |= 1 : mColorMask &= ~1u;
        g ? mColorMask |= 2 : mColorMask &= ~2u;
        b ? mColorMask |= 4 : mColorMask &= ~4u;
        a ? mColorMask |= 8 : mColorMask &= ~8u;
    }
    void setDepthFunc(u8 func) { mDepthFunc = func; }

    void setDepthEnable(bool test_enable, bool write_enable)
    {
        mDepthTestEnable = test_enable;
        mDepthWriteEnable = write_enable;
    }
    void setBlendEnableMask(u32 mask) { mBlendEnableMask = mask; }
    void setBlendConstantColor(const Color4f& color) { mBlendConstantColor = color; }
    void setBlendFactor(s32 target, u8 src, u8 dst)
    {
        mBlendTargets[target].mBlendFactorSrcRGB = src;
        mBlendTargets[target].mBlendFactorSrcA = src;
        mBlendTargets[target].mBlendFactorDstRGB = dst;
        mBlendTargets[target].mBlendFactorDstA = dst;
    }
    void setBlendEquation(s32 target, u8 equation)
    {
        mBlendTargets[target].mBlendEquationRGB = equation;
        mBlendTargets[target].mBlendEquationA = equation;
    }
    void setBlendEnable(bool enable)
    {
        if (enable)
            mBlendEnableMask |= 1;
        else
            mBlendEnableMask &= ~1u;
    }
    void setBlendEnable(s32 target, bool enable)
    {
        if (enable)
            mBlendEnableMask |= 1u << target;
        else
            mBlendEnableMask &= ~(1u << target);
    }

    void setAlphaTestFunc(u8 func) { mAlphaTestFunc = func; }
    void setPolygonOffsetFrontEnable(bool enable)
    {
        if (enable)
            mPolygonOffsetFrontEnable |= 1;
        else
            mPolygonOffsetFrontEnable &= ~1;
    }
    void setAlphaTestRef(f32 ref) { mAlphaTestRef = ref; }
    void setColorMask(s32 target, bool r, bool g, bool b, bool a)
    {
        u32 mask = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
        mColorMask = (mColorMask & ~(0xFu << (target * 4))) | (mask << (target * 4));
    }
    void setBlendFactorSrcRGB(s32 target, u8 factor) { mBlendTargets[target].mBlendFactorSrcRGB = factor; }
    void setBlendFactorSrcA(s32 target, u8 factor) { mBlendTargets[target].mBlendFactorSrcA = factor; }
    void setBlendFactorDstRGB(s32 target, u8 factor) { mBlendTargets[target].mBlendFactorDstRGB = factor; }
    void setBlendFactorDstA(s32 target, u8 factor) { mBlendTargets[target].mBlendFactorDstA = factor; }
    void setBlendEquationRGB(s32 target, u8 equation) { mBlendTargets[target].mBlendEquationRGB = equation; }
    void setBlendEquationA(s32 target, u8 equation) { mBlendTargets[target].mBlendEquationA = equation; }
    void setStencilTestEnable(bool enable) { mStencilTestEnable = enable; }
    void setStencilTestFunc(u8 func) { mStencilTestFunc = func; }
    void setStencilTestRef(s32 ref) { mStencilTestRef = ref; }
    void setStencilTestMask(u32 mask) { mStencilTestMask = mask; }
    void setStencilOp(u8 fail, u8 zfail, u8 zpass)
    {
        mStencilOpFail = fail;
        mStencilOpZFail = zfail;
        mStencilOpZPass = zpass;
    }
    void setStencilWriteMask(u32 mask) { mStencilWriteMask = mask; }
    void setPolygonOffsetBackEnable(bool enable) { mPolygonOffsetBackEnable = enable; }
    void setPolygonMode(u8 front, u8 back)
    {
        mPolygonModeFront = front;
        mPolygonModeBack = back;
    }

private:
    bool mDepthTestEnable;
    bool mDepthWriteEnable;
    bool mAlphaTestEnable;
    bool mStencilTestEnable;
    u32 mBlendEnableMask;
    Color4f mBlendConstantColor;
    f32 mAlphaTestRef;
    u32 mColorMask;
    s32 mStencilTestRef;
    u32 mStencilTestMask;
    BlendTarget mBlendTargets[cRenderTargetNum];
    f32 mPolygonOffsetFactor;
    f32 mPolygonOffsetUnits;
    f32 mPolygonOffsetClamp;
    u8 mDepthFunc;
    u8 mCullingMode;
    u8 mAlphaTestFunc;
    u8 mStencilOpFail;
    u8 mStencilOpZFail;
    u8 mStencilOpZPass;
    u8 mStencilTestFunc;
    u8 mPolygonModeFront;
    u8 mPolygonModeBack;
    u8 mPolygonOffsetFrontEnable;
    u8 mPolygonOffsetBackEnable;
    u32 mStencilWriteMask;
};
static_assert(sizeof(GraphicsContext) == 0x74);

}  // namespace sead

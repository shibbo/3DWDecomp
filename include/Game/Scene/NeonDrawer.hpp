#pragma once

#include <common/aglShaderEnum.h>
#include <container/seadSafeArray.h>
#include <gfx/seadColor.h>

namespace agl { class ShaderProgram; class RenderBuffer; }
namespace al { class ShaderHolder; class FullScreenQuadModel; class UniformBlock; class GBufferArray; }

class NeonDrawer {
public:
    NeonDrawer();
    void init(al::ShaderHolder* pShaderHolder);
    void setColor(const sead::Color4f& rMainColor, const sead::Color4f& rEdgeColor);
    agl::ShaderMode draw(const agl::RenderBuffer* pOutput, agl::RenderBuffer* pNeon,
                        al::GBufferArray* pGBuffer, agl::ShaderMode shaderMode,
                        s32 displayIndex, f32 nearClip, f32 farClip, bool isColorOnly) const;

private:
    al::FullScreenQuadModel* mQuad;
    const agl::ShaderProgram* mShader = nullptr;
    sead::Color4f mMainColor{1.0f, 1.0f, 1.0f, 1.0f};
    sead::Color4f mEdgeColor{0.0f, 0.0f, 0.0f, 1.0f};
    sead::SafeArray<al::UniformBlock*, 2> mUniformBlocks{{nullptr, nullptr}};
};
static_assert(sizeof(NeonDrawer) == 0x40);

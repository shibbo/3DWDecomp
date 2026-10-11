#include "Scene/NeonDrawer.hpp"

#include <common/aglRenderBuffer.h>
#include <common/aglShaderProgram.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/FullScreenQuadModel.hpp"
#include "Library/Shader/DeferredRendering/GBufferArray.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"

namespace {
const al::UniformBlockLayout sNeonLayout[] = {
    {0, agl::UniformBlock::cType_Float, 1},
    {1, agl::UniformBlock::cType_Float, 1},
    {2, agl::UniformBlock::cType_Float, 1},
    {3, agl::UniformBlock::cType_Float, 1},
    {4, agl::UniformBlock::cType_Float, 1},
    {5, agl::UniformBlock::cType_Float, 1},
    {6, agl::UniformBlock::cType_Float, 1},
    {7, agl::UniformBlock::cType_Vec4, 1},
    {8, agl::UniformBlock::cType_Vec4, 1},
};
struct {
    const char* values[1];
    const char* macros[1];
} sNeonVariation = {{"0"}, {"COLOR_TYPE"}};
}

NeonDrawer::NeonDrawer() : mQuad(new al::FullScreenQuadModel) {}

void NeonDrawer::init(al::ShaderHolder* pShaderHolder) {
    mUniformBlocks[0] = al::createUniformBlock(sNeonLayout, 9, nullptr, 2);
    mUniformBlocks[1] = al::createUniformBlock(sNeonLayout, 9, nullptr, 2);
    mShader = pShaderHolder->getShaderProgram("RenderNeonEffect");
}

void NeonDrawer::setColor(const sead::Color4f& rMainColor, const sead::Color4f& rEdgeColor) {
    mMainColor = sead::Color4f(rMainColor.r, rMainColor.g, rMainColor.b, rMainColor.a);
    mEdgeColor = sead::Color4f(rEdgeColor.r, rEdgeColor.g, rEdgeColor.b, rEdgeColor.a);
}

agl::ShaderMode NeonDrawer::draw(const agl::RenderBuffer* pOutput, agl::RenderBuffer* pNeon,
                                 al::GBufferArray* pGBuffer, agl::ShaderMode shaderMode,
                                 s32 displayIndex, f32 nearClip, f32 farClip, bool isColorOnly) const {
    sNeonVariation.values[0] = isColorOnly ? "1" : "0";
    const agl::ShaderProgram* shader = mShader->searchVariation(
        1, sNeonVariation.macros, sNeonVariation.values);
    pOutput->bind(al::GameFrameworkNx::getAglDrawContext());
    sead::Viewport viewport(*pNeon);
    viewport.apply(al::GameFrameworkNx::getAglDrawContext(), *pNeon);
    agl::SamplerLocation normal("cViewNormal");
    normal.search(*shader);
    agl::SamplerLocation depth("cViewDepth");
    depth.search(*shader);
    pGBuffer->activateSamplerNrmView(normal);
    pGBuffer->activateSamplerDepthView(depth);
    if (!isColorOnly) {
        agl::SamplerLocation neon("cTextureNeon");
        neon.search(*shader);
        agl::TextureSampler sampler(*pNeon->getRenderTargetColor());
        sampler.activate(al::GameFrameworkNx::getAglDrawContext(), neon, -1, false);
    }
    al::UniformBlock* const& uniform = mUniformBlocks[displayIndex];
    uniform->swap();
    uniform->setValue(0, nearClip);
    uniform->setValue(1, nearClip);
    uniform->setValue(2, farClip - nearClip);
    uniform->setValue(3, 0.0005f);
    uniform->setValue(4, 3.9f);
    if (displayIndex == 0) {
        uniform->setValue(5, 1.0f / static_cast<u32>(al::getDisplayWidth()));
        uniform->setValue(6, 1.0f / static_cast<u32>(al::getDisplayHeight()));
    } else {
        uniform->setValue(5, 1.0f / al::getSubDisplayWidth());
        uniform->setValue(6, 1.0f / al::getSubDisplayHeight());
    }
    uniform->setValueRef(7, mMainColor);
    uniform->setValueRef(8, mEdgeColor);
    auto* blockToFlush = uniform;
    const u32 offset = blockToFlush->getCurrentBlockOffset(0);
    const u32 size = blockToFlush->getBlockSize();
    agl::GPUMemVoidAddr addr = blockToFlush->getBuffer();
    agl::GPUMemVoidAddr(addr, offset).flushCPUCache(size);
    agl::UniformBlockLocation block("RenderNeon");
    block.search(*shader);
    uniform->activate(al::GameFrameworkNx::getAglDrawContext(), block);
    shader->activate(al::GameFrameworkNx::getAglDrawContext(), agl::cShaderMode_UniformBlock);
    sead::GraphicsContext context;
    context.setDepthTestEnable(true);
    context.setDepthWriteEnable(false);
    context.setDepthFunc(8);
    context.setAlphaTestEnable(false);
    context.setBlendEnable(true);
    context.apply(al::GameFrameworkNx::getAglDrawContext());
    mQuad->drawQuad();
    return shaderMode;
}

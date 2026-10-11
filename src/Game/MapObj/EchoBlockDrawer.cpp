#include "MapObj/EchoBlockDrawer.hpp"

#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <nn/g3d/g3d_ResShader.h>

#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/PeripheryRendering.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"

namespace {
const al::UniformBlockLayout sEchoBlockLayout[] = {
    {0, agl::UniformBlock::cType_Float, 1},
    {1, agl::UniformBlock::cType_Float, 1},
};
}

EchoBlockDrawer::EchoBlockDrawer() : mShader(nullptr), mUniformBlocks{}, _28(nullptr) {}

void EchoBlockDrawer::init(al::ShaderHolder* shaderHolder) {
    mUniformBlocks[0] = al::createUniformBlock(sEchoBlockLayout, 2, nullptr, 2);
    mUniformBlocks[1] = al::createUniformBlock(sEchoBlockLayout, 2, nullptr, 2);
    mShader = shaderHolder->getShadingModel("RenderMaterialEcho")->GetProgram(0);
}

void EchoBlockDrawer::draw(int viewIndex, const sead::LookAtCamera* camera,
                           const sead::PerspectiveProjection* projection) const {
    if (!_28)
        return;
    mUniformBlocks[viewIndex]->swap();
    mUniformBlocks[viewIndex]->setValue(0, projection->getNear());
    mUniformBlocks[viewIndex]->setValue(1, 1.0f / (projection->getFar() - projection->getNear()));
    mUniformBlocks[viewIndex]->flushCurrentBuffer();
    mUniformBlocks[viewIndex]->activate(al::GameFrameworkNx::getAglDrawContext(),
                                      al::getUniformBlockLocationRenderGBuffer());
    al::activateShader(mShader, agl::cShaderMode_UniformBlock);
}

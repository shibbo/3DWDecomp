#include "Player/FurEnv.hpp"

#include <common/aglShaderLocation.h>
#include <common/aglTextureSampler.h>
#include <shadow/aglDepthShadow.h>
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/GBufferArray.hpp"
#include "Library/Shader/DeferredRendering/PeripheryRendering.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Shadow/Depth/DepthShadowDrawer.hpp"
#include "Library/Shadow/ShadowDirector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/SceneObjID.hpp"

namespace {
const al::UniformBlockLayout sFurUboLayout[] = {
    {0, agl::UniformBlock::cType_Float, 1}, {1, agl::UniformBlock::cType_Float, 1},
    {2, agl::UniformBlock::cType_Float, 1}, {3, agl::UniformBlock::cType_Float, 1},
    {4, agl::UniformBlock::cType_Float, 1}, {5, agl::UniformBlock::cType_Float, 1},
    {6, agl::UniformBlock::cType_Float, 1}, {7, agl::UniformBlock::cType_Float, 1},
};

const al::UniformBlockLayout sFurShadowUboLayout[] = {
    {0, agl::UniformBlock::cType_Vec4, 4},
    {1, agl::UniformBlock::cType_Float, 1},
    {2, agl::UniformBlock::cType_Float, 1},
};

const char* sVariationMacros[] = {
    "cDepthShadowType",
    "cIsEnableShadowDistDamp",
    "cShadowPcfType",
    "cSkinWeightNum",
};
}  // namespace

/**
 * Creates the fur uniform blocks and fetches the shell fur shading model.
 * @param pGraphicsSystemInfo the scene's graphics system info
 */
FurEnv::FurEnv(al::GraphicsSystemInfo* pGraphicsSystemInfo)
    : mGraphicsSystemInfo(pGraphicsSystemInfo) {
    mFurUbo = al::createUniformBlock(sFurUboLayout, 8, nullptr, 2);
    mShadowUbo = al::createUniformBlock(sFurShadowUboLayout, 3, nullptr, 2);
    mShadingModel = al::ShaderHolder::instance()->getShadingModel("RenderShellFur");
}

/**
 * Destroys the fur uniform blocks.
 */
FurEnv::~FurEnv() {
    if (mFurUbo != nullptr) {
        delete mFurUbo;
        mFurUbo = nullptr;
    }

    if (mShadowUbo != nullptr) {
        delete mShadowUbo;
        mShadowUbo = nullptr;
    }
}

/**
 * Writes the current shadow parameters to the fur uniform blocks and flushes them.
 */
void FurEnv::update() {
    al::ShadowDirector* shadowDirector = mGraphicsSystemInfo->getShadowDirector();

    mFurUbo->setValue(0, 1.0f);
    mFurUbo->setValue(1, 1.0f);
    mFurUbo->setValue(2, shadowDirector->_400);
    mFurUbo->setValue(3, 1.0f);
    mFurUbo->setValue(4, shadowDirector->mDistDampStart);
    mFurUbo->setValue(5, 1.0f / (shadowDirector->mDistDampEnd - shadowDirector->mDistDampStart));
    mFurUbo->setValue(6, 1.0f);
    mFurUbo->setValue(7, shadowDirector->_1b0);
    mFurUbo->flushCurrentBuffer();
    mFurUbo->swap();

    al::DepthShadowDrawer* drawer = mGraphicsSystemInfo->getShadowDirector()->mDepthShadowDrawer;
    if (drawer != nullptr) {
        const agl::sdw::DepthShadowUnit& unit = drawer->getDepthShadow()->getUnit(0);
        mShadowUbo->setData(0, &unit.getTexMtx(), 0, 4);
        mShadowUbo->setValue(1, 12.0f);
        mShadowUbo->setValue(2, 0.0f);
        mShadowUbo->flushCurrentBuffer();
        mShadowUbo->swap();
    }
}

/**
 * Binds the albedo G-buffer, the depth shadow map and both fur uniform blocks for drawing.
 */
void FurEnv::activate() {
    if (mGraphicsSystemInfo->getDrawGBufferArray() != nullptr) {
        agl::SamplerLocation albedo(agl::ShaderLocation(3), "GBufAlbedo");
        mGraphicsSystemInfo->getDrawGBufferArray()->activateSamplerAlbedo(albedo);
    }

    al::DepthShadowDrawer* drawer = mGraphicsSystemInfo->getShadowDirector()->mDepthShadowDrawer;
    if (drawer != nullptr) {
        const agl::TextureSampler* sampler =
            drawer->getDepthShadow()->getShadowMap().getDepthSampler();
        agl::DrawContext* drawContext = al::GameFrameworkNx::getAglDrawContext();
        agl::SamplerLocation depthShadow(agl::ShaderLocation(4), "DepthShadow");
        sampler->activate(drawContext, depthShadow, -1, false);
    }

    mFurUbo->activate(al::GameFrameworkNx::getAglDrawContext(),
                      agl::UniformBlockLocation(agl::ShaderLocation(10), "Fur"));
    mShadowUbo->activate(al::GameFrameworkNx::getAglDrawContext(),
                         agl::UniformBlockLocation(agl::ShaderLocation(11), "FurShadow"));
}

/**
 * Searches the shell fur shader variation for the current shadow settings.
 * @param skinWeightNum the skin weight count of the drawn shape
 * @return the matching shader program
 */
const nn::g3d::ResShaderProgram* FurEnv::getShaderProgram(u32 skinWeightNum) const {
    const al::ShadowDirector* shadowDirector = mGraphicsSystemInfo->getShadowDirector();
    s32 depthShadowType;
    if (shadowDirector->isEnableVarianceShadow()) {
        depthShadowType = 3;
    } else {
        depthShadowType = shadowDirector->isEnableDepthShadow();
    }

    al::StringTmp<32> depthShadowTypeStr("%d", depthShadowType);
    al::StringTmp<32> distDampStr("%d", shadowDirector->mIsEnableDistDamp);
    al::StringTmp<32> pcfTypeStr("%d", shadowDirector->mPcfType);
    al::StringTmp<32> skinWeightNumStr("%d", skinWeightNum);
    const char* values[] = {
        depthShadowTypeStr.cstr(),
        distDampStr.cstr(),
        pcfTypeStr.cstr(),
        skinWeightNumStr.cstr(),
    };
    return al::searchVariation(mShadingModel, 4, sVariationMacros, values);
}

namespace rc {
/**
 * Gets the fur environment of a scene.
 * @param pHolder scene object holder user
 * @return the fur environment
 */
FurEnv* getFurEnv(const al::IUseSceneObjHolder* pHolder) {
    return al::getSceneObj<FurEnv>(pHolder, SceneObjID_FurEnv);
}
}  // namespace rc

#pragma once

#include <basis/seadTypes.h>
#include <common/aglRenderBuffer.h>
#include <common/aglRenderTarget.h>
#include <common/aglTextureSampler.h>
#include <container/seadBuffer.h>
#include <container/seadPtrArray.h>
#include <gfx/seadColor.h>
#include <math/seadVector.h>
#include <postfx/aglAutoExposure.h>
#include <utility/aglParameter.h>
#include <utility/aglParameterObj.h>

#include "Project/Draw/GraphicsParamKeeper.hpp"

#include "common/aglShaderEnum.h"

namespace agl {
class ShaderProgram;
class TextureData;
}  // namespace agl

namespace sead {
class Viewport;
}  // namespace sead

namespace agl::pfx {
class Bloom;
}  // namespace agl::pfx

namespace al {
class GraphicsSystemInfo;
class UniformBlock;

/**
 * HDR compose parameters of a graphics area.
 */
class HdrParam {
public:
    void init();
    bool operator==(const HdrParam& rOther) const;
    HdrParam& operator=(const HdrParam& rOther);
    void interp(const HdrParam& rA, const HdrParam& rB, f32 rate);

    agl::utl::IParameterObj* getParamObj() { return &mParamObj; }

    s32 getCameraMaskTex() const { return *mCameraMaskTex; }

    bool isMaskBloom() const { return *mIsMaskBloom; }

    bool isMaskLightStreak() const { return *mIsMaskLightStreak; }

    bool isMaskGodRay() const { return *mIsMaskGodRay; }

    s32 getExposureType() const { return *mExposureType; }

    f32 getCameraMaskBase() const { return *mCameraMaskBase; }

    f32 getCameraMaskScale() const { return *mCameraMaskScale; }

    const sead::Color4f& getCameraMaskDiffuse() const { return *mCameraMaskDiffuse; }

    s32 getCameraIndirectTex() const { return *mCameraIndirectTex; }

    f32 getCameraIndirectScale() const { return *mCameraIndirectScale; }

    const sead::Vector2f& getCameraIndirectTexScale() const { return *mCameraIndirectTexScale; }

    const sead::Vector2f& getCameraIndirectOffsetVelocity() const {
        return *mCameraIndirectOffsetVelocity;
    }

    f32 getChromaticAberrationSize() const { return *mChromaticAberrationSize; }

    s32 getCameraIndirect2Tex() const { return *mCameraIndirect2Tex; }

    f32 getCameraIndirect2Scale() const { return *mCameraIndirect2Scale; }

    const sead::Vector2f& getCameraIndirect2TexScale() const { return *mCameraIndirect2TexScale; }

    const sead::Vector2f& getCameraIndirect2OffsetVelocity() const {
        return *mCameraIndirect2OffsetVelocity;
    }

    f32 getAutoExposureMid() const { return *mAutoExposureMid; }

    f32 getAutoExposureRangeMax() const { return *mAutoExposureRangeMax; }

    f32 getAutoExposureRangeMin() const { return *mAutoExposureRangeMin; }

    f32 getAutoExposureIgnoreRangeMax() const { return *mAutoExposureIgnoreRangeMax; }

    f32 getAutoExposureBlendRateUp() const { return *mAutoExposureBlendRateUp; }

    f32 getAutoExposureBlendRateDown() const { return *mAutoExposureBlendRateDown; }

    f32 getAutoExposureHistogramScale() const { return *mAutoExposureHistogramScale; }

    f32 getAutoExposureIgnoreRangeMin() const { return *mAutoExposureIgnoreRangeMin; }

private:
    agl::utl::ParameterObj mParamObj;
    agl::utl::Parameter<s32> mCameraMaskTex;
    agl::utl::Parameter<bool> mIsMaskBloom;
    agl::utl::Parameter<bool> mIsMaskLightStreak;
    agl::utl::Parameter<bool> mIsMaskGodRay;
    agl::utl::Parameter<s32> mToneMapType;
    agl::utl::Parameter<s32> mExposureType;
    agl::utl::Parameter<f32> mCameraMaskBase;
    agl::utl::Parameter<f32> mCameraMaskScale;
    agl::utl::Parameter<sead::Color4f> mCameraMaskDiffuse;
    agl::utl::Parameter<s32> mCameraIndirectTex;
    agl::utl::Parameter<f32> mCameraIndirectScale;
    agl::utl::Parameter<sead::Vector2f> mCameraIndirectTexScale;
    agl::utl::Parameter<sead::Vector2f> mCameraIndirectOffsetVelocity;
    agl::utl::Parameter<f32> mChromaticAberrationSize;
    agl::utl::Parameter<s32> mCameraIndirect2Tex;
    agl::utl::Parameter<f32> mCameraIndirect2Scale;
    agl::utl::Parameter<sead::Vector2f> mCameraIndirect2TexScale;
    agl::utl::Parameter<sead::Vector2f> mCameraIndirect2OffsetVelocity;
    agl::utl::Parameter<s32> mCameraIndirect2Usage;
    agl::utl::Parameter<f32> mAutoExposureMid;
    agl::utl::Parameter<f32> mAutoExposureRangeMax;
    agl::utl::Parameter<f32> mAutoExposureRangeMin;
    agl::utl::Parameter<f32> mAutoExposureIgnoreRangeMax;
    agl::utl::Parameter<f32> mAutoExposureBlendRateUp;
    agl::utl::Parameter<f32> mAutoExposureBlendRateDown;
    agl::utl::Parameter<f32> mAutoExposureHistogramScale;
    agl::utl::Parameter<f32> mAutoExposureIgnoreRangeMin;
};

static_assert(sizeof(HdrParam) == 0x398);

/**
 * Composes the HDR post effects (bloom, flare filter, god ray, light streak) together with the
 * camera mask, camera indirect and auto exposure effects.
 */
class HdrCompose : public GraphicsParamRequestInterpKeeper<HdrParam> {
public:
    /**
     * Samplers and uniform block used to draw the compose of one view.
     */
    struct ViewContext {
        /**
         * Creates the samplers. The camera indirect textures repeat over the screen.
         */
        ViewContext() {
            mCameraIndirectSampler.setWrap(1, 1, 1);
            mCameraIndirect2Sampler.setWrap(1, 1, 1);
        }

        agl::TextureSampler mHdrImageSampler;
        agl::TextureSampler _170;
        agl::TextureSampler _2e0;
        agl::TextureSampler mCameraMaskSampler;
        agl::TextureSampler mCameraIndirectSampler;
        agl::TextureSampler mCameraIndirect2Sampler;
        UniformBlock* mUniformBlock = nullptr;
    };

    static_assert(sizeof(ViewContext) == 0x8a8);

    HdrCompose(s32 viewNum, GraphicsSystemInfo* pInfo);
    ~HdrCompose();

    virtual void endInit();

    bool isUsingMyHdrCompose() const;
    void movement(bool isPaused);
    void preDrawGraphics();
    void releaseComposeBuffer();
    void calcGPU();
    void setupComposeBuffer(s32 viewIndex, const agl::RenderBuffer& rBuffer,
                            const agl::pfx::Bloom* pBloom);
    bool isAtLeastOneCompose(s32 viewIndex, const agl::pfx::Bloom* pBloom) const;
    bool isAtLeastOneComposeMask(s32 viewIndex, const agl::pfx::Bloom* pBloom) const;
    const agl::RenderBuffer* getRenderBufferFlareFilter() const;
    const agl::RenderBuffer* getRenderBufferBloom() const;
    const agl::RenderBuffer* getRenderBufferGodRay() const;
    const agl::RenderBuffer* getRenderBufferLightStreak() const;
    agl::ShaderMode draw(s32 viewIndex, const agl::RenderBuffer& rBuffer,
                         const sead::Viewport& rViewport, const agl::TextureData& rTexture,
                         const agl::pfx::Bloom* pBloom, agl::ShaderMode shaderMode) const;

    void setEnableDangerIndicator(bool isEnable) { mIsEnableDangerIndicator = isEnable; }

private:
    bool isValidCameraMask() const {
        s32 cameraMaskTex = getCurrentParam().getCameraMaskTex();
        return cameraMaskTex != 0 && mCameraMaskTextures[cameraMaskTex - 1] != nullptr;
    }

    const agl::ShaderProgram* mShaderProgram;
    sead::PtrArray<ViewContext> mViewContexts;
    agl::pfx::AutoExposure mAutoExposure;
    sead::Buffer<agl::TextureData*> mCameraMaskTextures;
    sead::Buffer<agl::TextureData*> mCameraIndirectTextures;
    s32 mCameraIndirectTexType = -1;
    sead::Vector2f mCameraIndirectOffset = sead::Vector2f::zero;
    s32 mCameraIndirect2TexType = -1;
    sead::Vector2f mCameraIndirect2Offset = sead::Vector2f::zero;
    agl::TextureSampler mComposeSampler;
    agl::TextureSampler mComposeMaskSampler;
    agl::TextureData* mComposeTexture = nullptr;
    agl::TextureData* mComposeMaskTexture = nullptr;
    agl::RenderTargetColor mComposeTarget;
    agl::RenderTargetColor mComposeMaskTarget;
    agl::RenderBuffer mComposeBuffer;
    agl::RenderBuffer mComposeMaskBuffer;
    bool mIsEnableDangerIndicator = false;
    sead::Vector2f mDangerIndicatorParam = {250.0f, 100.0f};
    f32 _253c = 0.0f;
    sead::Vector2f mDangerIndicatorParam2 = {0.0f, 0.0f};
    f32 mDangerIndicatorParam3 = 0.0f;
    f32 mAspectRatio = 1.0f;
    bool mIsDisableCameraIndirect = false;
};

static_assert(sizeof(HdrCompose) == 0x2558);

}  // namespace al

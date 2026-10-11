#pragma once

#include <container/seadBuffer.h>
#include <gfx/seadColor.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <hostio/seadHostIONode.h>
#include <math/seadVector.h>
#include "common/aglRenderBuffer.h"
#include "common/aglRenderTarget.h"
#include "common/aglTextureData.h"
#include "common/aglTextureSampler.h"
#include "common/aglTextureEnum.h"
#include "cull/aglViewFrustumCulling.h"
#include "utility/aglContextParameterBuffer.h"
#include "utility/aglDebugTexturePage.h"
#include "utility/aglParameter.h"
#include "utility/aglParameterIO.h"

namespace sead {
class Heap;
namespace hostio {
class Context;
class PropertyEvent;
class Reflexible;
}  // namespace hostio
}  // namespace sead

namespace agl {
class DrawContext;
}

namespace agl::pfx {

class FlareFilterParameter {
public:
    FlareFilterParameter();
    virtual ~FlareFilterParameter() {}

    void initialize(utl::IParameterObj* pObj, sead::Heap* pHeap);
    void genMessageFlareFilterParameter(sead::hostio::Context* pContext);
    void listenPropertyEventFlareFilterParameter(sead::hostio::Reflexible* pReflexible,
                                                 const sead::hostio::PropertyEvent* pEvent);

    utl::Parameter<sead::Color4f> mColor;
    utl::Parameter<bool> mIsBlurAfterFlare;
    utl::Parameter<bool> mIsEnableThreshold;
    utl::Parameter<f32> mThreshold;
    utl::Parameter<s32> mGhostNum;
    utl::Parameter<f32> mGhostDispersal;
    sead::Buffer<utl::Parameter<sead::Color4f>> mGhostColors;
    utl::Parameter<bool> mIsEnableHalo;
    utl::Parameter<f32> mHaloWidth;
    utl::Parameter<sead::Color4f> mHaloColor;
    utl::Parameter<bool> mIsEnableChromaDistortion;
    utl::Parameter<sead::Vector3f> mChromaDistortion;
    utl::Parameter<f32> mChromaDistortionScale;
};
static_assert(sizeof(FlareFilterParameter) == 0x1b0);

class FlareFilter : public utl::ContextParameterBuffer<FlareFilter, FlareFilterParameter>,
                    public utl::IParameterIO,
                    public sead::hostio::Node {
public:
    class Tex {
    public:
        void alloc(DrawContext* pDrawContext, TextureFormat format, u32 width, u32 height,
                   const char* pName, bool withoutContext) const;
        void refer(DrawContext* pDrawContext, const TextureData* pTextureData) const
        {
            u32 width = pTextureData->getWidth(0);
            u32 height = pTextureData->getHeight(0);
            bool isDepth = u32(pTextureData->getTextureFormat() -
                               u16(TextureFormat::cTextureFormat_Depth_16)) < 4;
            mpTextureData = pTextureData;
            mIsDepth = isDepth;
            mSampler.applyTextureData(*pTextureData);
            mSampler.setFilter(1, 1, 1);
            mRenderBuffer.setRenderTargetColorNullAll();
            mRenderBuffer.setRenderTargetDepth(nullptr);
            if (mIsDepth)
            {
                mDepthTarget.applyTextureData(*mpTextureData);
                mRenderBuffer.setVirtualSize(sead::Vector2f(width, height));
                mRenderBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
                mRenderBuffer.setRenderTargetDepth(&mDepthTarget);
            }
            else
            {
                mColorTarget.applyTextureData(*mpTextureData);
                mRenderBuffer.setVirtualSize(sead::Vector2f(width, height));
                mRenderBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
                mRenderBuffer.setRenderTargetColor(&mColorTarget);
            }
            mWidth = width;
            mHeight = height;
            mViewport.setByFrameBuffer(mRenderBuffer);
        }
        void release() const;

        mutable const TextureData* mpTextureData = nullptr;
        mutable TextureSampler mSampler;
        mutable RenderBuffer mRenderBuffer;
        mutable RenderTargetColor mColorTarget;
        mutable RenderTargetDepth mDepthTarget;
        mutable sead::Viewport mViewport;
        mutable u32 mWidth;
        mutable u32 mHeight;
        mutable bool mIsAllocated = false;
        mutable bool mIsDepth = false;
        mutable bool mIsWithoutContext = false;
    };
    static_assert(sizeof(Tex) == 0x508);

    struct Context {
        cull::ViewFrustumCulling mCulling;
        Tex mTarget;
        Tex mSource;
        Tex mUnused;
        Tex mFlare;
        Tex mWork0;
        Tex mWork1;
    };
    static_assert(sizeof(Context) == 0x2068);

    FlareFilter();
    ~FlareFilter() override;

    void initialize(s32 contextNum, sead::Heap* pHeap);
    void initializeContext(Context* pContext, sead::Heap* pHeap);
    void calc();
    void calcView(s32 context, const cull::ViewFrustumCulling& rCulling);
    void calcGPU() const;
    void calcViewGPU(s32 context) const;
    void draw(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
              const sead::Viewport& rViewport, const TextureData& rTexture, s32 reduceNum,
              s32 blurNum) const;
    void drawToFlareBuffer(DrawContext* pDrawContext, s32 context, const TextureData& rTexture,
                           s32 reduceNum, s32 blurNum) const;
    void releaseFlareBuffer(s32 context) const;
    void drawDebug(DrawContext* pDrawContext, s32 context) const;
    void genMessage(sead::hostio::Context* pContext);
    void listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent);

    bool isEnable() const { return *mEnable && isEnableContext(-1); }
    bool isEnable(s32 context) const { return *mEnable && isEnableContext(context); }

private:
    void drawCopy_(DrawContext* pDrawContext, const Tex& rDst, const Tex& rSrc, bool isReduce,
                   bool isFirst, bool isThreshold, f32 threshold, f32 scale,
                   const sead::Color4f& rColor) const;
    void drawBlur_(DrawContext* pDrawContext, const Tex& rDst, const Tex& rSrc, s32 direction,
                   bool isSecond, bool b2, f32 scale, const sead::Color4f& rColor) const;

    utl::Parameter<bool> mEnable;
    s32 _458 = 0;
    sead::Buffer<Context> mUnusedContexts;
    sead::GraphicsContext mGraphicsContext;
    sead::GraphicsContext mGraphicsContextAdd;
    utl::DebugTexturePage mDebugTexturePage;
};

}  // namespace agl::pfx

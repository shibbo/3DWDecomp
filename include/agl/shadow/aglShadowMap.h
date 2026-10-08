#pragma once

#include <hostio/seadHostIONode.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "common/aglRenderBuffer.h"
#include "common/aglRenderTarget.h"
#include "common/aglTextureSampler.h"
#include "utility/aglDebugTexturePage.h"
#include "utility/aglDynamicTextureAllocator.h"
#include "utility/aglParameter.h"
#include "utility/aglParameterObj.h"

namespace sead
{
class Heap;
namespace hostio
{
class Context;
class PropertyEvent;
}  // namespace hostio
}  // namespace sead

namespace agl
{

class DrawContext;
class TextureData;

namespace sdw
{

class ShadowMap : public utl::IParameterObj, public sead::hostio::Node
{
public:
    struct CreateArg
    {
        s32 mCascadeNum;
    };

    ShadowMap();
    ~ShadowMap() override;

    void free();
    void initialize(const CreateArg& rArg, sead::Heap* pHeap);
    void allocDepthBuffer(DrawContext* pDrawContext, s32 sliceNum, s32 mipLevelNum);
    void freeFullOnly();
    void freeHalf() const;
    void freeQuat() const;
    void beginDepthBuffer(DrawContext* pDrawContext, s32 index, const sead::Vector2f& rOffset,
                          const sead::Vector2f& rScale);
    void endDepthBuffer(DrawContext* pDrawContext, s32 index);
    void drawReduce(DrawContext* pDrawContext) const;
    void drawVariance(DrawContext* pDrawContext) const;
    void setSize(const sead::Vector2i& rSize);
    void drawDebug(DrawContext* pDrawContext, s32 index, const sead::Matrix44f& rTexMtx,
                   const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);

    void genMessage(sead::hostio::Context* pContext);
    void genMessageParameter(sead::hostio::Context* pContext, sead::hostio::Node* pNode);
    void genMessageDebugParameter(sead::hostio::Context* pContext, sead::hostio::Node* pNode);
    void listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent);
    void listenPropertyEventParameter(const sead::hostio::PropertyEvent* pEvent);
    void listenPropertyEventDebugParameter(const sead::hostio::PropertyEvent* pEvent);

    s32 getShadowMapType() const { return *mShadowMapType; }
    void setEnableHiZ(bool isEnable) { *mEnableHiZ = isEnable; }
    s32 getWidth() const { return *mSizeW; }
    s32 getHeight() const { return *mSizeH; }
    const TextureSampler* getDepthSampler() const { return mDepthSampler; }
    const TextureData* getDepthTexture() const { return mDepthTexture; }

private:
    void freeFull_() const;
    void freeDepth_() const;
    void initializeDepthSampler_(TextureSampler* pSampler);

    static void freeTexture_(const TextureData*& rTexture)
    {
        if (rTexture)
        {
            utl::DynamicTextureAllocator::instance()->free(rTexture);
            rTexture = nullptr;
        }
    }

    utl::Parameter<s32> mShadowMapType;
    utl::Parameter<s32> mSizeW;
    utl::Parameter<s32> mSizeH;
    utl::Parameter<s32> mReduceType;
    utl::Parameter<bool> mEnableHiZ;
    utl::Parameter<bool> mForceArray;
    utl::Parameter<bool> mUse16UNorm;
    utl::Parameter<bool> mAllocWithoutContext;
    utl::Parameter<bool> mAllocFromMem1;
    utl::Parameter<bool> mExpandToMem2;
    utl::Parameter<bool> mExpandAllSlice;
    utl::Parameter<bool> mScissor;
    utl::Parameter<bool> mCreateHalf;
    utl::Parameter<s32> mHalfCascadeNum;
    utl::Parameter<bool> mCreateQuarter;
    utl::Parameter<s32> mQuarterCascadeNum;
    utl::Parameter<bool> mReduceMem1;
    utl::Parameter<f32> mScissorMargin;
    TextureSampler* mDepthSampler = nullptr;
    mutable const TextureData* mDepthTexture = nullptr;
    mutable const TextureData* mExpandTexture = nullptr;
    mutable TextureSampler mDepthTextureSampler;
    mutable const TextureData* mVarianceTexture = nullptr;
    mutable TextureSampler mVarianceSampler;
    mutable const TextureData* mHalfTexture = nullptr;
    mutable TextureSampler mHalfSampler;
    mutable const TextureData* mQuarterTexture = nullptr;
    mutable TextureSampler mQuarterSampler;
    mutable RenderTargetDepth mRenderTargetDepth;
    mutable RenderBuffer mRenderBuffer;
    mutable RenderTargetColor mRenderTargetColor;
    mutable RenderTargetDepth mReduceTargetDepth;
    mutable TextureSampler mReduceSampler;
    utl::DebugTexturePage mDebugTexturePage;
    bool mIsAllocWithoutContext = false;
    bool mIsDirty = false;
};
static_assert(sizeof(ShadowMap) == 0x10f0);

}  // namespace sdw
}  // namespace agl

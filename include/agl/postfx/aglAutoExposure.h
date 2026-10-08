#pragma once

#include <container/seadBuffer.h>
#include <gfx/seadColor.h>
#include <hostio/seadHostIONode.h>
#include <math/seadVector.h>
#include <prim/seadBitFlag.h>
#include "common/aglGPUMemAddr.h"
#include "common/aglGPUMemBlock.h"
#include "common/aglIndexStream.h"
#include "common/aglRenderBuffer.h"
#include "common/aglRenderTarget.h"
#include "common/aglTextureData.h"
#include "common/aglTextureSampler.h"
#include "common/aglVertexAttribute.h"
#include "common/aglVertexBuffer.h"
#include "utility/aglDebugTexturePage.h"
#include "utility/aglParameter.h"
#include "utility/aglParameterIO.h"
#include "utility/aglParameterObj.h"

namespace sead {
class Heap;
namespace hostio {
class Context;
class PropertyEvent;
}  // namespace hostio
}  // namespace sead

namespace agl {
class DrawContext;
class ShaderProgram;
}  // namespace agl

namespace agl::pfx {

struct AutoExposureVtx {
    sead::Vector3f mPos;
};
static_assert(sizeof(AutoExposureVtx) == 0xc);

class AutoExposureVtxStream {
public:
    void initialize(sead::Heap* pHeap);
    void create(s32 num);

    s32 mNum = 128;
    VertexAttribute mVertexAttribute;
    GPUMemBlock<AutoExposureVtx> mVertexBlock;
    VertexBuffer mVertexBuffer;
    GPUMemBlock<u16> mIndexBlock;
    IndexStream mIndexStream;
};
static_assert(sizeof(AutoExposureVtxStream) == 0x3d8);

class AutoExposure : public sead::hostio::Node, public utl::IParameterIO {
public:
    class ResultBuffer {
    public:
        void Create(sead::Heap* pHeap, s32 width, s32 height, TextureFormat format);
        void Bind(DrawContext* pDrawContext);
        void Clear(DrawContext* pDrawContext, const sead::Color4f& rColor);


        s32 mWidth;
        s32 mHeight;
        TextureSampler mSampler;
        RenderTargetColor mRenderTarget;
        RenderBuffer mRenderBuffer;
        TextureData mTexture;
        GPUMemAddr<u8> mImage;
    };
    static_assert(sizeof(ResultBuffer) == 0x498);

    enum ResultType {
        cResult_Prev0,
        cResult_Prev1,
        cResult_Histogram,
        cResult_HistogramDebug,
        cResult_Num
    };

    struct Context {
        TextureSampler mSampler;
        TextureSampler mSampler2;
        ResultBuffer mResults[cResult_Num];
        s32 mCurrent;
        s32 mPrev;
        bool mIsUpdated;
    };
    static_assert(sizeof(Context) == 0x1550);

    enum Flag {
        cFlag_DebugVertex = 1 << 1,
        cFlag_Simple = 1 << 2,
        cFlag_Cleared = 1 << 3,
        cFlag_ForceClear = 1 << 4,
    };

    AutoExposure();
    virtual ~AutoExposure();

    void initialize(s32 contextNum, sead::Heap* pHeap);
    void calc();
    void calcGPU() const;
    void calcGPU(s32 context) const;
    void setCommonShaderParam(DrawContext* pDrawContext, const ShaderProgram* pProgram) const;
    void draw(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
              const TextureData* pTexture) const;
    void drawHistogram(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
                       const TextureData* pTexture) const;
    void drawHistogramCalc(DrawContext* pDrawContext, s32 context,
                           const RenderBuffer& rRenderBuffer, const TextureData* pTexture) const;
    void drawSimple(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
                    const TextureData* pTexture) const;
    void drawDebug(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
                   const TextureData* pTexture) const;
    void drawHistogramDebugVertex(DrawContext* pDrawContext, s32 context,
                                  const RenderBuffer& rRenderBuffer,
                                  const TextureData* pTexture) const;
    void drawHistogramDebug(DrawContext* pDrawContext, s32 context) const;
    void enableSingleChannel(bool enable, TextureCompSel compSel);
    void genMessage(sead::hostio::Context* pContext);
    void listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent);

    const Context& getContext(s32 context) const { return mContexts[context]; }
    void setBlendRateUp(f32 rate) { *mBlendRateUp = rate; }
    void setBlendRateDown(f32 rate) { *mBlendRateDown = rate; }
    void setExposureMid(f32 mid) { *mExposureMid = mid; }
    void setRangeMin(f32 range) { *mRangeMin = range; }
    void setRangeMax(f32 range) { *mRangeMax = range; }
    void setLuminanceScale(f32 scale) { mLuminanceScale = scale; }
    void setLuminanceMax(f32 luminance) { mLuminanceMax = luminance; }
    void setLuminanceMin(f32 luminance) { mLuminanceMin = luminance; }

private:
    Context& getContext_(s32 context) const { return const_cast<Context&>(mContexts[context]); }

    sead::Buffer<Context> mContexts;
    mutable sead::BitFlag32 mFlags = 0;
    utl::DebugTexturePage mDebugTexturePage;
    AutoExposureVtxStream mVtxStream;
    utl::ParameterObj mParamObj;
    utl::Parameter<bool> mIsEnable{true, "IsEnable", "有効", &mParamObj};
    utl::Parameter<f32> mBlendRateUp{0.15f, "BlendRateUp", "明順応速度", &mParamObj};
    utl::Parameter<f32> mBlendRateDown{0.025f, "BlendRateDown", "暗順応速度", &mParamObj};
    utl::Parameter<f32> mExposureMid{0.18f, "ExposureMid", "基準値", &mParamObj};
    utl::Parameter<f32> mRangeMin{0.0f, "RangeMin", "暗くする倍率下限", &mParamObj};
    utl::Parameter<f32> mRangeMax{10.0f, "RangeMax", "明るくする倍率上限", &mParamObj};
    utl::Parameter<f32> mVertexOffsetX{0.0f, "VertexOffsetX", "サンプル点オフセットX", &mParamObj};
    utl::Parameter<f32> mVertexOffsetY{0.0f, "VertexOffsetY", "サンプル点オフセットY", &mParamObj};
    utl::Parameter<f32> mVertexScaleX{1.0f, "VertexScaleX", "サンプル点スケールX", &mParamObj};
    utl::Parameter<f32> mVertexScaleY{1.0f, "VertexScaleY", "サンプル点スケールY", &mParamObj};
    f32 mLuminanceScale = 2.0f;
    f32 mLuminanceRange = 1.0f;
    f32 mLuminanceMax = 10.0f;
    f32 mLuminanceMin = 0.0f;
};
static_assert(sizeof(AutoExposure) == 0x9d8);

}  // namespace agl::pfx

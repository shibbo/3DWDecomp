#pragma once

#include <container/seadSafeArray.h>
#include <gfx/seadColor.h>
#include <hostio/seadHostIONode.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include "common/aglRenderBuffer.h"
#include "common/aglRenderTarget.h"
#include "common/aglTextureData.h"
#include "common/aglTextureSampler.h"
#include "environment/aglEnvObj.h"
#include "utility/aglContextParameterBuffer.h"
#include "utility/aglDebugTexturePage.h"
#include "utility/aglDevTools.h"
#include "utility/aglDynamicTextureCache.h"
#include "utility/aglParameter.h"
#include "utility/aglParameterIO.h"

namespace sead {
class Heap;
class Viewport;
namespace hostio {
class Context;
class PropertyEvent;
class Reflexible;
}  // namespace hostio
}  // namespace sead

namespace agl {
class DrawContext;
class ShaderProgram;
}  // namespace agl

namespace agl::pfx {

// TODO
class BloomObj : public env::EnvObj {
public:
    static const env::TypeInfo* sTypeInfo;
};

class BloomParameter {
public:
    class Unit {
    public:
        void genMessage(sead::hostio::Context* pContext, bool isEditExpand);
        void getUniformThreshold(sead::Vector4f* pThreshold, sead::Vector4f* pBalance, f32 scale,
                                 const sead::Vector3f& rBalance) const;

        utl::Parameter<f32> mThreshold;
        utl::Parameter<f32> mThresholdRange;
        utl::Parameter<f32> mIntensity;
        utl::Parameter<sead::Color4f> mFinalGather;
        utl::Parameter<f32> mExpand;
    };
    static_assert(sizeof(Unit) == 0xa8);

    class Depth {
    public:
        utl::Parameter<bool> mEnable;
        utl::Parameter<f32> mStart;
        utl::Parameter<f32> mEnd;
        utl::Parameter<f32> mValue;
        utl::Parameter<f32> mValueStart;
    };
    static_assert(sizeof(Depth) == 0xa0);

    enum DepthType {
        cDepth_Gain,
        cDepth_Offset,
        cDepth_Shaft,
        cDepth_Num
    };

    static constexpr s32 cColorNum = 4;

    BloomParameter();

    void initialize(utl::IParameterObj* pObj, sead::Heap* pHeap);
    void genMessageBloomParameter(sead::hostio::Context* pContext);
    void listenPropertyEventBloomParameter(sead::hostio::Reflexible* pReflexible,
                                           const sead::hostio::PropertyEvent* pEvent);

    Unit mMain;
    Unit mShaft;
    sead::UnsafeArray<Depth, cDepth_Num> mDepths;
    utl::Parameter<sead::Color4f> mColors[cColorNum];
    utl::Parameter<s32> mEditType;
    utl::Parameter<s32> mFinalBlend;
    utl::Parameter<sead::Color4f> mThresholdBalance;
    utl::Parameter<s32> mExType;
    utl::Parameter<s32> mExIteration;
    utl::Parameter<bool> mEnableDepthClamp;
    utl::Parameter<bool> mEnableLuminanceOffset;
    utl::Parameter<bool> mEnableClampedLuminance;
    utl::Parameter<f32> mLuminanceIntensity;
    utl::Parameter<f32> mAspect;
    utl::Parameter<f32> mClampedLuminance;
    s32 mBalanceType = 0;
    u16 mDebugFlags = 0;
    sead::Vector3f mBalance;

protected:
    void updateBalance_();

private:
    static void requestGenMessage_(sead::hostio::Reflexible* pReflexible) {}
};
static_assert(sizeof(BloomParameter) == 0x550);

class Bloom : public utl::ContextParameterBuffer<Bloom, BloomParameter>,
              public utl::IParameterIO,
              public sead::hostio::Node {
public:
    struct DrawArg {
        const RenderBuffer* mpRenderBuffer;
        const sead::Viewport* mpViewport;
        const TextureData* mpColor;
        const TextureData* mpDepth;
        bool mIsLinearDepth;
        const TextureData* mpMask;
    };

    class MRT {
    public:
        MRT();

        void free(utl::DynamicTextureCache* pCache);
        void entry(DrawContext* pDrawContext, s32 context,
                   const utl::DebugTexturePage& rPage) const;

        TextureData* mTextures[5];
        TextureSampler mSampler;
        RenderTargetColor mTarget;
    };
    static_assert(sizeof(MRT) == 0x310);

    struct Context {
        TextureSampler mResultSampler;
        TextureSampler mDepthSampler;
        TextureSampler mColorSampler;
        TextureSampler mMaskSampler;
        RenderBuffer mRenderBuffer;
        MRT mMRTs[2];
        const TextureSampler* mpAddSampler;
        f32 mResolution;
        sead::Vector2f mScale;
        f32 mThresholdScale;
        f32 mNear;
        f32 mFar;
        utl::DynamicTextureCache mTextureCache;
    };
    static_assert(sizeof(Context) == 0xc98);

    enum Flag {
        cFlag_NoComposite = 1 << 0,
        cFlag_IgnoreDepth = 1 << 1,
        cFlag_NoBlend = 1 << 2,
        cFlag_NoGaussian = 1 << 3,
        cFlag_NoGather = 1 << 4,
        cFlag_NoFinalGather = 1 << 5,
        cFlag_UseMipLevel = 1 << 6,
        cFlag_Reduce = 1 << 7,
    };

    Bloom();
    ~Bloom() override;

    void initialize(s32 contextNum, sead::Heap* pHeap);
    void initializeContext(Context* pContext, sead::Heap* pHeap);
    void update();
    void calcGPU() const;
    void calcGPU(s32 context) const;
    void setNearFar(s32 context, f32 near, f32 far);
    void draw(DrawContext* pDrawContext, s32 context, const DrawArg& rArg) const;
    void drawToBloomBuffer(DrawContext* pDrawContext, s32 context, const DrawArg& rArg) const;
    void releaseBloomBuffer(s32 context) const;
    void genMessage(sead::hostio::Context* pContext);
    void listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent);

    Context& getContext(s32 context) const { return getContext_(context); }

    void setEnable(bool isEnable) { *mEnable = isEnable; }

    bool isEnable() const { return *mEnable && isEnableContext(-1); }
    bool isEnable(s32 context) const { return *mEnable && isEnableContext(context); }

protected:
    void postRead_() override;
    void callbackNotAppliable_(utl::IParameterObj* pObj, utl::ParameterBase* pParam,
                               utl::ResParameterObj obj) override;
    void callbackInvalidVersion_(utl::ResParameterArchive archive) override;

private:
    void draw_(DrawContext* pDrawContext, s32 context, const DrawArg& rArg, bool isBuffer) const;
    void drawTexture_(DrawContext* pDrawContext, const ShaderProgram& rProgram,
                      const TextureSampler& rSampler) const;
    void drawDetect_(DrawContext* pDrawContext, s32 context, bool isLinearDepth) const;
    void drawGaussian_(DrawContext* pDrawContext, s32 context, s32 level, f32 scale) const;
    void drawGather_(DrawContext* pDrawContext, const TextureSampler& rSampler,
                     const sead::Color4f& rColor, const sead::Color4f& rConstantColor) const;
    void drawShaft_(DrawContext* pDrawContext, s32 context) const;
    void drawDepthDepth_(DrawContext* pDrawContext, s32 context, s32 index,
                         const RenderBuffer& rRenderBuffer) const;

    utl::Parameter<bool> mEnable;
    sead::BitFlag16 mFlags = cFlag_UseMipLevel;
    utl::DebugTexturePage mDebugTexturePage;
};
static_assert(sizeof(Bloom) == 0xa40);

}  // namespace agl::pfx

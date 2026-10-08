#pragma once

#include <container/seadBuffer.h>
#include <gfx/seadColor.h>
#include <gfx/seadGraphicsContext.h>
#include <hostio/seadHostIOCurve.h>
#include <hostio/seadHostIONode.h>
#include <math/seadVector.h>
#include <prim/seadBitFlag.h>
#include "common/aglGPUMemAddr.h"
#include "common/aglRenderBuffer.h"
#include "common/aglRenderTarget.h"
#include "common/aglTextureData.h"
#include "common/aglTextureSampler.h"
#include "postfx/aglProcLUT.h"
#include "utility/aglParameter.h"
#include "utility/aglParameterIO.h"
#include "utility/aglParameterObj.h"

namespace sead {
class Heap;
namespace hostio {
class Context;
class NodeEvent;
class PropertyEvent;
}  // namespace hostio
}  // namespace sead

namespace agl {
class DrawContext;
namespace utl {
class DebugTexturePage;
}
}  // namespace agl

namespace agl::pfx {

class ColorCorrection : public sead::hostio::Node, public utl::IParameterIO {
public:
    enum Flag {
        cFlag_UpdateMap = 1 << 0,
        cFlag_UpdateProgram = 1 << 1,
        cFlag_Hue = 1 << 2,
        cFlag_Saturation = 1 << 3,
        cFlag_Brightness = 1 << 4,
        cFlag_Gamma = 1 << 5,
        cFlag_Level = 1 << 6,
        cFlag_ToyCamera = 1 << 7,
        cFlag_ToyCameraFirst = 1 << 8,
        cFlag_ForceUpdateMap = 1 << 16,
        cFlag_Loaded = 1 << 17,
    };

    static constexpr s32 cMapSize = 8;
    static constexpr s32 cLevelTableNum = 9;

    ColorCorrection();
    virtual ~ColorCorrection();

    void resetAll();
    void initialize(s32 contextNum, sead::Heap* pHeap, bool unused);
    void drawMap(DrawContext* pDrawContext) const;
    void draw(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer) const;
    void draw(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
              const TextureData& rTexture) const;
    void draw(DrawContext* pDrawContext, s32 context, const RenderBuffer& rRenderBuffer,
              const TextureSampler& rSampler) const;

    void genMessage(sead::hostio::Context* pContext);
    void genMessageParameters(sead::hostio::Context* pContext);
    void genMessageToyCameraParameters(sead::hostio::Context* pContext);
    void genMessageHsbParameters(sead::hostio::Context* pContext);
    void listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent);
    void listenNodeEvent(const sead::hostio::NodeEvent* pEvent);

    void setHue(f32 hue);
    void setSaturation(f32 saturation);
    void setBrightness(f32 brightness);
    void setGamma(f32 gamma);

    bool isEnable() const { return *mEnable; }
    s32 getVariationIndex() const { return mVariationIndex; }
    void setEnable(bool enable) { *mEnable = enable; }
    const sead::Vector2f& getMapScaleOffset() const { return mMapScaleOffset; }
    const TextureSampler& getMapSampler() const { return mMapSampler; }

protected:
    void postRead_() override;

private:
    void destroy_();
    void updateProgram_() const;
    void updateCurves_();
    void updateFlags_();
    bool enablePassHue_() const;
    bool enablePassSaturation_() const;
    bool enablePassGamma_() const;
    bool enablePassBrightness_() const;
    void setLevelCurve_(u32 index, sead::hostio::CurveType type, const f32* pData, u32 num);
    void updateMapCPU_();
    void convRGB_(u32* pDst, f32 r, f32 g, f32 b) const;
    void calcToyCamera_(f32* pR, f32* pG, f32* pB) const;
    f32 calcHue_(f32 r, f32 g, f32 b) const;
    void calcRGB_(u32* pDst, f32 h, f32 s, f32 v) const;
    void calcSaturation_(f32* pR, f32* pG, f32* pB, f32 saturation) const;
    void calcContrast_(f32* pValue, f32 contrast, f32 brightness) const;

    utl::ParameterObj mParamObj;
    utl::Parameter<bool> mEnable{true, "enable", "Enable", &mParamObj};
    utl::Parameter<f32> mHue{0.0f, "hue", "Hue", "Min=-180, Max=180", &mParamObj};
    utl::Parameter<f32> mSaturation{1.0f, "saturation", "Saturation", "Min=0, Max=2", &mParamObj};
    utl::Parameter<f32> mBrightness{1.0f, "brightness", "Brightness", "Min=0, Max=2", &mParamObj};
    utl::Parameter<f32> mGamma{1.0f, "gamma", "Gamma", "Min=0.25, Max=4", &mParamObj};
    utl::Parameter<bool> mToyCameraFirst{false, "order_toycam_hsb",
                                         "トイカメラフィルタを初めに処理する", &mParamObj};
    utl::Parameter<bool> mToyCameraEnable{false, "toycam_enable", "トイカメラフィルタ", &mParamObj};
    utl::Parameter<sead::Color4f> mToyCameraOffset1{sead::Color4f::cBlack, "toycam_offset1",
                                                    "トーンオフセット１", "Min=0, Max=1",
                                                    &mParamObj};
    utl::Parameter<sead::Color4f> mToyCameraOffset2{sead::Color4f::cBlack, "toycam_offset2",
                                                    "トーンオフセット２", "Min=0, Max=1",
                                                    &mParamObj};
    utl::Parameter<sead::Color4f> mToyCameraLevel1{sead::Color4f::cWhite, "toycam_level1",
                                                   "レベル補正１", "Min=0, Max=2", &mParamObj};
    utl::Parameter<sead::Color4f> mToyCameraLevel2{sead::Color4f::cWhite, "toycam_level2",
                                                   "レベル補正２", "Min=0, Max=2", &mParamObj};
    utl::Parameter<f32> mToyCameraSaturation1{1.0f, "toycam_saturation1", "彩度１",
                                              "Min=0, Max=2", &mParamObj};
    utl::Parameter<f32> mToyCameraSaturation2{1.0f, "toycam_saturation2", "彩度２",
                                              "Min=0, Max=2", &mParamObj};
    utl::Parameter<f32> mToyCameraBrightness{1.0f, "toycam_brightness", "明度", "Min=0, Max=2",
                                             &mParamObj};
    utl::Parameter<f32> mToyCameraContrast{1.0f, "toycam_contrast", "コントラスト",
                                           "Min=0, Max=2", &mParamObj};
    utl::Parameter<sead::Color4f> mToyCameraMulColor{sead::Color4f::cWhite, "toycam_mul_color",
                                                     "乗算カラー", "Min=0, Max=1", &mParamObj};
    sead::GraphicsContext mGraphicsContext;
    sead::Vector2f mMapScaleOffset{0.0f, 0.0f};
    RenderBuffer mRenderBuffer;
    RenderTargetColor mRenderTarget[cMapSize];
    TextureData mMapTexture;
    TextureSampler mMapSampler;
    GPUMemAddr<u8> mMapImage;
    sead::Buffer<TextureSampler> mSamplers;
    mutable s32 mVariationIndex = 0;
    sead::Vector4f mLevelTable[cLevelTableNum];
    proc::LUT mLevelCurve{"level", "レベル補正", &mParamObj};
    utl::DebugTexturePage* mDebugTexturePage = nullptr;
    mutable sead::BitFlag32 mFlags = 0;
};
static_assert(sizeof(ColorCorrection) == 0x1710);

}  // namespace agl::pfx

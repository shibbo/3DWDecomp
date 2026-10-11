#pragma once

#include <basis/seadTypes.h>
#include <common/aglGPUMemBlock.h>
#include <container/seadSafeArray.h>
#include <gfx/seadColor.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Yaml/ParameterBase.hpp"
#include "Project/Base/ParamRequestInterp.hpp"

namespace agl {
class DrawContext;
class IndexStream;
class RenderBuffer;
class ShaderProgram;
class TextureData;
class VertexAttribute;
class VertexBuffer;
}  // namespace agl

namespace agl::fx {
class RadialBlur;
}

namespace nn::g3d {
class ResFile;
}

namespace al {
class NoiseTextureKeeper;
class PlayerHolder;
class SceneCameraInfo;
class ShaderHolder;
class SimpleModelEnv;
class UniformBlock;

class VignettingParam : public IUseRequestParam {
public:
    VignettingParam();

    const char* getParamName() const override { return "Vignetting Draw"; }
    ParameterObj* getParamObj() override { return mParamObj; }
    const ParameterObj* getParamObj() const override { return mParamObj; }

    bool isBlurEnable() const { return mIsBlurEnable->getValue(); }
    bool isBlurPlayerEnable() const { return mIsBlurPlayerEnable->getValue(); }
    s32 getBlurType() const { return mBlurType->getValue(); }
    f32 getBlurRange() const { return mBlurRange->getValue(); }
    f32 getBlurChangeRange() const { return mBlurChangeRange->getValue(); }
    f32 getBlurPower() const { return mBlurPower->getValue(); }
    f32 getBlurPowerMax() const { return mBlurPowerMax->getValue(); }
    const sead::Vector2f& getBlurScale() const { return mBlurScale->getValue(); }
    const sead::Vector2f& getBlurOffset() const { return mBlurOffset->getValue(); }
    s32 getBlurQuality() const { return mBlurQuality->getValue(); }
    bool isColorEnable() const { return mIsColorEnable->getValue(); }
    bool isColorPlayerEnable() const { return mIsColorPlayerEnable->getValue(); }
    s32 getColorType() const { return mColorType->getValue(); }
    f32 getColorRange() const { return mColorRange->getValue(); }
    f32 getColorChangeRange() const { return mColorChangeRange->getValue(); }
    const sead::Vector2f& getColorScale() const { return mColorScale->getValue(); }
    const sead::Vector2f& getColorOffset() const { return mColorOffset->getValue(); }
    const sead::Color4f& getColor() const { return mColor->getValue(); }
    s32 getColorBlendType() const { return mColorBlendType->getValue(); }

private:
    ParameterObj* mParamObj;
    Parameter<bool>* mIsBlurEnable;
    Parameter<bool>* mIsBlurPlayerEnable;
    Parameter<s32>* mBlurType;
    Parameter<f32>* mBlurRange;
    Parameter<f32>* mBlurChangeRange;
    Parameter<f32>* mBlurPower;
    Parameter<f32>* mBlurPowerMax;
    Parameter<sead::Vector2f>* mBlurScale;
    Parameter<sead::Vector2f>* mBlurOffset;
    Parameter<s32>* mBlurQuality;
    Parameter<bool>* mIsColorEnable;
    Parameter<bool>* mIsColorPlayerEnable;
    Parameter<s32>* mColorType;
    Parameter<f32>* mColorRange;
    Parameter<f32>* mColorChangeRange;
    Parameter<sead::Vector2f>* mColorScale;
    Parameter<sead::Vector2f>* mColorOffset;
    Parameter<sead::Color4f>* mColor;
    Parameter<s32>* mColorBlendType;
};

class VignettingDrawer {
public:
    struct Vertex {
        sead::Vector2f mPos;
        sead::Vector2f mParam;
    };

    struct Mesh {
        agl::GPUMemBlock<Vertex> mVertexBlock;
        agl::GPUMemBlock<u16> mIndexBlock;
        agl::VertexBuffer* mVertexBuffer = nullptr;
        agl::VertexAttribute* mVertexAttribute = nullptr;
        agl::IndexStream* mIndexStream = nullptr;
    };

    struct ScreenScaleInfo {
        bool mIsValid;
        bool mIsEnable;
    };

    VignettingDrawer(ShaderHolder* pShaderHolder, const PlayerHolder* pPlayerHolder,
                     const SceneCameraInfo* pCameraInfo);
    ~VignettingDrawer();
    void endInit();
    void clearRequest();
    void update();
    void requestParam(s32 priority, s32 step, const VignettingParam& rParam);
    void draw(agl::DrawContext* pContext, const agl::RenderBuffer& rBuffer) const;
    const VignettingParam* getCurrentParam() const;
    bool isEnableBlur() const;
    agl::TextureData* allocBuffer(agl::DrawContext* pContext, const agl::TextureData& rSrc,
                                  const VignettingParam* pParam) const;
    void drawBlur(agl::DrawContext* pContext, agl::TextureData* pBlur,
                  const agl::TextureData& rSrc, const VignettingParam* pParam) const;
    void drawCompose(agl::DrawContext* pContext, agl::TextureData* pBlur,
                     const agl::RenderBuffer& rBuffer, const VignettingParam* pParam) const;
    void freeBuffer(agl::TextureData* pTexture) const;
    bool isEnableColor() const;
    void drawVignetting(agl::DrawContext* pContext, const agl::RenderBuffer& rBuffer,
                        const VignettingParam* pParam) const;
    void drawBlurMap(agl::DrawContext* pContext, const agl::TextureData& rSrc) const;
    void freeBlurMap() const;
    void setupCompose(agl::DrawContext* pContext, const agl::ShaderProgram* pProgram);

private:
    bool tryCalcPlayerScreenPos(sead::Vector2f* pPos) const;

    bool mIsEnableBlur = true;
    bool mIsEnableColor = true;
    ParamRequestInterp* mRequestInterp;
    agl::ShaderProgram* mShaderProgram = nullptr;
    const PlayerHolder* mPlayerHolder;
    const SceneCameraInfo* mCameraInfo;
    const ScreenScaleInfo* mScreenScaleInfo = nullptr;
    sead::SafeArray<Mesh, 2> mMeshes;
    mutable agl::TextureData* mBlurMap = nullptr;
};

static_assert(sizeof(VignettingDrawer) == 0x148);
}  // namespace al

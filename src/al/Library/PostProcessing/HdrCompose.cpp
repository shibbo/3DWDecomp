#include "Library/PostProcessing/HdrCompose.hpp"

#include <common/aglShaderLocation.h>
#include <common/aglShaderProgram.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <postfx/aglBloom.h>
#include <postfx/aglColorCorrection.h>
#include <postfx/aglFlareFilter.h>
#include <prim/seadEnum.h>
#include <prim/seadScopedLock.h>
#include <utility/aglDynamicTextureAllocator.h>
#include <utility/aglPrimitiveTexture.h>

#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Light/LightIntensityDirector.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/PostProcessing/FlareFilterDirector.hpp"
#include "Library/PostProcessing/GodRayDirector.hpp"
#include "Library/PostProcessing/LightStreakDirector.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Texture/TextureUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/PostProcessing/PostProcessingDrawerUtil.hpp"

namespace {

/**
 * Names of the camera mask textures, indexed by the CameraMaskTex parameter.
 */
class CameraMaskTexName {
public:
    /**
     * Gets the name of a camera mask texture.
     * @param index Texture index.
     * @return Texture name, or nullptr if the index is out of range.
     */
    static const char* text(s32 index) {
        if (static_cast<u32>(index) >= cCount) {
            return nullptr;
        }

        static char** spTextPtr = nullptr;

        if (spTextPtr != nullptr) {
            return spTextPtr[index];
        }

        {
            sead::ScopedLock<sead::CriticalSection> lock(sead::EnumUtil::getParseTextCS_());

            if (spTextPtr == nullptr) {
                static char* sTextPtr[cCount];
                static sead::FixedSafeString<sizeof(cTextAll)> sTextAll =
                    sead::SafeString(cTextAll);
                sead::EnumUtil::parseText_(sTextPtr, sTextAll.getBuffer(), cCount);
                spTextPtr = sTextPtr;
            }
        }

        return spTextPtr[index];
    }

private:
    static constexpr char cTextAll[] =
        "Do Not Use, TextureCameraMaskEdge, TextureCameraMaskDesert, TextureCameraMaskFrost, "
        "TextureCameraMaskFrozen";
    static constexpr s32 cCount = sead::EnumUtil::countValues(cTextAll, sizeof(cTextAll));
};

/**
 * Names of the camera indirect textures, indexed by the CameraIndirectTex parameters.
 */
class CameraIndirectTexName {
public:
    /**
     * Gets the name of a camera indirect texture.
     * @param index Texture index.
     * @return Texture name, or nullptr if the index is out of range.
     */
    static const char* text(s32 index) {
        if (static_cast<u32>(index) >= cCount) {
            return nullptr;
        }

        static char** spTextPtr = nullptr;

        if (spTextPtr != nullptr) {
            return spTextPtr[index];
        }

        {
            sead::ScopedLock<sead::CriticalSection> lock(sead::EnumUtil::getParseTextCS_());

            if (spTextPtr == nullptr) {
                static char* sTextPtr[cCount];
                static sead::FixedSafeString<sizeof(cTextAll)> sTextAll =
                    sead::SafeString(cTextAll);
                sead::EnumUtil::parseText_(sTextPtr, sTextAll.getBuffer(), cCount);
                spTextPtr = sTextPtr;
            }
        }

        return spTextPtr[index];
    }

private:
    static constexpr char cTextAll[] =
        "Do Not Use, TextureCameraIndirectWave, TextureCameraIndirectWaterDrop, "
        "TextureCameraIndirectFrozen, TextureCameraIndirectWaterDrop2";
    static constexpr s32 cCount = sead::EnumUtil::countValues(cTextAll, sizeof(cTextAll));
};

/**
 * Loads a camera mask texture from its archive.
 * @param type Camera mask texture index.
 * @return The loaded texture.
 */
agl::TextureData* createCameraMaskTexture(s32 type) {
    agl::TextureData* textureData = new agl::TextureData();
    const char* name = CameraMaskTexName::text(type);
    al::StringTmp<128> archiveName("ObjectData/%s", name);
    al::StringTmp<128> fileName("%s", name);
    al::makeTextureDataFromArchive(textureData, archiveName.cstr(), fileName.cstr(), name);
    return textureData;
}

/**
 * Loads a camera indirect texture from its archive.
 * @param type Camera indirect texture index.
 * @return The loaded texture.
 */
agl::TextureData* createCameraIndirectTexture(s32 type) {
    agl::TextureData* textureData = new agl::TextureData();
    const char* name = CameraIndirectTexName::text(type);
    al::StringTmp<128> archiveName("ObjectData/%s", name);
    al::StringTmp<128> fileName("%s", name);
    al::makeTextureDataFromArchive(textureData, archiveName.cstr(), fileName.cstr(), name);
    return textureData;
}

/**
 * Clears a texture to black.
 * @param pTextureData Texture to clear.
 */
void clearTexture(agl::TextureData* pTextureData);

const al::UniformBlockLayout cHdrComposeInfoLayout[] = {
    {0, agl::UniformBlock::cType_Vec4, 1},   {1, agl::UniformBlock::cType_Float, 1},
    {2, agl::UniformBlock::cType_Float, 1},  {3, agl::UniformBlock::cType_Float, 1},
    {4, agl::UniformBlock::cType_Float, 1},  {5, agl::UniformBlock::cType_Float, 1},
    {6, agl::UniformBlock::cType_Float, 1},  {7, agl::UniformBlock::cType_Vec2, 1},
    {8, agl::UniformBlock::cType_Vec2, 1},   {9, agl::UniformBlock::cType_Vec2, 1},
    {10, agl::UniformBlock::cType_Vec2, 1},  {11, agl::UniformBlock::cType_Vec2, 1},
    {12, agl::UniformBlock::cType_Vec3, 1},  {13, agl::UniformBlock::cType_Vec2, 1},
    {14, agl::UniformBlock::cType_Float, 1}, {15, agl::UniformBlock::cType_Float, 1},
};

}  // namespace

namespace al {

/**
 * Checks whether this compose is used instead of the agl one.
 * @return Always true.
 */
bool HdrCompose::isUsingMyHdrCompose() const {
    return true;
}

/**
 * Initializes all HDR compose parameters with their default values.
 */
void HdrParam::init() {
    mCameraMaskTex.init(0, "CameraMaskTex", "CameraMaskTex", "Min=0,Max=10", &mParamObj);
    mToneMapType.init(0, "ToneMapType", "ToneMapType", "Min=0,Max=0", &mParamObj);
    mExposureType.init(0, "ExposureType", "ExposureType", "Min=0, Max=1", &mParamObj);
    mIsMaskBloom.init(false, "IsMaskBloom", "IsMaskBloom", &mParamObj);
    mIsMaskLightStreak.init(false, "IsMaskLightStreak", "IsMaskLightStreak", &mParamObj);
    mIsMaskGodRay.init(false, "IsMaskGodRay", "IsMaskGodRay", &mParamObj);
    mCameraMaskBase.init(0.0f, "CameraMaskBase", "CameraMaskBase", "Min=0.0f, Max=1.0f",
                         &mParamObj);
    mCameraMaskScale.init(1.0f, "CameraMaskScale", "CameraMaskScale", "Min=0.0f, Max=5.0f",
                          &mParamObj);
    mCameraMaskDiffuse.init(sead::Color4f::cBlack, "CameraMaskDiffuse", "CameraMaskDiffuse",
                            "Min=0, Max=1", &mParamObj);
    mCameraIndirectTex.init(0, "CameraIndirectTex", "CameraIndirectTex", "Min=0,Max=10",
                            &mParamObj);
    mCameraIndirectScale.init(0.01f, "CameraIndirectScale", "CameraIndirectScale",
                              "Min=-0.1, Max=0.1", &mParamObj);
    mCameraIndirectTexScale.init(sead::Vector2f(1.0f, 1.0f), "CameraIndirectTexScale",
                                 "CameraIndirectTexScale", "Min=0.0f, Max=5.0f", &mParamObj);
    mCameraIndirectOffsetVelocity.init(sead::Vector2f(0.0f, 0.0f), "CameraIndirectOffsetVelocity",
                                       "CameraIndirectOffsetVelocity", "Min=-0.1f, Max=0.1f",
                                       &mParamObj);
    mChromaticAberrationSize.init(0.25f, "ChromaticAberrationSize", "ChromaticAberrationSize",
                                  "Min=0.0f, Max=1.0f", &mParamObj);
    mCameraIndirect2Tex.init(0, "CameraIndirect2Tex", "CameraIndirect2Tex", "Min=0,Max=10",
                             &mParamObj);
    mCameraIndirect2Scale.init(0.01f, "CameraIndirect2Scale", "CameraIndirect2Scale",
                               "Min=-0.1, Max=0.1", &mParamObj);
    mCameraIndirect2TexScale.init(sead::Vector2f(1.0f, 1.0f), "CameraIndirect2TexScale",
                                  "CameraIndirect2TexScale", "Min=0.0f, Max=5.0f", &mParamObj);
    mCameraIndirect2OffsetVelocity.init(
        sead::Vector2f(0.0f, 0.0f), "CameraIndirect2OffsetVelocity",
        "CameraIndirect2OffsetVelocity", "Min=-0.1f, Max=0.1f", &mParamObj);
    mCameraIndirect2Usage.init(0, "CameraIndirect2Usage", "CameraIndirect2Usage", "Min=0, Max=1",
                               &mParamObj);
    mAutoExposureMid.init(0.008f, "AutoExposureMid", "AutoExposureMid", "Min=0.0f, Max=0.01f",
                          &mParamObj);
    mAutoExposureRangeMax.init(20.0f, "AutoExposureRangeMax", "AutoExposureRangeMax",
                               "Min=0.0f, Max=10.0f", &mParamObj);
    mAutoExposureRangeMin.init(0.0f, "AutoExposureRangeMin", "AutoExposureRangeMin",
                               "Min=0.0f, Max=10.0f", &mParamObj);
    mAutoExposureIgnoreRangeMax.init(20.0f, "AutoExposureIgnoreRangeMax",
                                     "AutoExposureIgnoreRangeMax", "Min=0.0f, Max=10.0f",
                                     &mParamObj);
    mAutoExposureBlendRateUp.init(0.1f, "AutoExposureBlendRateUp", "AutoExposureBlendRateUp",
                                  "Min=0.0f, Max=1.0f", &mParamObj);
    mAutoExposureBlendRateDown.init(0.025f, "AutoExposureBlendRateDown",
                                    "AutoExposureBlendRateDown", "Min=0.0f, Max=1.0f",
                                    &mParamObj);
    mAutoExposureIgnoreRangeMin.init(0.0f, "AutoExposureIgnoreRangeMin",
                                     "AutoExposureIgnoreRangeMin", "Min=0.0f, Max=10.0f",
                                     &mParamObj);
    mAutoExposureHistogramScale.init(1.0f, "AutoExposureHistogramScale",
                                     "AutoExposureHistogramScale", "Min=0.0f, Max=5.0f",
                                     &mParamObj);
}

/**
 * Compares all parameter values with another HDR compose parameter.
 * @param rOther Parameter to compare with.
 * @return Whether all values are equal.
 */
bool HdrParam::operator==(const HdrParam& rOther) const {
    return *mCameraMaskTex == *rOther.mCameraMaskTex &&
           *mCameraIndirectTex == *rOther.mCameraIndirectTex &&
           *mCameraIndirectScale == *rOther.mCameraIndirectScale &&
           *mCameraIndirectTexScale == *rOther.mCameraIndirectTexScale &&
           *mCameraIndirectOffsetVelocity == *rOther.mCameraIndirectOffsetVelocity &&
           *mCameraIndirect2Tex == *rOther.mCameraIndirect2Tex &&
           *mCameraIndirect2Scale == *rOther.mCameraIndirect2Scale &&
           *mCameraIndirect2TexScale == *rOther.mCameraIndirect2TexScale &&
           *mCameraIndirect2OffsetVelocity == *rOther.mCameraIndirect2OffsetVelocity &&
           *mChromaticAberrationSize == *rOther.mChromaticAberrationSize &&
           *mToneMapType == *rOther.mToneMapType &&
           *mExposureType == *rOther.mExposureType &&
           *mIsMaskBloom == *rOther.mIsMaskBloom &&
           *mIsMaskLightStreak == *rOther.mIsMaskLightStreak &&
           *mIsMaskGodRay == *rOther.mIsMaskGodRay &&
           *mCameraMaskBase == *rOther.mCameraMaskBase &&
           *mCameraMaskScale == *rOther.mCameraMaskScale &&
           *mCameraMaskDiffuse == *rOther.mCameraMaskDiffuse &&
           *mAutoExposureMid == *rOther.mAutoExposureMid &&
           *mAutoExposureRangeMax == *rOther.mAutoExposureRangeMax &&
           *mAutoExposureRangeMin == *rOther.mAutoExposureRangeMin &&
           *mAutoExposureIgnoreRangeMax == *rOther.mAutoExposureIgnoreRangeMax &&
           *mAutoExposureIgnoreRangeMin == *rOther.mAutoExposureIgnoreRangeMin &&
           *mAutoExposureBlendRateUp == *rOther.mAutoExposureBlendRateUp &&
           *mAutoExposureBlendRateDown == *rOther.mAutoExposureBlendRateDown &&
           *mAutoExposureHistogramScale == *rOther.mAutoExposureHistogramScale;
}

/**
 * Copies all parameter values from another HDR compose parameter.
 * @param rOther Parameter to copy from.
 * @return This parameter.
 */
HdrParam& HdrParam::operator=(const HdrParam& rOther) {
    *mCameraMaskTex = *rOther.mCameraMaskTex;
    *mToneMapType = *rOther.mToneMapType;
    *mExposureType = *rOther.mExposureType;
    *mIsMaskBloom = *rOther.mIsMaskBloom;
    *mIsMaskLightStreak = *rOther.mIsMaskLightStreak;
    *mIsMaskGodRay = *rOther.mIsMaskGodRay;
    *mCameraMaskBase = *rOther.mCameraMaskBase;
    *mCameraMaskScale = *rOther.mCameraMaskScale;
    *mAutoExposureMid = *rOther.mAutoExposureMid;
    *mAutoExposureRangeMax = *rOther.mAutoExposureRangeMax;
    *mAutoExposureRangeMin = *rOther.mAutoExposureRangeMin;
    *mAutoExposureIgnoreRangeMax = *rOther.mAutoExposureIgnoreRangeMax;
    *mAutoExposureIgnoreRangeMin = *rOther.mAutoExposureIgnoreRangeMin;
    *mAutoExposureBlendRateUp = *rOther.mAutoExposureBlendRateUp;
    *mAutoExposureBlendRateDown = *rOther.mAutoExposureBlendRateDown;
    *mAutoExposureHistogramScale = *rOther.mAutoExposureHistogramScale;
    *mCameraIndirectTex = *rOther.mCameraIndirectTex;
    *mCameraIndirectScale = *rOther.mCameraIndirectScale;
    *mCameraIndirectTexScale = *rOther.mCameraIndirectTexScale;
    *mCameraIndirectOffsetVelocity = *rOther.mCameraIndirectOffsetVelocity;
    *mCameraIndirect2Tex = *rOther.mCameraIndirect2Tex;
    *mCameraIndirect2Scale = *rOther.mCameraIndirect2Scale;
    *mCameraIndirect2TexScale = *rOther.mCameraIndirect2TexScale;
    *mCameraIndirect2OffsetVelocity = *rOther.mCameraIndirect2OffsetVelocity;
    *mCameraMaskDiffuse = *rOther.mCameraMaskDiffuse;
    *mChromaticAberrationSize = *rOther.mChromaticAberrationSize;
    return *this;
}

/**
 * Interpolates between two HDR compose parameters. Scales, colors and auto exposure values are
 * blended, texture selections and flags switch at the halfway point.
 * @param rA Parameter at rate 0.
 * @param rB Parameter at rate 1.
 * @param rate Interpolation rate.
 */
void HdrParam::interp(const HdrParam& rA, const HdrParam& rB, f32 rate) {
    mCameraMaskBase.copyLerp(rA.mCameraMaskBase, rB.mCameraMaskBase, rate);
    mCameraMaskScale.copyLerp(rA.mCameraMaskScale, rB.mCameraMaskScale, rate);
    mAutoExposureMid.copyLerp(rA.mAutoExposureMid, rB.mAutoExposureMid, rate);
    mAutoExposureRangeMax.copyLerp(rA.mAutoExposureRangeMax, rB.mAutoExposureRangeMax, rate);
    mAutoExposureRangeMin.copyLerp(rA.mAutoExposureRangeMin, rB.mAutoExposureRangeMin, rate);
    mAutoExposureIgnoreRangeMax.copyLerp(rA.mAutoExposureIgnoreRangeMax,
                                         rB.mAutoExposureIgnoreRangeMax, rate);
    mAutoExposureIgnoreRangeMin.copyLerp(rA.mAutoExposureIgnoreRangeMin,
                                         rB.mAutoExposureIgnoreRangeMin, rate);
    mAutoExposureBlendRateUp.copyLerp(rA.mAutoExposureBlendRateUp, rB.mAutoExposureBlendRateUp,
                                      rate);
    mAutoExposureBlendRateDown.copyLerp(rA.mAutoExposureBlendRateDown,
                                        rB.mAutoExposureBlendRateDown, rate);
    mAutoExposureHistogramScale.copyLerp(rA.mAutoExposureHistogramScale,
                                         rB.mAutoExposureHistogramScale, rate);
    mCameraIndirectScale.copyLerp(rA.mCameraIndirectScale, rB.mCameraIndirectScale, rate);
    mCameraIndirectTexScale.copyLerp(rA.mCameraIndirectTexScale, rB.mCameraIndirectTexScale,
                                     rate);
    mCameraMaskDiffuse.copyLerp(rA.mCameraMaskDiffuse, rB.mCameraMaskDiffuse, rate);
    mCameraIndirectOffsetVelocity.copy(rate < 0.5f ? rA.mCameraIndirectOffsetVelocity :
                                                     rB.mCameraIndirectOffsetVelocity);
    mCameraIndirectTex.copy(rate < 0.5f ? rA.mCameraIndirectTex : rB.mCameraIndirectTex);
    mChromaticAberrationSize.copyLerp(rA.mChromaticAberrationSize, rB.mChromaticAberrationSize,
                                      rate);
    mCameraIndirect2Scale.copyLerp(rA.mCameraIndirect2Scale, rB.mCameraIndirect2Scale, rate);
    mCameraIndirect2TexScale.copyLerp(rA.mCameraIndirect2TexScale, rB.mCameraIndirect2TexScale,
                                      rate);
    mCameraIndirect2OffsetVelocity.copy(rate < 0.5f ? rA.mCameraIndirect2OffsetVelocity :
                                                      rB.mCameraIndirect2OffsetVelocity);
    mCameraIndirect2Tex.copy(rate < 0.5f ? rA.mCameraIndirect2Tex : rB.mCameraIndirect2Tex);
    mCameraMaskTex.copy(rate < 0.5f ? rA.mCameraMaskTex : rB.mCameraMaskTex);
    mToneMapType.copy(rate < 0.5f ? rA.mToneMapType : rB.mToneMapType);
    mExposureType.copy(rate < 0.5f ? rA.mExposureType : rB.mExposureType);
    mIsMaskBloom.copy(rate < 0.5f ? rA.mIsMaskBloom : rB.mIsMaskBloom);
    mIsMaskLightStreak.copy(rate < 0.5f ? rA.mIsMaskLightStreak : rB.mIsMaskLightStreak);
    mIsMaskGodRay.copy(rate < 0.5f ? rA.mIsMaskGodRay : rB.mIsMaskGodRay);
}

/**
 * Creates the HDR compose, its auto exposure and one view context per view.
 * @param viewNum Number of views.
 * @param pInfo Graphics system info.
 */
HdrCompose::HdrCompose(s32 viewNum, GraphicsSystemInfo* pInfo)
    : GraphicsParamRequestInterpKeeper<HdrParam>(pInfo, 15, "HdrCompose", "aglhdrcompose",
                                                 nullptr),
      mShaderProgram(ShaderHolder::instance()->getShaderProgram("HdrCompose")) {
    mAutoExposure.initialize(viewNum, nullptr);
    mViewContexts.allocBuffer(viewNum, nullptr);

    for (s32 i = 0; i < viewNum; i++) {
        ViewContext* viewContext = new ViewContext();
        viewContext->mUniformBlock = createUniformBlock(cHdrComposeInfoLayout, 16, nullptr, 2);
        mViewContexts.pushBack(viewContext);
    }
}

/**
 * Destroys the view contexts and the loaded camera textures.
 */
HdrCompose::~HdrCompose() {
    while (!mViewContexts.isEmpty()) {
        ViewContext* viewContext = mViewContexts.popBack();

        if (viewContext->mUniformBlock != nullptr) {
            delete viewContext->mUniformBlock;
            viewContext->mUniformBlock = nullptr;
        }

        delete viewContext;
    }

    mViewContexts.freeBuffer();

    for (s32 i = 0; i < mCameraMaskTextures.size(); i++) {
        delete mCameraMaskTextures[i];
        mCameraMaskTextures[i] = nullptr;
    }

    mCameraMaskTextures.freeBuffer();

    for (s32 i = 0; i < mCameraIndirectTextures.size(); i++) {
        delete mCameraIndirectTextures[i];
        mCameraIndirectTextures[i] = nullptr;
    }

    mCameraIndirectTextures.freeBuffer();
}

/**
 * Updates the requested parameters, the auto exposure settings and the camera indirect scroll.
 * @param isPaused Whether the scene is paused, which stops the camera indirect scroll.
 */
void HdrCompose::movement(bool isPaused) {
    updateRequest();
    const HdrParam& param = getCurrentParam();

    if (mCameraIndirectTexType != param.getCameraIndirectTex()) {
        mCameraIndirectTexType = param.getCameraIndirectTex();
        mCameraIndirectOffset.e = sead::Vector2f::zero.e;
    }

    if (mCameraIndirect2TexType != param.getCameraIndirect2Tex()) {
        mCameraIndirect2TexType = param.getCameraIndirect2Tex();
        mCameraIndirect2Offset.e = sead::Vector2f::zero.e;
    }

    if (param.getExposureType() == 1) {
        mAutoExposure.setExposureMid(param.getAutoExposureMid());
        mAutoExposure.setRangeMax(param.getAutoExposureRangeMax());
        mAutoExposure.setRangeMin(param.getAutoExposureRangeMin());
        mAutoExposure.setLuminanceMax(param.getAutoExposureIgnoreRangeMax());
        mAutoExposure.setLuminanceMin(param.getAutoExposureIgnoreRangeMin());
        mAutoExposure.setBlendRateUp(param.getAutoExposureBlendRateUp());
        mAutoExposure.setBlendRateDown(param.getAutoExposureBlendRateDown());
        mAutoExposure.setLuminanceScale(param.getAutoExposureHistogramScale());
    }

    if (!isPaused) {
        mCameraIndirectOffset += param.getCameraIndirectOffsetVelocity();
        mCameraIndirectOffset.x = wrapValue(mCameraIndirectOffset.x, 1.0f);
        mCameraIndirectOffset.y = wrapValue(mCameraIndirectOffset.y, 1.0f);
        mCameraIndirect2Offset += param.getCameraIndirect2OffsetVelocity();
        mCameraIndirect2Offset.x = wrapValue(mCameraIndirect2Offset.x, 1.0f);
        mCameraIndirect2Offset.y = wrapValue(mCameraIndirect2Offset.y, 1.0f);
    }

    for (s32 i = 0; i < mViewContexts.size(); i++) {
        mViewContexts[i]->mUniformBlock->swap();
    }
}

/**
 * Prepares drawing. Nothing to do.
 */
void HdrCompose::preDrawGraphics() {}

/**
 * Applies the default parameter and loads every camera mask and camera indirect texture used by
 * the default or a named parameter.
 */
void HdrCompose::endInit() {
    GraphicsParamRequestInterpKeeper<HdrParam>::endInit();

    mCameraMaskTextures.tryAllocBuffer(4, nullptr);

    for (s32 i = 0; i < mCameraMaskTextures.size(); i++) {
        mCameraMaskTextures[i] = nullptr;
    }

    mCameraIndirectTextures.tryAllocBuffer(4, nullptr);

    for (s32 i = 0; i < mCameraIndirectTextures.size(); i++) {
        mCameraIndirectTextures[i] = nullptr;
    }

    s32 cameraMaskTex = mDefaultParam.getCameraMaskTex();

    if (cameraMaskTex > 0) {
        s32 index = cameraMaskTex - 1;
        mCameraMaskTextures[index] = createCameraMaskTexture(cameraMaskTex);
    }

    for (s32 i = 0; i < mNamedParams.size(); i++) {
        s32 namedCameraMaskTex = mNamedParams(i)->getCameraMaskTex();

        if (namedCameraMaskTex > 0 && mCameraMaskTextures[namedCameraMaskTex - 1] == nullptr) {
            mCameraMaskTextures[namedCameraMaskTex - 1] =
                createCameraMaskTexture(namedCameraMaskTex);
        }
    }

    s32 cameraIndirectTex = mDefaultParam.getCameraIndirectTex();

    if (cameraIndirectTex > 0) {
        s32 index = cameraIndirectTex - 1;
        mCameraIndirectTextures[index] = createCameraIndirectTexture(cameraIndirectTex);
    }

    s32 cameraIndirect2Tex = mDefaultParam.getCameraIndirect2Tex();

    if (cameraIndirect2Tex > 0) {
        s32 index = cameraIndirect2Tex - 1;
        mCameraIndirectTextures[index] = createCameraIndirectTexture(cameraIndirect2Tex);
    }

    for (s32 i = 0; i < mNamedParams.size(); i++) {
        s32 namedCameraIndirectTex = mNamedParams(i)->getCameraIndirectTex();

        if (namedCameraIndirectTex > 0 &&
            mCameraIndirectTextures[namedCameraIndirectTex - 1] == nullptr) {
            mCameraIndirectTextures[namedCameraIndirectTex - 1] =
                createCameraIndirectTexture(namedCameraIndirectTex);
        }
    }

    for (s32 i = 0; i < mNamedParams.size(); i++) {
        s32 namedCameraIndirect2Tex = mNamedParams(i)->getCameraIndirect2Tex();

        if (namedCameraIndirect2Tex > 0 &&
            mCameraIndirectTextures[namedCameraIndirect2Tex - 1] == nullptr) {
            mCameraIndirectTextures[namedCameraIndirect2Tex - 1] =
                createCameraIndirectTexture(namedCameraIndirect2Tex);
        }
    }
}

/**
 * Allocates the compose textures of a view and attaches them to the compose render buffers.
 * @param viewIndex View index.
 * @param rBuffer Render buffer the compose is drawn to.
 * @param pBloom Bloom of the view.
 */
void HdrCompose::setupComposeBuffer(s32 viewIndex, const agl::RenderBuffer& rBuffer,
                                    const agl::pfx::Bloom* pBloom) {
    agl::utl::DynamicTextureAllocator* allocator = agl::utl::DynamicTextureAllocator::instance();
    const agl::TextureData* colorTexture = rBuffer.getRenderTargetColor();
    u32 width = colorTexture->getWidth() / 4;
    s32 height = colorTexture->getHeight(0) / 4;
    bool isCompose = isAtLeastOneCompose(viewIndex, pBloom);
    bool isComposeMask = isAtLeastOneComposeMask(viewIndex, pBloom);

    if (isCompose) {
        mComposeTexture = allocator->alloc(GameFrameworkNx::getAglDrawContext(), "Compose",
                                           agl::TextureFormat::cTextureFormat_R11_G11_B10_float,
                                           width, height, 1, nullptr,
                                           agl::utl::DynamicTextureAllocator::cAllocateType_0,
                                           true, false);
        clearTexture(mComposeTexture);
        mComposeSampler.applyTextureData(*mComposeTexture);
    }

    if (getCurrentParam().getCameraMaskTex() != 0 && isComposeMask) {
        mComposeMaskTexture = allocator->alloc(
            GameFrameworkNx::getAglDrawContext(), "ComposeWithMask",
            agl::TextureFormat::cTextureFormat_R11_G11_B10_float, width, height, 1, nullptr,
            agl::utl::DynamicTextureAllocator::cAllocateType_0, true, false);
        clearTexture(mComposeMaskTexture);
        mComposeMaskSampler.applyTextureData(*mComposeMaskTexture);
    }

    mComposeTarget.applyTextureData(mComposeSampler.getTextureData());
    mComposeBuffer.setVirtualSize(sead::Vector2f(width, height));
    mComposeBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
    mComposeBuffer.setRenderTargetColorNullAll();
    mComposeBuffer.setRenderTargetColor(&mComposeTarget);

    mComposeMaskTarget.applyTextureData(mComposeMaskSampler.getTextureData());
    mComposeMaskBuffer.setVirtualSize(sead::Vector2f(width, height));
    mComposeMaskBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
    mComposeMaskBuffer.setRenderTargetColorNullAll();
    mComposeMaskBuffer.setRenderTargetColor(&mComposeMaskTarget);

    mAspectRatio = static_cast<f32>(colorTexture->getHeight(0)) /
                   static_cast<f32>(colorTexture->getWidth(0));
}

/**
 * Checks whether at least one post effect is composed without the camera mask.
 * @param viewIndex View index.
 * @param pBloom Bloom of the view.
 * @return Whether a compose texture is needed.
 */
bool HdrCompose::isAtLeastOneCompose(s32 viewIndex, const agl::pfx::Bloom* pBloom) const {
    const agl::pfx::FlareFilter* flareFilter =
        mGraphicsSystemInfo->mFlareFilterDirector->getParam();
    const GodRayDirector* godRayDirector = mGraphicsSystemInfo->mGodRayDirector;
    const LightStreakDirector* lightStreakDirector = mGraphicsSystemInfo->mLightStreakDirector;
    bool isEnableLightStreak =
        lightStreakDirector != nullptr ? lightStreakDirector->isEnable() : false;
    bool isEnableGodRay = godRayDirector != nullptr ? godRayDirector->isEnable() : false;
    bool isEnableBloom = pBloom != nullptr && pBloom->isEnable(viewIndex);
    bool isValidMask = isValidCameraMask();

    if (flareFilter != nullptr && flareFilter->isEnable(viewIndex) && !isValidMask) {
        return true;
    }

    if (isEnableLightStreak && !getCurrentParam().isMaskLightStreak()) {
        return true;
    }

    if (isEnableGodRay && !getCurrentParam().isMaskGodRay()) {
        return true;
    }

    return isEnableBloom && !getCurrentParam().isMaskBloom();
}

/**
 * Checks whether at least one post effect is composed with the camera mask.
 * @param viewIndex View index.
 * @param pBloom Bloom of the view.
 * @return Whether a compose with mask texture is needed.
 */
bool HdrCompose::isAtLeastOneComposeMask(s32 viewIndex, const agl::pfx::Bloom* pBloom) const {
    if (getCurrentParam().getCameraMaskTex() == 0) {
        return false;
    }

    const agl::pfx::FlareFilter* flareFilter =
        mGraphicsSystemInfo->mFlareFilterDirector->getParam();
    const GodRayDirector* godRayDirector = mGraphicsSystemInfo->mGodRayDirector;
    const LightStreakDirector* lightStreakDirector = mGraphicsSystemInfo->mLightStreakDirector;
    bool isEnableLightStreak =
        lightStreakDirector != nullptr ? lightStreakDirector->isEnable() : false;
    bool isEnableGodRay = godRayDirector != nullptr ? godRayDirector->isEnable() : false;
    bool isEnableBloom = pBloom != nullptr && pBloom->isEnable(viewIndex);
    bool isValidMask = isValidCameraMask();

    if (flareFilter != nullptr && flareFilter->isEnable(viewIndex) && isValidMask) {
        return true;
    }

    if (isEnableLightStreak && getCurrentParam().isMaskLightStreak()) {
        return true;
    }

    if (isEnableGodRay && getCurrentParam().isMaskGodRay()) {
        return true;
    }

    return isEnableBloom && getCurrentParam().isMaskBloom();
}

}  // namespace al

namespace {

/**
 * Clears a texture to black.
 * @param pTextureData Texture to clear.
 */
void clearTexture(agl::TextureData* pTextureData) {
    agl::RenderBuffer renderBuffer;
    agl::RenderTargetColor renderTarget;
    renderTarget.applyTextureData(*pTextureData);
    renderBuffer.setVirtualSize(
        sead::Vector2f(pTextureData->getWidth(0), pTextureData->getHeight(0)));
    renderBuffer.setPhysicalArea(
        sead::BoundBox2f(0.0f, 0.0f, pTextureData->getWidth(0), pTextureData->getHeight(0)));
    renderBuffer.setRenderTargetColorNullAll();
    renderBuffer.setRenderTargetColor(&renderTarget);
    renderBuffer.setRenderTargetDepth(nullptr);

    sead::Viewport viewport(renderBuffer);
    viewport.apply(al::GameFrameworkNx::getDrawContext(), renderBuffer);
    renderBuffer.bind(al::GameFrameworkNx::getDrawContext());
    renderBuffer.clear(al::GameFrameworkNx::getDrawContext(), sead::FrameBuffer::cColor,
                       sead::Color4f::cBlack, 1.0f, 0);
}

}  // namespace

namespace al {

/**
 * Gets the render buffer the flare filter is composed to.
 * @return Compose with mask buffer if a camera mask is set, the compose buffer otherwise.
 */
const agl::RenderBuffer* HdrCompose::getRenderBufferFlareFilter() const {
    return getCurrentParam().getCameraMaskTex() > 0 ? &mComposeMaskBuffer : &mComposeBuffer;
}

/**
 * Gets the render buffer the bloom is composed to.
 * @return Compose with mask buffer if the bloom is masked, the compose buffer otherwise.
 */
const agl::RenderBuffer* HdrCompose::getRenderBufferBloom() const {
    if (getCurrentParam().isMaskBloom() && getCurrentParam().getCameraMaskTex() != 0) {
        return &mComposeMaskBuffer;
    }

    return &mComposeBuffer;
}

/**
 * Gets the render buffer the god ray is composed to.
 * @return Compose with mask buffer if the god ray is masked, the compose buffer otherwise.
 */
const agl::RenderBuffer* HdrCompose::getRenderBufferGodRay() const {
    if (getCurrentParam().isMaskGodRay() && getCurrentParam().getCameraMaskTex() != 0) {
        return &mComposeMaskBuffer;
    }

    return &mComposeBuffer;
}

/**
 * Gets the render buffer the light streak is composed to.
 * @return Compose with mask buffer if the light streak is masked, the compose buffer otherwise.
 */
const agl::RenderBuffer* HdrCompose::getRenderBufferLightStreak() const {
    if (getCurrentParam().isMaskLightStreak() && getCurrentParam().getCameraMaskTex() != 0) {
        return &mComposeMaskBuffer;
    }

    return &mComposeBuffer;
}

/**
 * Detaches the compose textures and gives them back to the dynamic texture allocator.
 */
void HdrCompose::releaseComposeBuffer() {
    const agl::TextureData& blackTexture =
        agl::utl::PrimitiveTexture::instance()
            ->getTextureSampler(agl::utl::PrimitiveTexture::cType_Black2D)
            ->getTextureData();
    mComposeSampler.applyTextureData(blackTexture);
    mComposeMaskSampler.applyTextureData(blackTexture);
    mComposeTarget.applyTextureData(blackTexture);
    mComposeMaskTarget.applyTextureData(blackTexture);

    agl::utl::DynamicTextureAllocator* allocator = agl::utl::DynamicTextureAllocator::instance();

    if (mComposeTexture != nullptr) {
        allocator->free(mComposeTexture);
        mComposeTexture = nullptr;
    }

    if (mComposeMaskTexture != nullptr) {
        allocator->free(mComposeMaskTexture);
        mComposeMaskTexture = nullptr;
    }
}

/**
 * Calculates the auto exposure on the GPU.
 */
void HdrCompose::calcGPU() {
    if (getCurrentParam().getExposureType() == 1) {
        mAutoExposure.calcGPU();
    }
}

/**
 * Draws the HDR image composed with the post effects, the camera mask and camera indirect.
 * @param viewIndex View index.
 * @param rBuffer Render buffer to draw to.
 * @param rViewport Viewport to draw with.
 * @param rTexture HDR image.
 * @param pBloom Bloom of the view.
 * @param shaderMode Current shader mode.
 * @return Shader mode after drawing.
 */
agl::ShaderMode HdrCompose::draw(s32 viewIndex, const agl::RenderBuffer& rBuffer,
                                 const sead::Viewport& rViewport,
                                 const agl::TextureData& rTexture, const agl::pfx::Bloom* pBloom,
                                 agl::ShaderMode shaderMode) const {
    const HdrParam& param = getCurrentParam();

    if (param.getExposureType() == 1) {
        sead::GraphicsContext context;
        context.setBlendEnable(false);
        context.setDepthEnable(false, false);
        context.apply(GameFrameworkNx::getDrawContext());
        mAutoExposure.draw(GameFrameworkNx::getAglDrawContext(), viewIndex, rBuffer, &rTexture);
    }

    ViewContext* viewContext = mViewContexts[viewIndex];
    viewContext->mHdrImageSampler.applyTextureData(rTexture);

    sead::GraphicsContext context;
    context.setDepthEnable(false, false);
    context.setBlendEnable(false);
    context.apply(GameFrameworkNx::getDrawContext());

    const char* macroNames[] = {"EFFECT_TYPE",      "COLOR_CORRECTION_TYPE", "EXPOSURE_TYPE",
                                "INDIRECT_TYPE",    "CAMERA_MASK_TYPE",      "CHROMATIC_ABERRATION",
                                "DANGER_INDICATOR"};
    const char* macroValues[] = {"1", "0", "0", "0", "0", "0", "0"};

    if (mIsEnableDangerIndicator) {
        macroValues[6] = "1";
    }

    LightIntensityDirector* lightIntensityDirector = mGraphicsSystemInfo->mLightIntensityDirector;
    ColorCorrectionParamKeeper* colorCorrectionKeeper =
        mGraphicsSystemInfo->mColorCorrectionParamKeeper;
    const agl::pfx::ColorCorrection* colorCorrection =
        colorCorrectionKeeper != nullptr ? colorCorrectionKeeper->getParam() : nullptr;
    bool isColorCorrection = colorCorrection != nullptr && colorCorrection->isEnable() &&
                             colorCorrection->getVariationIndex() != 0;
    bool isValidMask = isValidCameraMask();
    const sead::Color4f& diffuse = param.getCameraMaskDiffuse();
    bool isBlackDiffuse = diffuse.r == 0.0f && diffuse.g == 0.0f && diffuse.b == 0.0f;
    bool isCameraMaskDiffuse = isValidMask && diffuse.a != 0.0f && !isBlackDiffuse;
    bool isCameraIndirect = param.getCameraIndirectTex() != 0;
    bool isCameraIndirect2 = isCameraIndirect && param.getCameraIndirectScale() != 0.0f &&
                             param.getCameraIndirect2Tex() != 0 &&
                             param.getCameraIndirect2Scale() != 0.0f;
    bool isCompose = isAtLeastOneCompose(viewIndex, pBloom);
    bool isComposeMask = isAtLeastOneComposeMask(viewIndex, pBloom);
    bool isCameraMask = isValidMask && isComposeMask;

    if (param.getChromaticAberrationSize() != 0.0f) {
        macroValues[5] = "1";
    }

    if (isCameraMask) {
        viewContext->mCameraMaskSampler.applyTextureData(
            *mCameraMaskTextures[param.getCameraMaskTex() - 1]);
    }

    if (isCameraMaskDiffuse) {
        macroValues[4] = "1";
    }

    if (!mIsDisableCameraIndirect && isCameraIndirect) {
        viewContext->mCameraIndirectSampler.applyTextureData(
            *mCameraIndirectTextures[param.getCameraIndirectTex() - 1]);

        if (isCameraIndirect2) {
            viewContext->mCameraIndirect2Sampler.applyTextureData(
                *mCameraIndirectTextures[param.getCameraIndirect2Tex() - 1]);
        }

        macroValues[3] = isCameraIndirect2 ? "2" : "1";
    }

    if (isCompose && isComposeMask) {
        macroValues[0] = "3";
    } else if (!isCompose && isComposeMask) {
        macroValues[0] = "2";
    } else if (isCompose && !isComposeMask) {
        macroValues[0] = "1";
    } else {
        macroValues[0] = "0";
    }

    if (isColorCorrection) {
        macroValues[1] = "1";
    }

    switch (param.getExposureType()) {
    case 0:
        macroValues[2] = "0";
        break;
    case 1:
        macroValues[2] = "1";
        break;
    }

    const agl::ShaderProgram* program =
        mShaderProgram->searchVariation(7, macroNames, macroValues);
    program->activate(GameFrameworkNx::getAglDrawContext(), true);

    auto hdrImageLocation = searchShaderLocation<agl::SamplerLocation>(*program, "uHdrImage");
    viewContext->mHdrImageSampler.activate(GameFrameworkNx::getAglDrawContext(), hdrImageLocation,
                                           -1, false);

    if (param.getExposureType() == 1) {
        auto exposureLocation =
            searchShaderLocation<agl::SamplerLocation>(*program, "uExposureTexture");
        const agl::pfx::AutoExposure::Context& exposureContext =
            mAutoExposure.getContext(viewIndex);
        exposureContext.mResults[exposureContext.mCurrent].mSampler.activate(
            GameFrameworkNx::getAglDrawContext(), exposureLocation, -1, false);
    }

    UniformBlock* uniformBlock = viewContext->mUniformBlock;
    uniformBlock->setValue(1, lightIntensityDirector->getExposure());

    if (isColorCorrection) {
        viewContext->mUniformBlock->setData(11, &colorCorrection->getMapScaleOffset(), 0, 1);
        auto colorCorrectionLocation =
            searchShaderLocation<agl::SamplerLocation>(*program, "uColorCorrectionTable");
        colorCorrection->getMapSampler().activate(GameFrameworkNx::getAglDrawContext(),
                                                  colorCorrectionLocation, -1, false);
    }

    if (mIsEnableDangerIndicator) {
        viewContext->mUniformBlock->setData(12, &mDangerIndicatorParam, 0, 1);
        viewContext->mUniformBlock->setData(13, &mDangerIndicatorParam2, 0, 1);
        viewContext->mUniformBlock->setValue(14, mDangerIndicatorParam3);
        viewContext->mUniformBlock->setValue(15, mAspectRatio);
    }

    if (isCompose) {
        auto composeLocation = searchShaderLocation<agl::SamplerLocation>(*program, "uCompose");
        mComposeSampler.activate(GameFrameworkNx::getAglDrawContext(), composeLocation, -1,
                                 false);
    }

    if (isComposeMask) {
        auto composeMaskLocation =
            searchShaderLocation<agl::SamplerLocation>(*program, "uComposeWithMask");
        mComposeMaskSampler.activate(GameFrameworkNx::getAglDrawContext(), composeMaskLocation,
                                     -1, false);
    }

    if (isCameraMask) {
        auto cameraMaskLocation =
            searchShaderLocation<agl::SamplerLocation>(*program, "uCameraMask");

        if (cameraMaskLocation.isValid()) {
            viewContext->mCameraMaskSampler.activate(GameFrameworkNx::getAglDrawContext(),
                                                     cameraMaskLocation, -1, false);
        }
    }

    viewContext->mUniformBlock->setValue(2, param.getCameraMaskBase());
    viewContext->mUniformBlock->setValue(3, param.getCameraMaskScale());
    viewContext->mUniformBlock->setData(0, &param.getCameraMaskDiffuse(), 0, 1);

    if (isCameraIndirect) {
        auto cameraIndirectLocation =
            searchShaderLocation<agl::SamplerLocation>(*program, "uCameraIndirect");
        viewContext->mCameraIndirectSampler.activate(GameFrameworkNx::getAglDrawContext(),
                                                     cameraIndirectLocation, -1, false);

        if (isCameraIndirect2) {
            auto cameraIndirect2Location =
                searchShaderLocation<agl::SamplerLocation>(*program, "uCameraIndirect2");
            viewContext->mCameraIndirect2Sampler.activate(GameFrameworkNx::getAglDrawContext(),
                                                          cameraIndirect2Location, -1, false);
        }
    }

    viewContext->mUniformBlock->setValue(4, param.getCameraIndirectScale());
    viewContext->mUniformBlock->setData(7, &mCameraIndirectOffset, 0, 1);
    viewContext->mUniformBlock->setData(9, &param.getCameraIndirectTexScale(), 0, 1);
    viewContext->mUniformBlock->setValue(5, param.getCameraIndirect2Scale());
    viewContext->mUniformBlock->setData(8, &mCameraIndirect2Offset, 0, 1);
    viewContext->mUniformBlock->setData(10, &param.getCameraIndirect2TexScale(), 0, 1);
    viewContext->mUniformBlock->setValue(6, param.getChromaticAberrationSize());

    auto infoLocation =
        searchShaderLocation<agl::UniformBlockLocation>(*program, "HdrComposeInfo");
    viewContext->mUniformBlock->activate(GameFrameworkNx::getAglDrawContext(), infoLocation);
    viewContext->mUniformBlock->flushCurrentBuffer();

    rBuffer.bind(GameFrameworkNx::getDrawContext());
    rViewport.apply(GameFrameworkNx::getDrawContext(), rBuffer);
    drawPostProcessingQuad(GameFrameworkNx::getAglDrawContext());
    rBuffer.getRenderTargetColor()->invalidateGPUCache(GameFrameworkNx::getAglDrawContext());
    return shaderMode;
}

}  // namespace al

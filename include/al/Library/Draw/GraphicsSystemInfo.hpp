#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <container/seadStrTreeMap.h>
#include <container/seadTList.h>
#include <gfx/seadColor.h>
#include <heap/seadDisposer.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <nn/g3d/g3d_ViewVolume.h>
#include <postfx/aglFilterAA.h>
#include <prim/seadEnum.h>
#include <utility/aglParameter.h>
#include <utility/aglParameterIO.h>
#include <utility/aglParameterObj.h>

#include "common/aglShaderEnum.h"
#include "Library/Draw/GraphicsInitArg.hpp"

namespace agl {
class ShaderProgram;
}  // namespace agl

namespace agl::pfx {
class ColorCorrection;
}  // namespace agl::pfx

namespace agl::sdw {
class PrimitiveOcclusion;
class SSAO;
}  // namespace agl::sdw

namespace sead {
class Camera;
class LookAtCamera;
class PerspectiveProjection;
}  // namespace sead

namespace al {
class ApplicationMessageReceiver;
class AtmosScatter;
class AtmosScatterDrawer;
class CubeMapDirector;
class DepthOfFieldDrawer;
class DirectionalLightKeeper;
class EdgeDrawer;
class FlareFilterDirector;
class FogDirector;
class FullScreenQuadModel;
class FullScreenTriangle;
class GBufferArray;
class GodRayDirector;
class GpuMemAllocator;
class GraphicsAreaDirector;
class GraphicsParamFilePath;
class GraphicsParamKeeperImpl;
class GraphicsStressDirector;
class HdrCompose;
class LightIntensityDirector;
class LightStreakDirector;
class LiveActorKit;
class ModelLightDirector;
class NoiseTextureKeeper;
class OccludedEffectDirector;
class PartsGraphics;
class PostProcessingFilter;
class PrePassLightKeeper;
class RadialBlurDirector;
class Resource;
class SceneCameraInfo;
class ShaderCubeMapKeeper;
class ShaderEnvTextureKeeper;
class ShaderHolder;
class ShaderMirrorDirector;
class ShadowDirector;
class SimpleModelEnv;
class SkyboxDirector;
class SSIIKeeper;
class UniformBlock;
class ViewRenderer;

template <typename T>
class GraphicsParamKeeper;

using SSAOParamKeeper = GraphicsParamKeeper<agl::sdw::SSAO>;
using ColorCorrectionParamKeeper = GraphicsParamKeeper<agl::pfx::ColorCorrection>;

SEAD_ENUM(GraphicsAreaTarget, Player, CameraPos, CameraLookAt)

/**
 * Owns every graphics director of a scene (lights, shadows, post effects, atmosphere...) and
 * drives their per-frame update.
 */
class GraphicsSystemInfo : public sead::IDisposer {
public:
    using UniformBlockArray = sead::PtrArray<UniformBlock>;
    using ViewIndexedUboArrayTree = sead::StrTreeMap<128, const UniformBlockArray*>;
    using PartsGraphicsList = sead::TList<PartsGraphics*>;

    GraphicsSystemInfo(const char* pStageName);
    ~GraphicsSystemInfo() override;

    ShaderCubeMapKeeper* getShaderCubeMapKeeper() const;
    const UniformBlockArray* getViewIndexedUboArray(const char* pName) const;
    void setViewIndexedUboArray(const char* pName, const UniformBlockArray* pArray);
    void initAtmosScatter(LiveActorKit* pKit);
    void init(const GraphicsInitArg& rArg, LiveActorKit* pKit);
    void initProjectResource();
    void initStageResource(const Resource* pResource, const char* pStageName, LiveActorKit* pKit,
                           bool isSkipAreaParam, s32 scenarioNo);
    void endInit();
    void setDrawEnv(s32 viewIndex, GBufferArray* pGBufferArray, const sead::Camera* pCamera,
                    const sead::PerspectiveProjection* pProjection);
    void clearGraphicsRequest();
    void cancelLerp();
    void updateGraphics(bool isPaused);
    void preDrawGraphics(const SceneCameraInfo* pCameraInfo);
    void updateViewGpu(s32 viewIndex, const sead::Camera* pCamera,
                       const sead::PerspectiveProjection* pProjection);
    void updateViewVolume(const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);
    bool tryGetAtmosLightDir(sead::Vector3f* pDir) const;
    bool registPartsGraphics(PartsGraphics* pPartsGraphics);
    bool tryDirectionalLightInfo(sead::Vector3f* pDir, const char* pName, f32* pIntensity) const;
    agl::ShaderMode drawFarClearGBuffer(agl::ShaderMode shaderMode) const;
    agl::ShaderMode drawFarClear(agl::ShaderMode shaderMode, bool isGBuffer,
                                 const sead::Color4f& rColor) const;
    void activateDirLitColorTex() const;

    const GraphicsInitArg& getInitArg() const { return mInitArg; }
    CubeMapDirector* getCubeMapDirector() const { return mCubeMapDirector; }
    DirectionalLightKeeper* getDirectionalLightKeeper() const { return mDirectionalLightKeeper; }
    PrePassLightKeeper* getPrePassLightKeeper() const { return mPrePassLightKeeper; }
    SkyboxDirector* getSkyboxDirector() const { return mSkyboxDirector; }
    GraphicsAreaDirector* getGraphicsAreaDirector() const { return mGraphicsAreaDirector; }
    LightIntensityDirector* getLightIntensityDirector() const { return mLightIntensityDirector; }
    UniformBlock* getLightEnvUbo() const { return mLightEnvUbo; }
    ShaderEnvTextureKeeper* getShaderEnvTextureKeeper() const { return mShaderEnvTextureKeeper; }
    ModelLightDirector* getModelLightDirector() const { return mModelLightDirector; }
    ShadowDirector* getShadowDirector() const { return mShadowDirector; }
    GraphicsStressDirector* getGraphicsStressDirector() const { return mGraphicsStressDirector; }
    ShaderMirrorDirector* getShaderMirrorDirector() const { return mShaderMirrorDirector; }
    FogDirector* getFogDirector() const { return mFogDirector; }
    PostProcessingFilter* getPostProcessingFilter() const { return mPostProcessingFilter; }
    const nn::g3d::ViewVolume& getViewVolume() const { return mViewVolume; }
    ViewRenderer* getViewRenderer() const { return mViewRenderer; }
    SimpleModelEnv* getSimpleModelEnv() const { return mSimpleModelEnv; }
    s32 getDrawEnvUpdateCount() const { return mDrawEnvUpdateCount; }
    const sead::Vector3f& getDrawCameraPos() const { return mDrawCameraPos; }
    GBufferArray* getDrawGBufferArray() const { return mDrawGBufferArray; }
    sead::LookAtCamera* getDrawCamera() const { return mDrawCamera; }
    sead::PerspectiveProjection* getDrawProjection() const { return mDrawProjection; }
    s32 getDrawViewIndex() const { return mDrawViewIndex; }
    AtmosScatter* getAtmosScatter() const { return mAtmosScatter; }
    GpuMemAllocator* getGpuMemAllocator() const { return mGpuMemAllocator; }
    ApplicationMessageReceiver* getApplicationMessageReceiver() const {
        return mApplicationMessageReceiver;
    }
    const char* getLodSettingName() const { return mLodSettingName; }
    GraphicsAreaTarget getAreaTarget() const { return GraphicsAreaTarget(mAreaTarget); }
    void setAreaTarget(GraphicsAreaTarget::ValueType target) { mAreaTarget = target; }

    ViewIndexedUboArrayTree mViewIndexedUboArrayTree;
    union {
        GraphicsInitArg mInitArg;
        // Older aliases of mInitArg.mAtmosScatterType and mInitArg._20 used by other units.
        struct {
            s32 _40;
            u8 _44[0x60 - 0x44];
            s32 _60;
        };
    };
    CubeMapDirector* mCubeMapDirector;
    DirectionalLightKeeper* mDirectionalLightKeeper;
    SkyboxDirector* mSkyboxDirector;
    GraphicsAreaDirector* mGraphicsAreaDirector;
    LightIntensityDirector* mLightIntensityDirector;
    RadialBlurDirector* mRadialBlurDirector;
    PrePassLightKeeper* mPrePassLightKeeper;
    ShaderEnvTextureKeeper* mShaderEnvTextureKeeper;
    ModelLightDirector* mModelLightDirector;
    ShadowDirector* mShadowDirector;
    EdgeDrawer* mEdgeDrawer;
    DepthOfFieldDrawer* mDepthOfFieldDrawer;
    GraphicsStressDirector* mGraphicsStressDirector;
    ShaderMirrorDirector* mShaderMirrorDirector;
    SSAOParamKeeper* mSSAOParamKeeper;
    ColorCorrectionParamKeeper* mColorCorrectionParamKeeper;
    FlareFilterDirector* mFlareFilterDirector;
    GodRayDirector* mGodRayDirector;
    FogDirector* mFogDirector;
    OccludedEffectDirector* mOccludedEffectDirector;
    LightStreakDirector* mLightStreakDirector;
    HdrCompose* mHdrCompose;
    SSIIKeeper* mSSIIKeeper;
    agl::sdw::PrimitiveOcclusion* mPrimitiveOcclusion;
    PostProcessingFilter* mPostProcessingFilter;
    NoiseTextureKeeper* mNoiseTextureKeeper;
    ShaderHolder* mShaderHolder;
    s32 mAreaTarget;
    nn::g3d::ViewVolume mViewVolume;
    ViewRenderer* mViewRenderer;
    SimpleModelEnv* mSimpleModelEnv;
    GBufferArray* mDrawGBufferArray;
    sead::LookAtCamera* mDrawCamera;
    sead::PerspectiveProjection* mDrawProjection;
    s32 mDrawViewIndex;
    s32 mDrawEnvUpdateCount;
    sead::Vector3f mDrawCameraPos;
    agl::pfx::FilterAA mFilterAA;
    AtmosScatter* mAtmosScatter;
    void* _d60;
    AtmosScatterDrawer* mAtmosScatterDrawer;
    FullScreenQuadModel* mFullScreenQuadModel;
    const agl::ShaderProgram* mFarClearShader;
    UniformBlock* mFarClearUbo;
    GraphicsParamFilePath* mParamFilePath;
    agl::utl::IParameterIO mParamIO;
    agl::utl::ParameterObj mParamObj;
    agl::utl::Parameter<s32> mAtmosScatterType;
    agl::utl::Parameter<bool> mIsUsingUpdateAtmosCubeMap;
    UniformBlock* mLightEnvUbo;
    UniformBlock* mLightEnvExUbo;
    f32 mLightEnvParams[4];
    ApplicationMessageReceiver* mApplicationMessageReceiver;
    GpuMemAllocator* mGpuMemAllocator;
    FullScreenTriangle* mFullScreenTriangle;
    const char* mLodSettingName;
    bool mIsEnableForceCameraAreaFind;
    PartsGraphicsList mPartsGraphicsList;
};

static_assert(sizeof(GraphicsSystemInfo) == 0x1080);

}  // namespace al

namespace alGfxUtil {
void tryWarningLightPresetSetting(const al::GraphicsSystemInfo* pInfo);
}  // namespace alGfxUtil

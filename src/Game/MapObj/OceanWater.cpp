#include "MapObj/OceanWater.hpp"

#include <cstring>
#include <common/aglDisplayList.h>
#include <common/aglDrawContext.h>
#include <common/aglRenderBuffer.h>
#include <common/aglShaderLocation.h>
#include <common/aglShaderProgram.h>
#include <common/aglTextureSampler.h>
#include <common/aglVertexAttribute.h>
#include <common/aglVertexBuffer.h>
#include <driver/aglGraphicsDriverMgr.h>
#include <gfx/seadCamera.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <nerd/nerdMath.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_Resources.h>
#include <nn/gfx/gfx_Texture.h>
#include <nvn/nvn_FuncPtrInline.h>
#include <prim/seadDelegate.h>
#include <prim/seadSafeString.h>
#include <thread/seadDelegateThread.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Fog/FogDirector.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Sequence/Sequence.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/CubeMapDirector.hpp"
#include "Library/Shader/DeferredRendering/FullScreenQuadModel.hpp"
#include "Library/Shader/DeferredRendering/GBufferArray.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/Shader/ForwardRendering/CubeMapKeeper.hpp"
#include "Library/Shader/ForwardRendering/ShaderEnvTextureKeeper.hpp"
#include "Library/Shader/ForwardRendering/ShaderFresnelTextureKeeper.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Shadow/ShadowDirector.hpp"
#include <prim/seadPtrUtil.h>
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "Project/Model/SimpleModelG3D.hpp"
#include "Project/OceanWave/OceanWaveUserInfo.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Application.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameSystem.hpp"
#include "Util/SceneUtil.hpp"

/**
 * Size of the water UV space, shared with the other water drawers.
 */
sead::Vector4f cWaterUVSize;

namespace {
using TextureViewImpl = nn::gfx::detail::TextureViewImpl<nn::gfx::ApiVariationNvn8>;
using OceanWaterDelegate = sead::Delegate2<OceanWater, sead::Thread*, s64>;

const sead::Color4f cLightColor = {1.0f, 1.0f, 1.0f, 0.0f};
const sead::Vector3f cLightPos = {140.0f, -64.0f, -737.0f};

const al::UniformBlockLayout cUniformLayout[] = {
    {0, agl::UniformBlock::cType_Vec3, 1},   {1, agl::UniformBlock::cType_Vec4, 1},
    {2, agl::UniformBlock::cType_Vec3, 1},   {3, agl::UniformBlock::cType_Vec4, 1},
    {4, agl::UniformBlock::cType_Float, 1},  {5, agl::UniformBlock::cType_Vec3, 1},
    {6, agl::UniformBlock::cType_Float, 1},  {7, agl::UniformBlock::cType_Vec4, 1},
    {8, agl::UniformBlock::cType_Float, 1},  {9, agl::UniformBlock::cType_Float, 1},
    {10, agl::UniformBlock::cType_Vec4, 1},  {11, agl::UniformBlock::cType_Float, 1},
    {12, agl::UniformBlock::cType_Float, 1}, {13, agl::UniformBlock::cType_Float, 1},
    {14, agl::UniformBlock::cType_Float, 1}, {15, agl::UniformBlock::cType_Int, 1},
    {16, agl::UniformBlock::cType_Vec4, 1},  {17, agl::UniformBlock::cType_Float, 1},
    {18, agl::UniformBlock::cType_Vec4, 1},  {19, agl::UniformBlock::cType_Vec3, 1},
    {20, agl::UniformBlock::cType_Float, 1}, {21, agl::UniformBlock::cType_Float, 1},
    {22, agl::UniformBlock::cType_Float, 1}, {23, agl::UniformBlock::cType_Float, 1},
    {24, agl::UniformBlock::cType_Vec3, 1},  {25, agl::UniformBlock::cType_Vec2, 1},
    {26, agl::UniformBlock::cType_Float, 1}, {27, agl::UniformBlock::cType_Float, 1},
    {28, agl::UniformBlock::cType_Float, 1}, {29, agl::UniformBlock::cType_Float, 1},
    {30, agl::UniformBlock::cType_Vec2, 1},  {31, agl::UniformBlock::cType_Vec4, 1},
    {32, agl::UniformBlock::cType_Vec2, 1},  {33, agl::UniformBlock::cType_Float, 1},
    {34, agl::UniformBlock::cType_Vec3, 1},  {35, agl::UniformBlock::cType_Vec2, 1},
    {36, agl::UniformBlock::cType_Vec2, 1},  {37, agl::UniformBlock::cType_Vec2, 1},
    {38, agl::UniformBlock::cType_Vec2, 1},  {39, agl::UniformBlock::cType_Float, 1},
    {40, agl::UniformBlock::cType_Float, 1}, {41, agl::UniformBlock::cType_Vec2, 1},
    {42, agl::UniformBlock::cType_Float, 1}, {43, agl::UniformBlock::cType_Float, 1},
    {44, agl::UniformBlock::cType_Float, 1}, {45, agl::UniformBlock::cType_Vec3, 1},
    {46, agl::UniformBlock::cType_Vec2, 1},  {47, agl::UniformBlock::cType_Float, 1},
    {48, agl::UniformBlock::cType_Vec2, 1},  {49, agl::UniformBlock::cType_Vec2, 1},
    {50, agl::UniformBlock::cType_Float, 1}, {51, agl::UniformBlock::cType_Float, 1},
    {52, agl::UniformBlock::cType_Float, 1}, {53, agl::UniformBlock::cType_Vec4, 4},
    {54, agl::UniformBlock::cType_Float, 1}, {55, agl::UniformBlock::cType_Float, 1},
    {56, agl::UniformBlock::cType_Float, 1}, {57, agl::UniformBlock::cType_Float, 1},
    {58, agl::UniformBlock::cType_Float, 1}, {59, agl::UniformBlock::cType_Float, 1},
};

bool sIsWakeAttached = false;

constexpr agl::VertexStreamFormat cFormatFloat2 = agl::VertexStreamFormat(22);
constexpr agl::VertexStreamFormat cFormatFloat3 = agl::VertexStreamFormat(34);
constexpr agl::VertexStreamFormat cFormatFloat4 = agl::VertexStreamFormat(46);

/**
 * Opens a named debug group on the current command buffer.
 * @param pName name of the group
 */
inline void pushDebugGroup(const char* pName) {
    NVNcommandBuffer* commandBuffer =
        agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext());
    reinterpret_cast<void (*)(NVNcommandBuffer*, const char*)>(
        pfnc_nvnCommandBufferPushDebugGroup)(commandBuffer, pName);
}

/**
 * Closes the debug group opened last on the current command buffer.
 */
inline void popDebugGroup() {
    nvnCommandBufferPopDebugGroup(
        agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext()));
}

/**
 * Checks whether the frame is drawn.
 * @return Whether the frame is drawn.
 */
inline bool isDrawFrame() {
    al::GameFrameworkNx* framework = al::GameFrameworkNx::sInstance;
    return framework->_27c || !framework->_27b;
}

/**
 * Calls the commands recorded in a display list.
 * @param rDisplayList display list to call
 */
inline void callDisplayList(const agl::DisplayList& rDisplayList) {
    NVNcommandBuffer* commandBuffer =
        agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext());
    nvnCommandBufferCallCommands(commandBuffer, 1, rDisplayList.getHandlePtr());
}

/**
 * Advances a UV scroll by one frame.
 * @param pScroll scroll to advance
 * @param rUV UV transform whose Z and W are the scroll speed
 */
inline void scrollUV(sead::Vector2f* pScroll, const sead::Vector4f& rUV) {
    sead::Vector2f speed(rUV.z, rUV.w);
    pScroll->x = speed.x * (1.0f / 60.0f) + pScroll->x;
    pScroll->y = speed.y * (1.0f / 60.0f) + pScroll->y;
}

/**
 * Reads a vector of two components.
 * @param pVec vector to fill
 * @param rIter iterator holding the vector
 * @param pKey key of the vector
 */
inline void tryGetVector2(sead::Vector2f* pVec, const al::ByamlIter& rIter, const char* pKey) {
    al::ByamlIter iter;

    if (rIter.tryGetIterByKey(&iter, pKey)) {
        iter.tryGetFloatByKey(&pVec->x, "X");
        iter.tryGetFloatByKey(&pVec->y, "Y");
    }
}

/**
 * Resets the size of the water UV space.
 */
inline void resetWaterUVSize() {
    cWaterUVSize = {500.0f, 500.0f, 0.001f, 0.001f};
}

/**
 * Reads the name of one sampler of a material.
 * @param pRes material resource
 * @param index sampler index
 * @return The sampler name, or nullptr when the material has no sampler dictionary.
 */
inline const char* getSamplerName(const nn::g3d::ResMaterial* pRes, s32 index) {
    const nn::util::ResDic* dic = pRes->ToData().pSamplerDic.Get();
    return dic != nullptr ? dic->GetKey(index).data() : nullptr;
}

/**
 * Loads a texture of a material into a texture data.
 * @param pTexture texture data to fill
 * @param pMaterial material holding the texture
 * @param index sampler index of the texture
 */
inline void initTextureFromMaterial(agl::TextureData* pTexture,
                                    const nn::g3d::MaterialObj* pMaterial, s32 index) {
    auto* view = static_cast<const TextureViewImpl*>(pMaterial->GetTextureView(index));
    auto* nvnTexture = static_cast<const NVNtexture*>(view->ToData()->pNvnTexture.ptr);
    pTexture->initializeFromNVNtexture(*nvnTexture);
    pTexture->invalidateCPUCache();
}

/**
 * Sets up a sampler for a repeated, filtered water texture.
 * @param pSampler sampler to set up
 */
inline void setupWaterSampler(agl::TextureSampler* pSampler) {
    pSampler->setFilter(1, 1, 2);
    pSampler->setWrap(1, 1, 1);
}

/**
 * Sets up a sampler for a water normal map, limiting its mip levels.
 * @param pSampler sampler to set up
 */
inline void setupNormalSampler(agl::TextureSampler* pSampler) {
    setupWaterSampler(pSampler);
    pSampler->setMinLod(0.0f);
    pSampler->setMaxLod(6.0f);
    pSampler->setLodBias(0.0f);
}

/**
 * Gets the camera the water is seen from.
 * @param pActor actor asking
 * @return The main look at camera.
 */
inline const sead::LookAtCamera* getWaterCamera(const al::LiveActor* pActor) {
    if (pActor->getCameraDirector_RS() != nullptr) {
        return &pActor->getCameraDirector_RS()->getLookAtMain();
    }

    return pActor->getSceneCameraInfo()->mLookAtCamera;
}

/**
 * Gets the camera info of the scene the water is in.
 * @param pActor actor asking
 * @return The scene camera info.
 */
inline const al::SceneCameraInfo* getWaterSceneCameraInfo(const al::LiveActor* pActor) {
    if (pActor->getCameraDirector_RS() != nullptr) {
        return pActor->getCameraDirector_RS()->getSceneCameraInfo();
    }

    return pActor->getSceneCameraInfo();
}

/**
 * Gets the view the water grid is generated for.
 * @param pActor actor asking
 * @return The main camera view.
 */
inline const al::CameraViewInfo* getWaterViewInfo(const al::LiveActor* pActor) {
    return getWaterSceneCameraInfo(pActor)->getViewAt(0);
}

/**
 * Gets the shadow view projection matrix, kept in the depth shadow drawer state.
 * @param pDirector shadow director
 * @return The four rows of the matrix.
 */
inline const sead::Vector4f* getShadowMtx(const al::ShadowDirector* pDirector) {
    return static_cast<const sead::Vector4f*>(
        sead::PtrUtil::addOffset(pDirector->mDepthShadowDrawer, 0xc8));
}
}  // namespace

/**
 * Gets the ocean water scene object.
 * @param pHolder scene object holder user
 * @return The ocean water, or nullptr when the scene has none.
 */
OceanWater* OceanWater::getOceanWater(const al::IUseSceneObjHolder* pHolder) {
    return al::tryGetSceneObj<OceanWater>(pHolder, SceneObjID_OceanWater);
}

/**
 * Constructs the ocean water and initializes the wave simulation.
 * @param pName actor name
 */
OceanWater::OceanWater(const char* pName) : OceanWaveDirector(pName), mGridData() {
    mIsVisible = true;
    WaveGrid::InitRenderData(mGridData.mRenderData);
    mGenerator.init(this);
    mGenerator.initOcean(&mOcean);
    mOcean.init();
    mSeaOffset = mGenerator.getSeaOffset();
    mPolygonModeFront = 2;
    mPolygonModeBack = 2;
    _2892e0 = 0;
    _2892e4 = 1;
    mReflectionType = 2;
    mRenderMode = RenderMode_Forward;
    mReflectionRayMaxStep = 6.0f;
    mReflectionRayStartScale = 600.0f;
    mReflectionRayScaleFactor = 1.08f;
    mIsGaussianBlur = false;
    mWaveFactorDampDistMin = 50000000.0f;
    mWaveFactorDampDistMax = 100000000.0f;
    _23bf08 = false;
    mNormalModeIndex = 0;
    mDisasterModeIndex = 1;
}

/**
 * Stops the water thread and releases the uniform blocks and the wave grid buffers.
 */
OceanWater::~OceanWater() {
    if (mThread != nullptr) {
        kill();

        if (mThread != nullptr) {
            delete mThread;
            mThread = nullptr;
        }
    }

    if (mUniformBlocks[0] != nullptr) {
        delete mUniformBlocks[0];
        mUniformBlocks[0] = nullptr;
    }

    if (mUniformBlocks[1] != nullptr) {
        delete mUniformBlocks[1];
        mUniformBlocks[1] = nullptr;
    }

    WaveGrid::Free(mGridData);
}

/**
 * Initializes the shaders, the textures of the water model and the wave grid thread.
 * @param rInfo actor init info
 */
void OceanWater::init(const al::ActorInitInfo& rInfo) {
    mShader = nullptr;
    mComposeShader = nullptr;
    mNormalsShader = al::ShaderHolder::instance()->getShaderProgram("WaterNormals");
    mReflectionsShader = al::ShaderHolder::instance()->getShaderProgram("WaterSSR");
    mDeferredShader = al::ShaderHolder::instance()->getShaderProgram("WaterDeferredModel");
    mQuadModel = new al::FullScreenQuadModel();
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::invalidateClipping(this);

    if (mActorPoseKeeper == nullptr) {
        al::initActorPoseTQSV(this);
    }

    if (!al::trySyncStageSwitchAppear(this)) {
        al::trySyncStageSwitchKill(this);
    }

    const char* cubeMapName = nullptr;
    al::tryGetStringArg(&cubeMapName, rInfo, "CubeMapUnitName");

    if (cubeMapName != nullptr) {
        mCubeMapIndex = getSceneInfo()
                            ->graphicsSystemInfo->getShaderCubeMapKeeper()
                            ->findCubeMapIndexByName(cubeMapName);
    }

    cubeMapName = nullptr;
    al::tryGetStringArg(&cubeMapName, rInfo, "DisasterCubeMapUnitName");

    if (cubeMapName != nullptr) {
        mDisasterCubeMapIndex = getSceneInfo()
                                    ->graphicsSystemInfo->getShaderCubeMapKeeper()
                                    ->findCubeMapIndexByName(cubeMapName);
    } else {
        mDisasterCubeMapIndex = mCubeMapIndex;
    }

    mDeferredCubeMapIndex = mCubeMapIndex;
    mUniformBlocks[0] = al::createUniformBlock(cUniformLayout, 60, nullptr, 2);
    mUniformBlocks[1] = al::createUniformBlock(cUniformLayout, 60, nullptr, 2);

    const nn::g3d::MaterialObj* material =
        getModelKeeper()->getModelCafe()->getModelG3D()->getMaterialObj(0);

    for (s32 i = 0; i < material->GetResource()->GetSamplerCount(); i++) {
        const nn::g3d::ResMaterial* res = material->GetResource();

        if (strcmp(getSamplerName(res, i), "albedo0") == 0) {
            initTextureFromMaterial(&mTextures[0][0], material, i);
        } else if (strcmp(getSamplerName(res, i), "normal0") == 0) {
            initTextureFromMaterial(&mTextures[0][1], material, i);
        } else if (strcmp(getSamplerName(res, i), "normal1") == 0) {
            initTextureFromMaterial(&mTextures[0][2], material, i);
        } else if (strcmp(getSamplerName(res, i), "albedo0_disaster") == 0) {
            initTextureFromMaterial(&mTextures[1][0], material, i);
        } else if (strcmp(getSamplerName(res, i), "normal0_disaster") == 0) {
            initTextureFromMaterial(&mTextures[1][1], material, i);
        } else if (strcmp(getSamplerName(res, i), "normal1_disaster") == 0) {
            initTextureFromMaterial(&mTextures[1][2], material, i);
        }
    }

    mThread = new sead::DelegateThread(
        "WaterThread", new OceanWaterDelegate(this, &OceanWater::threadFunc_), nullptr,
        sead::Thread::cDefaultPriority, sead::MessageQueue::BlockType::Blocking, 0x7fffffff,
        0x2000, 32);
    mThread->setAffinity(sead::CoreIdMask(sead::CoreId::cSub1));
    mMessageQueue.allocate(1, nullptr);
    const al::CameraViewInfo* viewInfo = getWaterViewInfo(this);
    WaveGrid::InitMeshData(mGridData.mMeshData, *viewInfo, getSceneInfo()->sceneObjHolder);
    s32 phase = SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(this));
    WaveGrid::EnableMultipleTiers(mGridData, phase >= 2 && phase <= 10);
    al::registerExecutorUser(this, rInfo.getExecuteDirector(), "OceanWaterRequest");
}

/**
 * Regenerates the wave grid in the water thread.
 * @param pThread water thread
 * @param message buffer index to fill, plus one
 */
void OceanWater::threadFunc_(sead::Thread* pThread, s64 message) {
    mThreadBufferIndex = message - 1;
    f32 x;
    f32 z;
    f32 dirX;
    f32 dirZ;
    setSeaCenterPosition(0, &x, &z, &dirX, &dirZ);
    const sead::LookAtCamera* camera = getWaterCamera(this);
    nerd::sqrt((camera->getAt() - camera->getPos()).squaredLength());
    const al::CameraViewInfo* viewInfo = getWaterViewInfo(this);
    WaveGrid::Generate(mGridData, *viewInfo, mThreadBufferIndex);
    WaveGrid::FlushCPU(mGridData, mThreadBufferIndex);
    mMessageQueue.push(mThreadBufferIndex, sead::MessageQueue::BlockType::Blocking);
}

/**
 * Gets the center of the sea, which follows the player.
 * @param unused unused
 * @param pX output X position
 * @param pZ output Z position
 * @param pDirX output X of the horizontal front direction
 * @param pDirZ output Z of the horizontal front direction
 */
void OceanWater::setSeaCenterPosition(long unused, f32* pX, f32* pZ, f32* pDirX, f32* pDirZ) {
    al::LiveActor* player = mPlayer;
    const sead::Vector3f& trans = al::getTrans(player);
    sead::Vector3f front;
    al::calcFrontDir(&front, player);
    *pX = trans.x;
    *pZ = trans.z;
    *pDirX = front.x;
    *pDirZ = front.z;
    f32 length = sqrtf(*pDirX * *pDirX + *pDirZ * *pDirZ);

    if (length > 0.01f) {
        *pDirX /= length;
        *pDirZ /= length;
    }
}

/**
 * Starts the water thread once every actor is placed.
 */
void OceanWater::initAfterPlacement() {
    mSeaY = al::getTrans(this).y;
    mFrame = 0;
    mPlayer = al::getPlayerActor(this, 0);
    mSeaOffset = mSeaY;
    al::tryFindAreaObj(this, "OceanWaveGenerator");
    mThread->start();
    mCurrentVertexAttributes = &mGridData.mRenderData.mVertexAttributes[0][mBufferIndex];
}

/**
 * Reads the water parameters of the stage.
 * @param rIter water parameter file
 * @param pName unused
 */
void OceanWater::initFromYaml(const al::ByamlIter& rIter, const char* pName) {
    rIter.tryGetIntByKey(&mReflectionType, "ReflectionType");
    rIter.tryGetFloatByKey(&mReflectionRayMaxStep, "ReflectionRayMaxStep");
    rIter.tryGetFloatByKey(&mReflectionRayStartScale, "ReflectionRayStartScale");
    rIter.tryGetFloatByKey(&mReflectionRayScaleFactor, "ReflectionRayScaleFactor");
    rIter.tryGetFloatByKey(&mWaveFactorDampDistMin, "WaveFactorDampDistMin");
    rIter.tryGetFloatByKey(&mWaveFactorDampDistMax, "WaveFactorDampDistMax");
    al::ByamlIter paramsIter;

    if (!rIter.tryGetIterByKey(&paramsIter, "Params")) {
        return;
    }

    mParamsNum = paramsIter.getSize();

    if (mParamsNum < 1) {
        return;
    }

    mParams = new Params[mParamsNum];

    for (s32 i = 0; i < mParamsNum; i++) {
        al::ByamlIter iter;

        if (paramsIter.tryGetIterByIndex(&iter, i)) {
            iter.tryGetStringByKey(&mParams[i].mName, "Name");
            initModeFromYaml("NormalMode", mParams[i].mModes[0], iter);
            initModeFromYaml("DisasterMode", mParams[i].mModes[1], iter);
        }
    }

    mCurrentParams = mParams;
    mCurrentParams = findParamsByName("Default");

    if (mCurrentParams != nullptr) {
        mOceanData = mCurrentParams->mModes[0];
    }
}

/**
 * Reads the look of one mode of a parameter set.
 * @param pName mode name
 * @param rData look to fill
 * @param rIter parameter set
 */
void OceanWater::initModeFromYaml(const char* pName, OceanData& rData,
                                  const al::ByamlIter& rIter) {
    al::ByamlIter iter;

    if (!rIter.tryGetIterByKey(&iter, pName)) {
        iter = rIter;
    }

    iter.tryGetIntByKey(&rData.mFresnelType, "FresnelType");
    initColorFromYaml("Ambient", rData.mAmbient, iter);
    initMaterialFromYaml("Albedo", rData.mAlbedoUV, rData.mAlbedoUVRot, rData.mAlbedoIntensity,
                         iter);
    initMaterialFromYaml("NormalMap1", rData.mNormalMap1UV, rData.mNormalMap1UVRot,
                         rData.mNormalMap1Intensity, iter);
    initMaterialFromYaml("NormalMap2", rData.mNormalMap2UV, rData.mNormalMap2UVRot,
                         rData.mNormalMap2Intensity, iter);
    initRefractionFromYaml("Refraction", rData.mRefractionColor, rData.mRefractionFactor,
                           rData.mRefractionFadeHeight, iter);
    initReflectionFromYaml("Reflection", rData, iter);
    iter.tryGetFloatByKey(&rData.mAmbientWaveAmplitude, "AmbientWaveAmplitude");
    iter.tryGetFloatByKey(&rData.mAmbientWave1.x, "AmbientWave1Speed");
    iter.tryGetFloatByKey(&rData.mAmbientWave1.y, "AmbientWave1Wavelength");
    iter.tryGetFloatByKey(&rData.mAmbientWave2.x, "AmbientWave2Speed");
    iter.tryGetFloatByKey(&rData.mAmbientWave2.y, "AmbientWave2Wavelength");
    initColorFromYaml("FoamColor", rData.mFoamColor, iter);
    tryGetVector2(&rData.mFoamTileSize, iter, "FoamTileSize");
    tryGetVector2(&rData.mFoamAnimSpeed, iter, "FoamAnimSpeed");
    tryGetVector2(&rData.mFoamAnimWaveLength, iter, "FoamAnimWaveLength");
    tryGetVector2(&rData.mFoamAnimWaveAmplitude, iter, "FoamAnimWaveAmplitude");
    iter.tryGetFloatByKey(&rData.mSurfAnimSpeed, "SurfAnimSpeed");
    iter.tryGetFloatByKey(&rData.mSurfNormalMapScale, "SurfNormalMapScale");
    tryGetVector2(&rData.mSurfAnimWavelength, iter, "SurfAnimWavelength");
    iter.tryGetFloatByKey(&rData.mSurfAnimAmplitude, "SurfAnimAmplitude");
    iter.tryGetFloatByKey(&rData.mSurfInOutAnimDistance, "SurfInOutAnimDistance");
    iter.tryGetFloatByKey(&rData.mSurfWidth, "SurfWidth");
    iter.tryGetFloatByKey(&rData.mCubeMapIncidenceAngleMin, "CubeMapIncidenceAngleMin");
    iter.tryGetFloatByKey(&rData.mCubeMapIncidenceAngleMax, "CubeMapIncidenceAngleMax");
}

/**
 * Finds a parameter set by name.
 * @param pName name of the parameter set
 * @return The parameter set, or the current one when there is none with that name.
 */
OceanWater::Params* OceanWater::findParamsByName(const char* pName) const {
    if (pName != nullptr) {
        for (s32 i = 0; i < mParamsNum; i++) {
            if (al::isEqualString(mParams[i].mName, pName)) {
                return &mParams[i];
            }
        }
    }

    return mCurrentParams;
}

/**
 * Reads a color.
 * @param pName key of the color
 * @param rColor color to fill
 * @param rIter iterator holding the color
 */
void OceanWater::initColorFromYaml(const char* pName, sead::Color4f& rColor,
                                   const al::ByamlIter& rIter) {
    al::ByamlIter iter;

    if (rIter.tryGetIterByKey(&iter, pName)) {
        iter.tryGetFloatByKey(&rColor.r, "R");
        iter.tryGetFloatByKey(&rColor.g, "G");
        iter.tryGetFloatByKey(&rColor.b, "B");
        iter.tryGetFloatByKey(&rColor.a, "A");
    }
}

/**
 * Reads the UV scale and the UV scroll of a material.
 * @param pName key of the material
 * @param rUV UV scale to fill
 * @param rUVScroll UV scroll to fill
 * @param rIter iterator holding the material
 */
void OceanWater::initMaterialFromYaml(const char* pName, sead::Vector2f& rUV,
                                      sead::Vector2f& rUVScroll, const al::ByamlIter& rIter) {
    al::ByamlIter iter;
    rIter.tryGetIterByKey(&iter, pName);
    al::ByamlIter vecIter;
    iter.tryGetIterByKey(&vecIter, "UV");
    vecIter.tryGetFloatByKey(&rUV.x, "X");
    vecIter.tryGetFloatByKey(&rUV.y, "Y");
    iter.tryGetIterByKey(&vecIter, "UVScroll");
    vecIter.tryGetFloatByKey(&rUVScroll.x, "X");
    vecIter.tryGetFloatByKey(&rUVScroll.y, "Y");
}

/**
 * Reads the UV transform and the intensity of a material.
 * @param pName key of the material
 * @param rUV UV scale and scroll to fill
 * @param rUVRot UV rotation to fill
 * @param rIntensity intensity to fill
 * @param rIter iterator holding the material
 */
void OceanWater::initMaterialFromYaml(const char* pName, sead::Vector4f& rUV, f32& rUVRot,
                                      f32& rIntensity, const al::ByamlIter& rIter) {
    al::ByamlIter iter;
    rIter.tryGetIterByKey(&iter, pName);
    al::ByamlIter uvIter;
    iter.tryGetIterByKey(&uvIter, "UV");
    uvIter.tryGetFloatByKey(&rUV.x, "X");
    uvIter.tryGetFloatByKey(&rUV.y, "Y");
    uvIter.tryGetFloatByKey(&rUV.z, "Z");
    uvIter.tryGetFloatByKey(&rUV.w, "W");
    iter.tryGetFloatByKey(&rUVRot, "UVRot");
    iter.tryGetFloatByKey(&rIntensity, "Intensity");
}

/**
 * Reads the refraction parameters.
 * @param pName key of the refraction
 * @param rColor refraction color to fill
 * @param rFactor refraction factor to fill
 * @param rFadeHeight fade height to fill
 * @param rIter iterator holding the refraction
 */
void OceanWater::initRefractionFromYaml(const char* pName, sead::Color4f& rColor, f32& rFactor,
                                        f32& rFadeHeight, const al::ByamlIter& rIter) {
    al::ByamlIter iter;
    rIter.tryGetIterByKey(&iter, pName);
    al::ByamlIter colorIter;
    iter.tryGetFloatByKey(&rFactor, "RefractionFactor");
    iter.tryGetIterByKey(&colorIter, "Color");
    colorIter.tryGetFloatByKey(&rColor.r, "R");
    colorIter.tryGetFloatByKey(&rColor.g, "G");
    colorIter.tryGetFloatByKey(&rColor.b, "B");
    colorIter.tryGetFloatByKey(&rColor.a, "A");
    iter.tryGetFloatByKey(&rFadeHeight, "FadeHeight");
}

/**
 * Reads the reflection parameters.
 * @param pName key of the reflection
 * @param rData look to fill
 * @param rIter iterator holding the reflection
 */
void OceanWater::initReflectionFromYaml(const char* pName, OceanData& rData,
                                        const al::ByamlIter& rIter) {
    al::ByamlIter iter;

    if (!rIter.tryGetIterByKey(&iter, pName)) {
        return;
    }

    iter.tryGetFloatByKey(&rData.mReflectionDistance, "Distance");
    iter.tryGetFloatByKey(&rData.mReflectionBias, "Bias");
    iter.tryGetFloatByKey(&rData.mReflectionBlend, "Blend");
    iter.tryGetFloatByKey(&rData.mReflectionFactor, "Factor");
    initColorFromYaml("TintColor", rData.mReflectionTintColor, iter);
    iter.tryGetFloatByKey(&rData.mReflectionDepthCutoff, "DepthCutoff");
    iter.tryGetFloatByKey(&rData.mCubeMapReflectionFactor, "CubeMapReflectionFactor");
    iter.tryGetFloatByKey(&rData.mCubeMapReflectionBlend, "CubeMapReflectionBlend");
}

/**
 * Creates a ripple at an actor.
 * @param pActor actor making the ripple
 * @param pInfo ripple parameters
 * @return Always false.
 */
bool OceanWater::createWave(const al::LiveActor* pActor, const al::OceanWaveInfo* pInfo) {
    sead::Vector3f pos = al::getTrans(pActor);

    if (pInfo->mJointName != nullptr) {
        const sead::Matrix34f* jointMtx = al::getJointMtxPtr(pActor, pInfo->mJointName);

        if (jointMtx != nullptr) {
            pos = jointMtx->getTranslation();
        }
    }

    sead::Vector3f center = pos + pInfo->mPosOffset;
    mOcean.createRipple(center.x, center.z, pInfo->mSize, pInfo->mSpeed, pInfo->mTime, pInfo->mAmp,
                        pInfo->mLen);
    return false;
}

/**
 * Gets the height of the waves at a position.
 * @param rPos position
 * @return The wave height.
 */
f32 OceanWater::getY(const sead::Vector3f& rPos) {
    return mOcean.getWavePoint(rPos.x, rPos.z)->mHeight;
}

/**
 * Stops the water thread.
 */
void OceanWater::kill() {
    if (mThread != nullptr) {
        mThread->quit(false);
        mThread->waitDone();
    }
}

/**
 * Appears and starts the appear action.
 */
void OceanWater::appear() {
    al::LiveActor::appear();
    al::tryStartAction(this, "Appear");
}

/**
 * Receives a message.
 * @param pMsg message
 * @param pOther sender sensor
 * @param pSelf receiver sensor
 * @return Whether the message was handled.
 */
bool OceanWater::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (al::isMsgAskSafetyPoint(pMsg)) {
        return true;
    }

    if (al::isMsgShowModel(pMsg)) {
        al::showModelIfHide(this);
        return true;
    }

    if (al::isMsgHideModel(pMsg)) {
        al::hideModelIfShow(this);
        return true;
    }

    return false;
}

/**
 * Draws the water surface with the current render mode.
 */
void OceanWater::draw() const {
    al::LiveActor::draw();

    if (!mIsVisible || al::isDead(this)) {
        return;
    }

    pushDebugGroup("WATER");
    sead::GraphicsContext context;
    context.setDepthEnable(true, true);
    context.setDepthFunc(2);
    context.setCullingMode(0);
    context.setPolygonMode(mPolygonModeFront, mPolygonModeBack);
    context.setBlendEnable(false);
    context.setBlendEquationA(0, 1);
    context.apply(al::GameFrameworkNx::getDrawContext());

    if (mRenderMode == RenderMode_Indirect) {
        sead::GraphicsContext indirectContext;
        indirectContext.setDepthEnable(true, false);
        indirectContext.setDepthFunc(2);
        indirectContext.setCullingMode(0);
        indirectContext.setPolygonMode(mPolygonModeFront, mPolygonModeBack);
        indirectContext.setBlendEnable(0, false);
        indirectContext.setBlendEnable(1, false);
        indirectContext.setBlendEquationA(0, 1);
        indirectContext.apply(al::GameFrameworkNx::getDrawContext());
        const agl::RenderBuffer* renderBuffer =
            al::GameFrameworkNx::getAglDrawContext()->getBoundRenderBuffer();
        drawNormals();
        drawReflections();
        indirectContext.setDepthEnable(true, true);
        indirectContext.apply(al::GameFrameworkNx::getDrawContext());
        drawCompose(renderBuffer);
        return;
    }

    if (mRenderMode == RenderMode_Deferred) {
        drawDeffered();
        return;
    }

    const char* macros[2] = {"REFLECTION", "FOG_TYPE"};
    const char* values[2] = {"0", "0"};

    if (mReflectionType == 2) {
        values[0] = "2";
    } else if (mReflectionType == 1) {
        values[0] = "1";
    }

    al::FogDirector* fogDirector = getSceneInfo()->graphicsSystemInfo->getFogDirector();

    if (fogDirector != nullptr && *fogDirector->getFogParam().mIntensityMax > 0.0f) {
        values[1] = "1";
    }

    const agl::ShaderProgram* program = mShader->searchVariation(2, macros, values);

    if (program == nullptr) {
        return;
    }

    bindUniformBlock(0);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    bindTextures(program);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    popDebugGroup();
}

/**
 * Draws the normals of the water surface.
 */
void OceanWater::drawNormals() const {
    pushDebugGroup("Water Normals");
    getSceneInfo();
    resetWaterUVSize();
    const agl::ShaderProgram* program = mNormalsShader->getVariation(0);
    bindUniformBlock(0);
    agl::SamplerLocation normalLocation("cNormal");
    normalLocation.search(*program);
    agl::SamplerLocation normal2Location("cNormal2");
    normal2Location.search(*program);
    agl::TextureSampler normalSampler;
    agl::TextureSampler normal2Sampler;
    normalSampler.applyTextureData(mTextures[mTextureSetIndex][1]);
    setupNormalSampler(&normalSampler);
    normalSampler.activate(al::GameFrameworkNx::getAglDrawContext(), normalLocation, -1, false);
    normal2Sampler.applyTextureData(mTextures[mTextureSetIndex][2]);
    setupWaterSampler(&normal2Sampler);
    normal2Sampler.setLodBias(0.0f);
    normal2Sampler.setMinLod(0.0f);
    normal2Sampler.setMaxLod(6.0f);
    normal2Sampler.activate(al::GameFrameworkNx::getAglDrawContext(), normal2Location, -1, false);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mGridData.mRenderData.mDisplayLists[0][mBufferIndex]);
    popDebugGroup();
}

/**
 * Draws the screen space reflections of the water surface.
 */
void OceanWater::drawReflections() const {
    pushDebugGroup("Water Reflections");
    getSceneInfo();
    resetWaterUVSize();
    const agl::ShaderProgram* program = mReflectionsShader->getVariation(0);
    bindUniformBlock(0);
    bindTextures(program);
    agl::SamplerLocation normalLocation("cNormal");
    normalLocation.search(*program);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mGridData.mRenderData.mDisplayLists[0][mBufferIndex]);
    popDebugGroup();
}

/**
 * Composes the water surface into a render buffer.
 * @param pRenderBuffer render buffer to draw into
 */
void OceanWater::drawCompose(const agl::RenderBuffer* pRenderBuffer) const {
    pushDebugGroup("Water Compose");
    sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    al::GraphicsStressDirector* stressDirector =
        getSceneInfo()->graphicsSystemInfo->getGraphicsStressDirector();
    al::getDisplayWidth();
    stressDirector->getBufferSizeX();
    al::getDisplayHeight();
    stressDirector->getBufferSizeY();
    getSceneInfo();
    sead::Viewport viewport(*pRenderBuffer);
    pRenderBuffer->bind(al::GameFrameworkNx::getDrawContext());
    viewport.apply(al::GameFrameworkNx::getDrawContext(), *pRenderBuffer);
    resetWaterUVSize();
    const char* macros[1] = {"GAUSIAN_BLUR"};
    const char* values[1] = {"0"};

    if (mIsGaussianBlur) {
        values[0] = "1";
    }

    const agl::ShaderProgram* program = mComposeShader->searchVariation(1, macros, values);
    bindUniformBlock(0);
    bindTextures(program);
    agl::SamplerLocation sceneLocation("cScene");
    sceneLocation.search(*program);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mGridData.mRenderData.mDisplayLists[0][mBufferIndex]);
    popDebugGroup();
}

/**
 * Draws the water surface into the G-buffer.
 */
void OceanWater::drawDeffered() const {
    pushDebugGroup("Water Deferred");
    getSceneInfo()->graphicsSystemInfo->getDrawGBufferArray()->bindRenderBuffer(4);
    sead::GraphicsContext context;
    context.setDepthEnable(true, true);
    context.setDepthFunc(2);
    context.setCullingMode(0);
    context.setPolygonMode(mPolygonModeFront, mPolygonModeBack);
    context.setBlendEnable(0, false);
    context.setBlendEnable(1, false);
    context.setBlendEnable(2, false);
    context.setBlendEnable(3, false);
    context.setBlendEquationA(0, 1);
    context.apply(al::GameFrameworkNx::getDrawContext());
    resetWaterUVSize();
    const agl::ShaderProgram* program = mDeferredShader->getVariation(0);
    bindTexturesDeferred(program);
    bindUniformBlock(0);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mGridData.mRenderData.mDisplayLists[0][mBufferIndex]);
    popDebugGroup();
}

/**
 * Fills and binds one of the uniform blocks of the water shaders.
 * @param index uniform block to use, 1 for the second tier
 */
void OceanWater::bindUniformBlock(u32 index) const {
    DisasterModeController::tryGetController(this);
    resetWaterUVSize();
    al::UniformBlock* const& block = mUniformBlocks[index];
    al::UniformBlockSetter setter(block, 0);
    block->setValueRef(0, cLightPos);
    block->setValueRef(1, cLightColor);
    block->setValue(6, mFrame * (1.0f / 60.0f));
    block->setValueRef(3, sead::Vector4f(mOceanData.mAlbedoUV.x, mOceanData.mAlbedoUV.y,
                                      mAlbedoScroll.x, mAlbedoScroll.y));
    block->setValue(4, mOceanData.mAlbedoUVRot * sead::Mathf::deg2rad(1.0f));
    block->setValueRef(7, sead::Vector4f(mOceanData.mNormalMap1UV.x, mOceanData.mNormalMap1UV.y,
                                      mNormalMap1Scroll.x, mNormalMap1Scroll.y));
    block->setValue(8, mOceanData.mNormalMap1UVRot * sead::Mathf::deg2rad(1.0f));
    block->setValue(9, mOceanData.mNormalMap1Intensity);
    block->setValueRef(10, sead::Vector4f(mOceanData.mNormalMap2UV.x, mOceanData.mNormalMap2UV.y,
                                       mNormalMap2Scroll.x, mNormalMap2Scroll.y));
    block->setValue(11, mOceanData.mNormalMap2UVRot * sead::Mathf::deg2rad(1.0f));
    block->setValue(12, mOceanData.mNormalMap2Intensity);
    block->setValue(12, mOceanData.mRefractionFactor);
    if (index != 0) {
        block->setValue(14, mSeaY + 2500.0f);
    } else {
        block->setValue(14, mSeaY);
    }

    block->setValue(13, mOceanData.mRefractionFactor);
    block->setValueRef(16, mOceanData.mRefractionColor);
    block->setValue(17, mOceanData.mRefractionFadeHeight);
    block->setValueRef(18, cWaterUVSize);
    block->setValueRef(19, getWaterSceneCameraInfo(this)->mLookAtCamera->getPos());
    block->setValue(20, mOceanData.mReflectionDistance);
    block->setValue(21, mOceanData.mReflectionBias);
    block->setValueRef(2, mOceanData.mAmbient);
    block->setValueRef(5, mOceanData.mAlbedoColor);
    block->setValue(22, mOceanData.mReflectionFactor);
    block->setValue(23, mOceanData.mReflectionBlend);
    block->setValueRef(24, mOceanData.mReflectionTintColor);
    block->setValueRef(25, sead::Vector2f(0.5f, 0.75f));
    block->setValue(26, mOceanData.mReflectionDepthCutoff);
    block->setValue(27, mOceanData.mCubeMapReflectionFactor);
    block->setValue(28, mOceanData.mCubeMapReflectionBlend);
    block->setValue(29, mReflectionRayMaxStep);
    block->setValueRef(30, mReflectionRayStartScale);
    block->setValueRef(31, sead::Vector4f(mFlowDir.x, mFlowDir.z, 10.0f, 0.0f));
    block->setValueRef(32, _164);
    block->setValue(33, 10.0f);
    block->setValueRef(34, sead::Vector3f(mOceanData.mFoamColor.r, mOceanData.mFoamColor.g,
                                       mOceanData.mFoamColor.b));
    block->setValueRef(35, mOceanData.mFoamTileSize);
    block->setValueRef(36, mOceanData.mFoamAnimSpeed);
    block->setValueRef(37, mOceanData.mFoamAnimWaveLength);
    block->setValueRef(38, mOceanData.mFoamAnimWaveAmplitude);
    block->setValue(59, mHeightMapScale);
    block->setValue(39, mOceanData.mSurfAnimSpeed);
    block->setValue(40, mOceanData.mSurfNormalMapScale);
    block->setValueRef(41, mOceanData.mSurfAnimWavelength);
    block->setValue(42, mOceanData.mSurfAnimAmplitude);
    block->setValue(43, mOceanData.mSurfInOutAnimDistance);
    block->setValue(44, mOceanData.mSurfWidth);
    block->setValueRef(45, sead::Vector3f(0.09f, 0.65f, 1.0f));
    block->setValueRef(46, sead::Vector2f(mOceanData.mCubeMapIncidenceAngleMin,
                                       mOceanData.mCubeMapIncidenceAngleMax));
    block->setValue(47, mOceanData.mAmbientWaveAmplitude);
    block->setValueRef(48, mOceanData.mAmbientWave1);
    block->setValueRef(49, mOceanData.mAmbientWave2);
    block->setValue(50, 0.7f);
    block->setValue(51, 1.0f);
    block->setValue(52, 223200.0f);
    al::ShadowDirector* shadowDirector = getSceneInfo()->graphicsSystemInfo->getShadowDirector();
    block->setData(53, getShadowMtx(shadowDirector), 0, 4);
    block->setValue(54, 0.0f);
    f32 dampStart = shadowDirector->mDistDampStart;
    f32 dampEnd = shadowDirector->mDistDampEnd;
    f32 dampRate = 1.0f / (dampStart + ((dampEnd < dampStart ? -0.0f : 1.0f) - dampEnd));
    block->setValue(55, -dampStart);
    block->setValue(56, dampRate);
    block->setValue(57, mWaveFactorDampDistMin);
    block->setValue(58, mWaveFactorDampDistMax);
    agl::ShaderLocation location;
    location.setLocation(agl::cShaderType_Vertex, 2);
    location.setLocation(agl::cShaderType_Fragment, 2);
    location.setLocation(agl::cShaderType_Geometry, 2);
    block->activate(al::GameFrameworkNx::getAglDrawContext(), location);
}

/**
 * Binds the textures of the forward water shaders.
 * @param pProgram shader program to bind to
 */
void OceanWater::bindTextures(const agl::ShaderProgram* pProgram) const {
    agl::SamplerLocation baseLocation("cBase");
    baseLocation.search(*pProgram);
    agl::SamplerLocation normalLocation("cNormal");
    normalLocation.search(*pProgram);
    agl::SamplerLocation normal2Location("cNormal2");
    normal2Location.search(*pProgram);
    agl::SamplerLocation sceneLocation("cScene");
    sceneLocation.search(*pProgram);
    agl::SamplerLocation viewDepthLocation("cViewDepth");
    viewDepthLocation.search(*pProgram);
    const agl::TextureData* indirectTexture =
        getSceneInfo()->graphicsSystemInfo->getShaderEnvTextureKeeper()->getIndirectTexture();
    getSceneInfo()->graphicsSystemInfo->getDrawGBufferArray()->activateSamplerNearestAlbedo(
        sceneLocation);
    getSceneInfo()->graphicsSystemInfo->getDrawGBufferArray()->activateSamplerNearestDepthView(
        viewDepthLocation);
    agl::TextureSampler baseSampler;
    agl::TextureSampler normalSampler;
    agl::TextureSampler normal2Sampler;
    agl::TextureSampler indirectSampler;

    if (indirectTexture != nullptr) {
        indirectSampler.applyTextureData(*indirectTexture);
        setupWaterSampler(&baseSampler);
        indirectSampler.activate(al::GameFrameworkNx::getAglDrawContext(),
                                 al::getSamplerLocationIndirect(), -1, false);
    }

    baseSampler.applyTextureData(mTextures[mTextureSetIndex][0]);
    setupWaterSampler(&baseSampler);
    baseSampler.activate(al::GameFrameworkNx::getAglDrawContext(), baseLocation, -1, false);
    normalSampler.applyTextureData(mTextures[mTextureSetIndex][1]);
    setupNormalSampler(&normalSampler);
    normalSampler.activate(al::GameFrameworkNx::getAglDrawContext(), normalLocation, -1, false);
    normal2Sampler.applyTextureData(mTextures[mTextureSetIndex][2]);
    setupNormalSampler(&normal2Sampler);
    normal2Sampler.activate(al::GameFrameworkNx::getAglDrawContext(), normal2Location, -1, false);
    al::CubeMapDirector* cubeMapDirector = getSceneInfo()->graphicsSystemInfo->getCubeMapDirector();
    al::ShaderFresnelTextureKeeper* fresnelKeeper =
        getSceneInfo()->graphicsSystemInfo->getShaderEnvTextureKeeper()->getFresnelTextureKeeper();
    s32 cubeMapIndex = mCubeMapIndex;
    al::GraphicsAreaDirector* areaDirector =
        getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector();

    if (areaDirector != nullptr) {
        al::CurrentGraphicsAreaParam param;
        areaDirector->getCurrentGraphicsAreaParam(&param,
                                                  al::GraphicsAreaParamType::CubeMapCapturePoint);

        if (param.mPriority < 0 || param.mPriority > 1000) {
            cubeMapIndex = -1;
        }
    }

    cubeMapDirector->activateCubeMapTexture(cubeMapIndex, 0, 0, false);
    fresnelKeeper->activateFresnelTexture(mOceanData.mFresnelType, false);
}

/**
 * Sends the next wave grid request to the water thread.
 */
void OceanWater::execute() {
    if (isDrawFrame()) {
        mThread->sendMessage(mThreadBufferIndex + 1, sead::MessageQueue::BlockType::Blocking);
    }
}

/**
 * Receives the buffer the water thread finished.
 */
void OceanWater::receiveThreadMessage() {
    mBufferIndex = mMessageQueue.pop(sead::MessageQueue::BlockType::Blocking);
    mThreadBufferIndex = (mBufferIndex & 1) ^ 1;
}

/**
 * Updates the ocean look, the wave grid buffers and the player wake.
 * @param isMovement whether the actor moves this frame
 */
void OceanWater::update(bool isMovement) {
    if (isMovement) {
        if (mIsFirstUpdate) {
            mIsFirstUpdate = false;
            GameSystem* gameSystem = GameSystemFunction::getGameSystem();

            if (gameSystem != nullptr && gameSystem->getSequence() != nullptr &&
                gameSystem->getSequence()->getScene() != nullptr &&
                rc::isSingleModeBossScene(gameSystem->getSequence()->getScene())) {
                mDisasterRate = 1.0f;
                mTextureSetIndex = 1;
            }
        }

        DisasterModeController* controller = DisasterModeController::tryGetController(this);

        if (controller != nullptr) {
            s32 state = static_cast<s32>(controller->getState());

            if (state == 5 || state == 6) {
                mDisasterRate += 1.0f / 120.0f;
                mDisasterRate = sead::Mathf::clamp(mDisasterRate, 0.0f, 1.0f);
                mTextureSetIndex = 0;
            } else if (state >= 7 && state <= 9) {
                mDisasterRate = 1.0f;
                mTextureSetIndex = 1;
            } else if (state >= 1 && state <= 3) {
                mDisasterRate = 0.0f;
                mTextureSetIndex = 0;
            }
        }

        Params* toParams = mCurrentParams;
        Params* fromParams = mCurrentParams;
        f32 areaRate = 1.0f;
        al::GraphicsAreaDirector* areaDirector =
            getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector();

        if (areaDirector != nullptr) {
            al::CurrentGraphicsAreaParam param;
            areaDirector->getCurrentGraphicsAreaParam(&param, al::GraphicsAreaParamType::Water);
            toParams = findParamsByName(param.mParamName);
            fromParams = findParamsByName(param.mPrevParamName);
            areaRate = param.mRate;
        }

        OceanData modeData[2];

        if (fromParams != nullptr && toParams != nullptr) {
            modeData[mNormalModeIndex] = OceanData::lerp(fromParams->mModes[mNormalModeIndex],
                                                         toParams->mModes[mNormalModeIndex],
                                                         areaRate);
            modeData[mDisasterModeIndex] =
                OceanData::lerp(fromParams->mModes[mDisasterModeIndex],
                                toParams->mModes[mDisasterModeIndex], areaRate);
            mOceanData = OceanData::lerp(modeData[mNormalModeIndex],
                                         modeData[mDisasterModeIndex], mDisasterRate);
        }

        scrollUV(&mAlbedoScroll, mOceanData.mAlbedoUV);
        scrollUV(&mNormalMap1Scroll, mOceanData.mNormalMap1UV);
        scrollUV(&mNormalMap2Scroll, mOceanData.mNormalMap2UV);

        if (mTargetHeightMapScale > mHeightMapScale) {
            f32 scale = mHeightMapScale + 0.01f;
            mHeightMapScale = scale < mTargetHeightMapScale ? scale : mTargetHeightMapScale;
        } else if (mTargetHeightMapScale < mHeightMapScale) {
            f32 scale = mHeightMapScale + -0.01f;
            mHeightMapScale = scale > mTargetHeightMapScale ? scale : mTargetHeightMapScale;
        }
    }

    if (!mThread->isDone() && isDrawFrame()) {
        receiveThreadMessage();
    }

    mCurrentVertexAttributes = &mGridData.mRenderData.mVertexAttributes[0][mBufferIndex];

    if (!isMovement) {
        return;
    }

    mFrame++;
    al::LiveActor::movement();

    if (mPlayer == nullptr) {
        return;
    }

    bool isInWater = al::getTrans(mPlayer).y <= mSeaY;

    if (isInWater && !sIsWakeAttached) {
        sIsWakeAttached = true;
    } else if (!isInWater && sIsWakeAttached) {
        sIsWakeAttached = false;
    }

    if (sIsWakeAttached) {
        mOcean.attachWakeTo(mPlayer);
    } else {
        mOcean.detachWakeFrom(mPlayer);
    }
}

/**
 * Moves the actor.
 */
void OceanWater::movement() {
    update(true);
}

/**
 * Keeps the wave grid buffers in sync while the actor is paused.
 * @param isPaused unused
 */
void OceanWater::movementPaused(bool isPaused) {
    update(false);
}

/**
 * Hides the water surface.
 */
void OceanWater::hide() {
    mIsVisible = false;
}

/**
 * Shows the water surface.
 */
void OceanWater::show() {
    mIsVisible = true;
}

/**
 * Sets the scale of the wave heights.
 * @param scale target scale
 * @param isImmediate whether to apply the scale at once instead of blending to it
 */
void OceanWater::setHeightMapScale(f32 scale, bool isImmediate) {
    mTargetHeightMapScale = scale;

    if (isImmediate) {
        mHeightMapScale = scale;
    }
}

/**
 * Sets up the vertex streams of an OceanVertex vertex buffer.
 * @param pBuffer vertex buffer
 * @param pAttribute vertex attribute to create
 */
void OceanWater::setupStreams(agl::VertexBuffer* pBuffer, agl::VertexAttribute* pAttribute) {
    pBuffer->setUpStream(0, cFormatFloat3, 0, false);
    pBuffer->setUpStream(1, cFormatFloat3, 12, false);
    pBuffer->setUpStream(2, cFormatFloat2, 24, false);
    pBuffer->setUpStream(3, cFormatFloat4, 32, false);
    pBuffer->setUpStream(4, cFormatFloat3, 48, false);
    pBuffer->setUpStream(5, cFormatFloat3, 60, false);
    pAttribute->create(6, nullptr);
    pAttribute->setVertexStream(0, pBuffer, 0);
    pAttribute->setVertexStream(1, pBuffer, 1);
    pAttribute->setVertexStream(2, pBuffer, 2);
    pAttribute->setVertexStream(3, pBuffer, 3);
    pAttribute->setVertexStream(4, pBuffer, 4);
    pAttribute->setVertexStream(5, pBuffer, 5);
    pAttribute->setUp();
}

/**
 * Calculates the normals of the vertices (unused).
 * @param rVertices vertices
 */
void OceanWater::calculateNormals(agl::GPUMemBlock<OceanVertex>& rVertices) {}

/**
 * Calculates the UVs of the vertices (unused).
 */
void OceanWater::calculateUVs() {}

/**
 * Binds the textures of the deferred water shader.
 * @param pProgram shader program to bind to
 */
void OceanWater::bindTexturesDeferred(const agl::ShaderProgram* pProgram) const {
    agl::SamplerLocation baseLocation("cBase");
    baseLocation.search(*pProgram);
    agl::SamplerLocation normalLocation("cNormal");
    normalLocation.search(*pProgram);
    agl::SamplerLocation normal2Location("cNormal2");
    normal2Location.search(*pProgram);
    agl::SamplerLocation sceneLocation("cScene");
    sceneLocation.search(*pProgram);
    agl::SamplerLocation viewDepthLocation("cViewDepth");
    viewDepthLocation.search(*pProgram);
    agl::SamplerLocation inkEdgeLocation("tInkEdge");
    inkEdgeLocation.search(*pProgram);
    agl::SamplerLocation seaFoamLocation("tSeaFoam");
    seaFoamLocation.search(*pProgram);
    const agl::TextureData* indirectTexture =
        getSceneInfo()->graphicsSystemInfo->getShaderEnvTextureKeeper()->getIndirectTexture();
    agl::TextureSampler baseSampler;
    agl::TextureSampler normalSampler;
    agl::TextureSampler normal2Sampler;
    agl::TextureSampler indirectSampler;
    agl::TextureSampler inkEdgeSampler;
    agl::TextureSampler seaFoamSampler;

    if (indirectTexture != nullptr) {
        indirectSampler.applyTextureData(*indirectTexture);
        setupWaterSampler(&baseSampler);
        indirectSampler.activate(al::GameFrameworkNx::getAglDrawContext(),
                                 al::getSamplerLocationIndirect(), -1, false);
    }

    baseSampler.applyTextureData(mTextures[mTextureSetIndex][0]);
    setupWaterSampler(&baseSampler);
    baseSampler.activate(al::GameFrameworkNx::getAglDrawContext(), baseLocation, -1, false);
    normalSampler.applyTextureData(mTextures[mTextureSetIndex][1]);
    setupNormalSampler(&normalSampler);
    normalSampler.activate(al::GameFrameworkNx::getAglDrawContext(), normalLocation, -1, false);
    normal2Sampler.applyTextureData(mTextures[mTextureSetIndex][2]);
    setupNormalSampler(&normal2Sampler);
    normal2Sampler.activate(al::GameFrameworkNx::getAglDrawContext(), normal2Location, -1, false);
    inkEdgeSampler.applyTextureData(mInkEdgeTexture);
    inkEdgeSampler.setFilter(1, 1, 2);
    inkEdgeSampler.setWrap(1, 7, 1);
    inkEdgeSampler.activate(al::GameFrameworkNx::getAglDrawContext(), inkEdgeLocation, -1, false);
    seaFoamSampler.applyTextureData(mSeaFoamTexture);
    setupWaterSampler(&seaFoamSampler);
    seaFoamSampler.activate(al::GameFrameworkNx::getAglDrawContext(), seaFoamLocation, -1, false);
    al::CubeMapDirector* cubeMapDirector = getSceneInfo()->graphicsSystemInfo->getCubeMapDirector();
    al::ShaderFresnelTextureKeeper* fresnelKeeper =
        getSceneInfo()->graphicsSystemInfo->getShaderEnvTextureKeeper()->getFresnelTextureKeeper();
    s32 cubeMapIndex = mDeferredCubeMapIndex;
    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (controller != nullptr && controller->isDisasterModeAnim()) {
        cubeMapIndex = mDisasterCubeMapIndex;
    }

    if (cubeMapIndex < 0) {
        al::GraphicsAreaDirector* areaDirector =
            getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector();

        if (areaDirector != nullptr) {
            al::CurrentGraphicsAreaParam param;
            areaDirector->getCurrentGraphicsAreaParam(
                &param, al::GraphicsAreaParamType::CubeMapCapturePoint);

            if (param.mPriority < 0 || param.mPriority > 1000) {
                cubeMapIndex = -1;
            }
        }
    }

    cubeMapDirector->activateCubeMapTexture(cubeMapIndex, 0, 0, false);
    fresnelKeeper->activateFresnelTexture(mOceanData.mFresnelType, false);
}

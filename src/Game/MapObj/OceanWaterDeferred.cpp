#include "MapObj/OceanWaterDeferred.hpp"

#include <cmath>
#include <cstring>
#include <common/aglDrawContext.h>
#include <common/aglGPUMemAddr.h>
#include <common/aglRenderBuffer.h>
#include <common/aglRenderTarget.h>
#include <common/aglShaderLocation.h>
#include <common/aglShaderProgram.h>
#include <common/aglTextureSampler.h>
#include <common/aglVertexAttribute.h>
#include <common/aglVertexBuffer.h>
#include <driver/aglGraphicsDriverMgr.h>
#include <g3d/aglNW4FToNN.h>
#include <g3d/aglTextureDataInitializerG3D.h>
#include <gfx/seadCamera.h>
#include <gfx/seadGraphics.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <heap/seadHeap.h>
#include <nerd/nerdMath.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_ResFile.h>
#include <nn/g3d/g3d_Resources.h>
#include <nn/gfx/gfx_Texture.h>
#include <nvn/nvn_FuncPtrInline.h>
#include <prim/seadDelegate.h>
#include <prim/seadSafeString.h>
#include <shadow/aglDepthShadow.h>
#include <shadow/aglShadowMap.h>
#include <thread/seadDelegateThread.h>

#include "AreaObj/InkArea.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Fog/FogDirector.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/Util.hpp"
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
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/CubeMapDirector.hpp"
#include "Library/Shader/DeferredRendering/FullScreenQuadModel.hpp"
#include "Library/Shader/DeferredRendering/GBufferArray.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/Shader/ForwardRendering/ShaderEnvTextureKeeper.hpp"
#include "Library/Shader/ForwardRendering/ShaderFresnelTextureKeeper.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Shadow/Depth/DepthShadowDrawer.hpp"
#include "Library/Shadow/ShadowDirector.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
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

/**
 * Size of the water UV space used by the indirect water shaders.
 */
sead::Vector4f cWaterUVSizeIndirect;

/**
 * Level of detail of the wave patches: vertex spacing scale near and far from the patch center.
 */
f32 gLODScaleMinIndirect;
f32 gLODScaleMaxIndirect;
f32 gLODDistMinIndirect;
f32 gLODDistMaxIndirect;
f32 gLODDistMinSqIndirect;
f32 gLODDistMaxSqIndirect;
f32 gLODRatioMultIndirect;

namespace {
using TextureViewImpl = nn::gfx::detail::TextureViewImpl<nn::gfx::ApiVariationNvn8>;
using OceanWaterIndirectDelegate = sead::Delegate2<OceanWaterIndirect, sead::Thread*, s64>;

/**
 * Water data file of each story phase, used when an ink area does not force one.
 */
const char* const cPhaseWaterDataNames[] = {
    "WaterDataPhase0", "WaterDataPhase1",  "WaterDataBoss1",
    "WaterDataPhase2", "WaterDataBoss2",   "WaterDataPhase3",
    "WaterDataBoss3",  "WaterDataPhase3PlessieChase", "WaterDataPhase4",
    "WaterDataBoss4",  "WaterDataPhase4PlessieChase",
};

const sead::Vector4f cIndirectLightParam = {1.0f, 1.0f, 10.0f, 132.0f};
const sead::Vector3f cIndirectLightPos = {-140.0f, 64.0f, 738.0f};
const sead::Color4f cIndirectLightColor = {1.0f, 1.0f, 1.0f, 0.0f};
const sead::Vector4f cIndirectWaveParam = {690.0f, 12.0f, 489.0f, 407.13f};
const sead::Color4f cIndirectDeepColor = {0.465f, 0.797f, 0.991f, 1.0f};
const sead::Color4f cIndirectWhiteColor = {1.0f, 1.0f, 1.0f, 1.0f};

const al::UniformBlockLayout cIndirectUniformLayout[] = {
    {0, agl::UniformBlock::cType_Vec4, 1},   {1, agl::UniformBlock::cType_Vec3, 1},
    {2, agl::UniformBlock::cType_Vec4, 1},   {3, agl::UniformBlock::cType_Vec3, 1},
    {4, agl::UniformBlock::cType_Vec4, 1},   {5, agl::UniformBlock::cType_Float, 1},
    {6, agl::UniformBlock::cType_Vec3, 1},   {7, agl::UniformBlock::cType_Float, 1},
    {8, agl::UniformBlock::cType_Vec4, 1},   {9, agl::UniformBlock::cType_Float, 1},
    {10, agl::UniformBlock::cType_Float, 1}, {11, agl::UniformBlock::cType_Vec4, 1},
    {12, agl::UniformBlock::cType_Float, 1}, {13, agl::UniformBlock::cType_Float, 1},
    {14, agl::UniformBlock::cType_Float, 1}, {15, agl::UniformBlock::cType_Float, 1},
    {16, agl::UniformBlock::cType_Vec4, 1},  {17, agl::UniformBlock::cType_Int, 1},
    {18, agl::UniformBlock::cType_UInt, 1},  {19, agl::UniformBlock::cType_Float, 1},
    {20, agl::UniformBlock::cType_Vec4, 1},  {21, agl::UniformBlock::cType_Vec4, 1},
    {22, agl::UniformBlock::cType_Float, 1}, {23, agl::UniformBlock::cType_Vec4, 1},
    {24, agl::UniformBlock::cType_Vec4, 1},  {25, agl::UniformBlock::cType_Vec3, 1},
    {26, agl::UniformBlock::cType_Float, 1}, {27, agl::UniformBlock::cType_Float, 1},
    {28, agl::UniformBlock::cType_Float, 1}, {29, agl::UniformBlock::cType_Float, 1},
    {30, agl::UniformBlock::cType_Float, 1}, {31, agl::UniformBlock::cType_Float, 1},
    {32, agl::UniformBlock::cType_Float, 1}, {33, agl::UniformBlock::cType_Vec2, 1},
};

bool sIsWakeAttachedIndirect = false;

constexpr s64 cPatchSize = 120;
constexpr f32 cGridCellSize = 2000.0f;

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
 * Calls the commands recorded in a display list.
 * @param rDisplayList display list to call
 */
inline void callDisplayList(const agl::DisplayList& rDisplayList) {
    NVNcommandBuffer* commandBuffer =
        agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext());
    nvnCommandBufferCallCommands(commandBuffer, 1, rDisplayList.getHandlePtr());
}

/**
 * Resets the size of the indirect water UV space.
 */
inline void resetWaterUVSize() {
    cWaterUVSizeIndirect = {500.0f, 500.0f, 0.001f, 0.001f};
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
 * Gets the CPU mapping of a GPU memory block.
 * @param rBlock block to map
 * @return The first element of the block.
 */
template <typename T>
T* getBufferPtr(const agl::GPUMemBlock<T>& rBlock) {
    return reinterpret_cast<T*>(
        static_cast<u8*>(nvnMemoryPoolMap(rBlock.getMemoryPool()->getDriverPool())) +
        rBlock.getByteOffset());
}

/**
 * Sets the normal of a triangle corner to the normal of the triangle.
 * @param pVertex corner to set the normal of
 * @param rNext next corner of the triangle
 * @param rPrev previous corner of the triangle
 */
inline void calcFaceNormal(OceanVertex* pVertex, const OceanVertex& rNext,
                           const OceanVertex& rPrev) {
    sead::Vector3f edgePrev = rPrev.mPos - pVertex->mPos;
    sead::Vector3f edgeNext = rNext.mPos - pVertex->mPos;
    sead::Vector3f normal;
    normal.setCross(edgePrev, edgeNext);
    f32 invLength = 1.0f / nerd::sqrt(normal.squaredLength());
    pVertex->mNormal = {invLength * normal.x, invLength * normal.y, invLength * normal.z};
}

/**
 * Draws the triangles of an index stream.
 * @param pDrawContext draw context to draw with
 * @param rStream index stream
 */
inline void drawIndexStream(agl::DrawContext* pDrawContext, const agl::IndexStream& rStream) {
    u32 count = rStream.getCount();

    if (count == 0) {
        return;
    }

    NVNdrawPrimitive primitive = rStream.getPrimitiveType();
    NVNcommandBuffer* commandBuffer = agl::driver::getNvnCommandBuffer(pDrawContext);
    u64 address = nvnBufferGetAddress(rStream.getNvnBuffer());
    nvnCommandBufferDrawElements(commandBuffer, primitive,
                                 static_cast<NVNindexType>(rStream.getFormat()), count, address);
}

/**
 * Gets the smaller of two coordinates without branching.
 * @param a first coordinate
 * @param b second coordinate
 * @return The smaller coordinate.
 */
inline s64 selectMin(s64 a, s64 b) {
    s64 mask = (a - b) >> 31;
    return (b & ~mask) | (a & mask);
}

/**
 * Gets the larger of two coordinates without branching.
 * @param a first coordinate
 * @param b second coordinate
 * @return The larger coordinate.
 */
inline s64 selectMax(s64 a, s64 b) {
    s64 mask = (b - a) >> 31;
    return (b & ~mask) | (a & mask);
}

/**
 * Moves a grid point of a wave patch to its level of detail position.
 * @param pVertex vertex to place
 * @param pPatch wave patch
 * @param x grid X position
 * @param z grid Z position
 */
inline void placeLodVertex(OceanVertex* pVertex, const WavePatch* pPatch, s64 x, s64 z) {
    s64 dx = static_cast<f32>(x) - pPatch->mCenterX;
    s64 dz = static_cast<f32>(z) - pPatch->mCenterZ;
    f32 scale = gLODScaleMinIndirect + gLODRatioMultIndirect * (dx * dx + dz * dz);
    pVertex->mPos.x = pPatch->mCenterX + scale * dx;
    pVertex->mPos.z = pPatch->mCenterZ + scale * dz;
}

/**
 * Grows the bounds of a wave patch to contain a vertex.
 * @param pPatch wave patch
 * @param rPos vertex position
 */
inline void growPatchBounds(WavePatch* pPatch, const sead::Vector3f& rPos) {
    s64 x = rPos.x;
    s64 z = rPos.z;
    pPatch->mMinX = selectMin(x, pPatch->mMinX);
    pPatch->mMaxX = selectMax(x, pPatch->mMaxX);
    pPatch->mMinZ = selectMin(z, pPatch->mMinZ);
    pPatch->mMaxZ = selectMax(z, pPatch->mMaxZ);
}

/**
 * Converts a position to ink lookup texture coordinates.
 * @param rPos position
 * @param rCenter center of the ink area
 * @param rSize size of the ink area
 * @return Coordinates in the ink area, from 0 to 1.
 */
inline sead::Vector2f calcInkAreaUV(const sead::Vector3f& rPos, const sead::Vector3f& rCenter,
                                    const sead::Vector2f& rSize) {
    f32 x = rPos.x - rCenter.x;
    f32 z = rPos.z - rCenter.z;
    f32 u = (x + rSize.x * 0.5f) / rSize.x;
    f32 v = (z + rSize.y * 0.5f) / rSize.y;
    return {u, v};
}
}  // namespace

/**
 * Constructs the deferred ocean water.
 * @param pName actor name
 */
OceanWaterDeferred::OceanWaterDeferred(const char* pName) : OceanWater(pName) {}

/**
 * Destroys the deferred ocean water.
 */
OceanWaterDeferred::~OceanWaterDeferred() {
    if (mIsInkLookupBuilt) {
        mIsInkLookupBuilt = false;
    }
}

/**
 * Initializes the ocean and loads the texture copy shader.
 * @param rInfo actor init info
 */
void OceanWaterDeferred::init(const al::ActorInitInfo& rInfo) {
    OceanWater::init(rInfo);
    mCopyTexturesShader = al::ShaderHolder::instance()->getShaderProgram("CopyTextures2");
}

/**
 * Finds the ink area of the stage and loads its ink lookup texture.
 */
void OceanWaterDeferred::initAfterPlacement() {
    OceanWater::initAfterPlacement();
    mInkArea = al::tryFindAreaObj(this, "InkArea");

    if (mInkArea == nullptr) {
        return;
    }

    s32 phase = SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(this));
    const al::AreaShape* shape = mInkArea->getAreaShape();
    shape->calcTrans(&mFlowDir);
    mInkTexture.getMipHeight(0);
    _164.x = shape->mScale.x * 500.0f * 2.0f;
    _164.y = shape->mScale.z * 500.0f * 2.0f;
    auto* inkArea = static_cast<const InkArea*>(mInkArea);

    if (inkArea != nullptr) {
        mIsShaderInk = loadInkArea(inkArea->getTextureName(), phase, inkArea->getForceInkName()) &&
                       !inkArea->isDisableShaderInk();
    }
}

/**
 * Loads the ink lookup texture and the ink edge and sea foam textures.
 * @param pTextureName name of the ink lookup texture
 * @param phase unlocked story phase
 * @param pDataName water data file, or nullptr to use the one of the phase
 * @return Whether the ink lookup and the ink edge textures were found.
 */
bool OceanWaterDeferred::loadInkArea(const char* pTextureName, s32 phase, const char* pDataName) {
    if (pTextureName == nullptr) {
        return false;
    }

    const char* dataName = cPhaseWaterDataNames[phase];

    if (pDataName != nullptr) {
        dataName = pDataName;
    }

    al::Resource* systemResource = al::findOrCreateResource("SystemData/WaterData", nullptr);
    al::StringTmp<128> archiveName("SystemData/%s", dataName);
    al::StringTmp<128> fileName("%s.bfres", dataName);
    al::Resource* resource = al::findOrCreateResource(archiveName, nullptr);

    if (systemResource == nullptr || resource == nullptr) {
        return false;
    }

    void* systemFile = systemResource->getOtherFile("WaterData.bfres", nullptr);
    void* file = resource->getOtherFile(fileName, nullptr);

    if (systemFile != nullptr && file != nullptr) {
        nn::g3d::ResFile* systemResFile = nn::g3d::ResFile::ResCast(systemFile);
        nn::g3d::ResFile* resFile = nn::g3d::ResFile::ResCast(file);

        if (systemResFile != nullptr && resFile != nullptr) {
            agl::g3d::ResFile::Setup(systemResFile);
            agl::g3d::ResFile::Setup(resFile);
            nn::gfx::ResTexture* inkTexture = agl::g3d::ResFile::GetTexture(resFile, pTextureName);

            if (inkTexture != nullptr) {
                mIsInkTextureLoaded = true;
                agl::g3d::TextureDataInitializerG3D::initialize(&mInkTexture, *inkTexture);
                mInkSamples =
                    new InkSample[mInkTexture.getWidth(0) * mInkTexture.getHeight(0)];
            }

            nn::gfx::ResTexture* inkEdgeTexture =
                agl::g3d::ResFile::GetTexture(systemResFile, "InkEdge");

            if (inkEdgeTexture != nullptr) {
                agl::g3d::TextureDataInitializerG3D::initialize(&mInkEdgeTexture, *inkEdgeTexture);
            }

            nn::gfx::ResTexture* seaFoamTexture =
                agl::g3d::ResFile::GetTexture(systemResFile, "SeaFoam");

            if (seaFoamTexture != nullptr) {
                agl::g3d::TextureDataInitializerG3D::initialize(&mSeaFoamTexture,
                                                                *seaFoamTexture);

                if (inkTexture != nullptr && inkEdgeTexture != nullptr) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * Moves the actor.
 */
void OceanWaterDeferred::movement() {
    OceanWater::movement();
}

/**
 * Checks whether a position is in the ink painted on the water.
 * @param rPos position
 * @return Whether the position is in ink.
 */
bool OceanWaterDeferred::isInInk(const sead::Vector3f& rPos) {
    if (!mIsShaderInk || mInkArea == nullptr || !mIsInkLookupBuilt) {
        return false;
    }

    u32 height = mInkTexture.getHeight(0);

    if (rPos.y > mSeaY + 10.0f) {
        return false;
    }

    sead::Vector2f uv = calcInkAreaUV(rPos, mFlowDir, _164);
    u32 width = mInkTexture.getWidth(0);
    return getInkSample(uv.x * width, uv.y * height)->getInk() != 0xf;
}

/**
 * Gets a texel of the ink lookup, clamped to the lookup size.
 * @param x column
 * @param y row
 * @return The ink sample.
 */
const OceanWaterDeferred::InkSample* OceanWaterDeferred::getInkSample(s32 x, s32 y) const {
    u32 width = mInkWidth;
    u32 height = mInkHeight;
    u32 column = x < 0 ? 0 : (x < width ? x : width - 1);
    u32 row = y < 0 ? 0 : (y < height ? y : height - 1);
    return &mInkSamples[row * width + column];
}

/**
 * Samples the wave blend of the ink lookup at a position.
 * @param rPos position
 * @return The ink sample with only its wave blend set.
 */
OceanWaterDeferred::InkSample OceanWaterDeferred::sampleBaseHeight(const sead::Vector3f& rPos) {
    InkSample result = {};

    if (mInkArea == nullptr || !mIsInkLookupBuilt) {
        return result;
    }

    sead::Vector2f uv = calcInkAreaUV(rPos, mFlowDir, _164);
    f32 scaledX = uv.x * mInkHeight;
    f32 scaledY = uv.y * mInkWidth;
    f32 x = sead::Mathu::min(mInkHeight, scaledX);
    f32 y = sead::Mathu::min(mInkWidth, scaledY);
    result.mInkBlend = getInkSample(x, y)->mInkBlend & 0xf0;
    return result;
}

/**
 * Samples the ink lookup at a position.
 * @param rPos position
 * @return The ink sample, or an empty sample outside of an ink area.
 */
OceanWaterDeferred::InkSample OceanWaterDeferred::sampleTexture(const sead::Vector3f& rPos) const {
    if (mInkArea == nullptr || !mIsInkLookupBuilt) {
        return {};
    }

    sead::Vector2f uv = calcInkAreaUV(rPos, mFlowDir, _164);
    f32 scaledX = uv.x * mInkHeight;
    f32 scaledY = uv.y * mInkWidth;
    f32 x = sead::Mathu::min(mInkHeight, scaledX);
    f32 y = sead::Mathu::min(mInkWidth, scaledY);
    const InkSample* sample = getInkSample(x, y);
    return {sample->mInkBlend, sample->mHeight};
}

/**
 * Gets the height of the water surface at a position.
 * @param rPos position
 * @param isWithWave whether to add the ambient waves
 * @return The water height.
 */
f32 OceanWaterDeferred::getWaterHeight(const sead::Vector3f& rPos, bool isWithWave) {
    InkSample sample = sampleTexture(rPos);
    f32 height = sample.mHeight;
    f32 blend = sample.getBlend() / 15.0f;
    f32 baseHeight = 0.0f;

    if (blend >= 0.5f) {
        baseHeight = rPos.y > 1250.5f ? 2501.0f : 0.0f;
    }

    if (!isWithWave) {
        return baseHeight;
    }

    f32 scale = fmaxf(height / -255.0f + 1.0f, 0.5f);
    f32 rate = std::fabs(blend - 0.5f) * 2.0f * scale;
    f32 time = mFrame * (1.0f / 60.0f);
    f32 wave = sinf(mOceanData.mAmbientWave1.x * time + rPos.x / mOceanData.mAmbientWave1.y) +
               sinf(time * mOceanData.mAmbientWave2.x + rPos.z / mOceanData.mAmbientWave2.y);
    return al::lerpValue(rate, baseHeight, baseHeight + wave * mOceanData.mAmbientWaveAmplitude);
}

/**
 * Draws the water surface into the G-buffer, one tier after the other.
 */
void OceanWaterDeferred::draw() const {
    if (!mIsVisible) {
        return;
    }

    al::GraphicsSystemInfo* graphicsInfo = getSceneInfo()->graphicsSystemInfo;
    const agl::RenderBuffer* boundBuffer =
        al::GameFrameworkNx::getAglDrawContext()->getBoundRenderBuffer();

    {
        agl::RenderBuffer renderBuffer;
        agl::RenderTargetColor colorTarget;
        agl::RenderTargetColor copyTarget;
        agl::RenderTargetDepth depthTarget;
        al::ShaderEnvTextureKeeper* keeper = graphicsInfo->getShaderEnvTextureKeeper();
        keeper->setCopiedDepthTarget(boundBuffer->getRenderTargetDepth());
        const agl::RenderTargetColor* boundColor = boundBuffer->getRenderTargetColor();
        colorTarget.applyTextureData(*boundColor);
        depthTarget.applyTextureData(*boundBuffer->getRenderTargetDepth());
        s32 width = boundColor->getMipWidth(0);
        s32 height = boundColor->getMipHeight(0);
        copyTarget.applyTextureData(*keeper->getCopiedColorTexture());
        renderBuffer.setVirtualSize(sead::Vector2f(width, height));
        renderBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
        renderBuffer.setRenderTargetColorNullAll();
        renderBuffer.setRenderTargetColor(&colorTarget, 0);
        renderBuffer.setRenderTargetColor(&copyTarget, 1);
        renderBuffer.setRenderTargetDepth(&depthTarget);
        renderBuffer.bind(al::GameFrameworkNx::getDrawContext());
    }

    sead::GraphicsContext context;
    context.setDepthEnable(true, true);
    context.setDepthFunc(4);
    context.setCullingMode(2);
    context.setPolygonMode(2, 2);
    context.setColorMask(0xffff);
    context.setStencilTestEnable(true);
    context.setBlendEnableMask(0);
    context.setStencilTestRef(1);
    context.setStencilTestMask(0xff);
    context.setStencilOp(1, 1, 3);
    context.setStencilTestFunc(8);
    context.setStencilWriteMask(0xff);
    context.apply(al::GameFrameworkNx::getDrawContext());
    al::GBufferArray* gBufferArray = graphicsInfo->getDrawGBufferArray();
    gBufferArray->getGBufAlbedoTex();
    const agl::TextureData* sceneTexture =
        graphicsInfo->getShaderEnvTextureKeeper()->getIndirectTexture();
    const agl::TextureData* viewDepthTexture = gBufferArray->getGBufDepthViewTex();
    const agl::TextureData* shadowTexture = graphicsInfo->getShadowDirector()
                                                ->mDepthShadowDrawer->getDepthShadow()
                                                ->getShadowMap()
                                                .getDepthTexture();

    for (u32 tier = 1;; tier--) {
        if (WaveGrid::IsTierVisible(mGridData, tier)) {
            const char* macros[3] = {"INK_AREA", "DISPLACE_VERTEX", "TEX_COORD_CALC_TYPE"};
            const char* values[3] = {mIsShaderInk ? "1" : "0",
                                     WaveGrid::TierTriCount(mGridData, tier) > 2 ? "1" : "0",
                                     "0"};
            const agl::ShaderProgram* program = mDeferredShader->searchVariation(3, macros, values);
            program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
            bindTexturesDeferred(program);
            agl::SamplerLocation sceneLocation("cScene");
            sceneLocation.search(*program);
            agl::SamplerLocation viewDepthLocation("cViewDepth");
            viewDepthLocation.search(*program);
            agl::SamplerLocation inkAreaLocation("tInkArea");
            inkAreaLocation.search(*program);
            agl::SamplerLocation depthShadowLocation("cDepthShadow");
            depthShadowLocation.search(*program);
            agl::TextureSampler sceneSampler;
            sceneSampler.applyTextureData(*sceneTexture);
            sceneSampler.setFilter(0, 0, 2);
            sceneSampler.setWrap(7, 7, 7);
            sceneSampler.activate(al::GameFrameworkNx::getAglDrawContext(), sceneLocation, -1,
                                  false);
            agl::TextureSampler viewDepthSampler;
            viewDepthSampler.applyTextureData(*viewDepthTexture);
            viewDepthSampler.setFilter(0, 0, 1);
            viewDepthSampler.setWrap(7, 7, 7);
            viewDepthSampler.activate(al::GameFrameworkNx::getAglDrawContext(), viewDepthLocation,
                                      -1, false);
            agl::TextureSampler inkAreaSampler;
            inkAreaSampler.applyTextureData(mInkTexture);
            inkAreaSampler.setFilter(1, 1, 2);
            inkAreaSampler.setWrap(7, 7, 7);
            inkAreaSampler.activate(al::GameFrameworkNx::getAglDrawContext(), inkAreaLocation, -1,
                                    false);
            agl::TextureSampler depthShadowSampler;
            depthShadowSampler.applyTextureData(*shadowTexture);
            depthShadowSampler.setFilter(1, 1, 2);
            depthShadowSampler.setWrap(7, 7, 7);
            depthShadowSampler.activate(al::GameFrameworkNx::getAglDrawContext(),
                                        depthShadowLocation, -1, false);
            bindUniformBlock(tier);
            WaveGrid::DisplayTier(mGridData, tier, mBufferIndex);
        }

        if (tier == 0) {
            break;
        }
    }
}

/**
 * Copies the frame buffers before the water is drawn (unused).
 */
void OceanWaterDeferred::drawCopyBuffers() const {}

/**
 * Reads the ink lookup texture back from the GPU into the ink samples.
 * @return Always true.
 */
bool OceanWaterDeferred::buildSampleHeightLookup() {
    if (mIsInkLookupBuilt || !mIsInkTextureLoaded) {
        return true;
    }

    mInkMutex.lock();
    mInkAccessor.beginPeek(al::GameFrameworkNx::getAglDrawContext(), mInkTexture,
                           al::tryFindNamedHeap("AglHeap"));
    mInkWidth = mInkTexture.getMipWidth(0);
    mInkHeight = mInkTexture.getMipHeight(0);

    for (u32 y = 0; y < mInkHeight; y++) {
        for (u64 x = 0; x < mInkWidth; x++) {
            sead::Vector4<u32> color;
            mInkAccessor.peek(&color, x, y);
            InkSample& sample = mInkSamples[y * mInkWidth + x];
            sample.setInk(color.x >> 4);
            sample.setBlend(color.y >> 4);
            sample.mHeight = color.w;
        }
    }

    mInkAccessor.finalizeImageBuffer();
    mInkMutex.unlock();
    mIsInkLookupBuilt = true;
    return true;
}

/**
 * Gets the indirect ocean water scene object.
 * @param pHolder scene object holder user
 * @return The ocean water.
 */
OceanWaterIndirect* OceanWaterIndirect::getOceanWater(const al::IUseSceneObjHolder* pHolder) {
    return al::getSceneObj<OceanWaterIndirect>(pHolder, SceneObjID_OceanWater);
}

/**
 * Constructs the indirect ocean water and initializes the wave simulation.
 * @param pName actor name
 */
OceanWaterIndirect::OceanWaterIndirect(const char* pName) : OceanWaveDirector(pName) {
    initWavePatch();
    mGenerator.init(this);
    mGenerator.initOcean(&mOcean);
    mOcean.init();
    mSeaOffset = mGenerator.getSeaOffset();
    mPolygonModeFront = 2;
    mPolygonModeBack = 2;
    _556a0 = 0;
    mWaveMode = 1;
    mReflectionType = 2;
    mRenderMode = OceanWater::RenderMode_Forward;
    mFresnelType = 14;
    mReflectionRayMaxStep = 10.0f;
    mReflectionRayScale.x = 50.0f;
    mReflectionRayScale.y = 1.01f;
    mIsGaussianBlur = false;
    mIsLerping = false;
    mFromModeIndex = 0;
    mToModeIndex = 1;
}

/**
 * Sets up the wave patches, the round patch mask and its border, and the normal scale table.
 */
void OceanWaterIndirect::initWavePatch() {
    mWavePatchNum = 1;

    for (u64 i = 0; i < mWavePatchNum; i++) {
        memset(&mWavePatches[i], 0, sizeof(WavePatch));
        mWavePatches[i].mSize = cPatchSize;
        mWavePatches[i].mPointNum = cPatchSize * cPatchSize;
    }

    u64 i = 0;

    for (; i < mWavePatchNum; i++) {
        WavePatch& patch = mWavePatches[i];
        patch.mX = 0;
        patch.mZ = 0;
        patch.mSize = cPatchSize;
        patch.mPointNum = cPatchSize * cPatchSize;
    }

    for (; i < 4; i++) {
        memset(&mWavePatches[i], 0, sizeof(WavePatch));
    }

    for (s64 z = 0; z < cPatchSize / 2; z++) {
        s64 halfLength = sqrt(cPatchSize * cPatchSize / 4 - z * z) + 0.5;
        mRowSpans[cPatchSize / 2 + z].mStart = cPatchSize / 2 - halfLength;
        mRowSpans[cPatchSize / 2 - 1 - z].mStart = cPatchSize / 2 - halfLength;
        mRowSpans[cPatchSize / 2 + z].mLength = halfLength * 2;
        mRowSpans[cPatchSize / 2 - 1 - z].mLength = halfLength * 2;
    }

    for (u64 i = 0; i < mWavePatchNum; i++) {
        WavePatch& patch = mWavePatches[i];
        setupVertices(patch.mSize - 1, patch.mSize - 1, patch.mSize, patch.mSize);
        setupIndices(patch.mSize - 1, patch.mSize - 1);
        patch.mVertexBlock = &mVertexBlocks[0];
    }

    u8 mask[cPatchSize][cPatchSize];
    memset(mask, 0, sizeof(mask));

    for (s64 z = 0; z < cPatchSize; z++) {
        const RowSpan& span = mRowSpans[z];

        for (s64 x = span.mStart + span.mLength - 1; x >= span.mStart; x--) {
            mask[z][x] = 0xff;
        }
    }

    for (s64 z = 0; z < cPatchSize; z++) {
        const RowSpan& span = mRowSpans[z];

        for (s64 x = span.mStart + span.mLength - 1; x >= span.mStart; x--) {
            if (z == 0 || z == cPatchSize - 1) {
                if (mask[z][x] == 0xff) {
                    mask[z][x] = 1;
                }
            } else if (mask[z][x] == 0xff) {
                if (x == cPatchSize - 1 || x == 0 || mask[z - 1][x - 1] == 0 ||
                    mask[z - 1][x] == 0 || mask[z - 1][x + 1] == 0 || mask[z][x - 1] == 0 ||
                    mask[z][x + 1] == 0 || mask[z + 1][x - 1] == 0 || mask[z + 1][x] == 0 ||
                    mask[z + 1][x + 1] == 0) {
                    mask[z][x] = 1;
                }
            }
        }
    }

    memset(mBorders[0].mIndices, 0xff, sizeof(mBorders[0].mIndices));
    mBorders[0].mNum = 0;

    for (s64 z = 0; z < cPatchSize; z++) {
        for (s64 x = 0; x < cPatchSize; x++) {
            u8 value = mask[z][x];

            if (value != 0 && value != 0xff) {
                BorderList& border = mBorders[value - 1];
                border.mIndices[border.mNum] = z * cPatchSize + x;
                border.mNum++;
            }
        }
    }

    for (s64 z = 0; z < 64; z++) {
        f32 distZ = static_cast<f32>(z * z) / 961.0f;

        for (s64 x = 0; x < 64; x++) {
            f32 distX = static_cast<f32>(x * x) / 961.0f;
            mNormalScales[z][x] = 1.0f / sqrtf(distZ + (distX + 1.0f));
        }
    }
}

/**
 * Stops the water thread and releases the uniform block.
 */
OceanWaterIndirect::~OceanWaterIndirect() {
    if (mThread != nullptr) {
        kill();

        if (mThread != nullptr) {
            delete mThread;
            mThread = nullptr;
        }
    }

    if (mUniformBlock != nullptr) {
        delete mUniformBlock;
        mUniformBlock = nullptr;
    }
}

/**
 * Initializes the shaders, the textures of the water model and the water thread.
 * @param rInfo actor init info
 */
void OceanWaterIndirect::init(const al::ActorInitInfo& rInfo) {
    mShader = al::ShaderHolder::instance()->getShaderProgram("WaterIndirect");
    mNormalsShader = al::ShaderHolder::instance()->getShaderProgram("WaterNormalsIndirect");
    mReflectionsShader = al::ShaderHolder::instance()->getShaderProgram("WaterSSRIndirect");
    mComposeShader = al::ShaderHolder::instance()->getShaderProgram("WaterComposeIndirect");
    mDeferredShader =
        al::ShaderHolder::instance()->getShaderProgram("WaterDeferredModelIndirect");
    mQuadModel = new al::FullScreenQuadModel();
    mVertexNums[0] = 0;
    mVertexNums[1] = 0;
    mVertexNums[2] = 0;
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::invalidateClipping(this);

    if (mActorPoseKeeper == nullptr) {
        al::initActorPoseTQSV(this);
    }

    if (!al::trySyncStageSwitchAppear(this)) {
        al::trySyncStageSwitchKill(this);
    }

    mUniformBlock = al::createUniformBlock(cIndirectUniformLayout, 34, nullptr, 2);
    const nn::g3d::MaterialObj* material =
        getModelKeeper()->getModelCafe()->getModelG3D()->getMaterialObj(0);

    for (s32 i = 0; i < material->GetResource()->GetSamplerCount(); i++) {
        const nn::g3d::ResMaterial* res = material->GetResource();

        if (strcmp(getSamplerName(res, i), "albedo0") == 0) {
            initTextureFromMaterial(&mTextures[0], material, i);
        } else if (strcmp(getSamplerName(res, i), "normal0") == 0) {
            initTextureFromMaterial(&mTextures[1], material, i);
        } else if (strcmp(getSamplerName(res, i), "normal1") == 0) {
            initTextureFromMaterial(&mTextures[2], material, i);
        } else if (strcmp(getSamplerName(res, i), "noise") == 0) {
            initTextureFromMaterial(&mTextures[3], material, i);
        }
    }

    mThread = new sead::DelegateThread(
        "WaterThread", new OceanWaterIndirectDelegate(this, &OceanWaterIndirect::threadFunc_),
        nullptr, sead::Thread::cDefaultPriority, sead::MessageQueue::BlockType::NonBlocking,
        0x7fffffff, 0x2000, 32);
    mThread->setAffinity(sead::CoreIdMask(sead::CoreId::cSub1));
    mRequestQueue.allocate(1, nullptr);
    mResultQueue.allocate(1, nullptr);
}

/**
 * Updates the ocean simulation and the wave patches in the water thread.
 * @param pThread water thread
 * @param message unused
 */
void OceanWaterIndirect::threadFunc_(sead::Thread* pThread, s64 message) {
    s64 bufferIndex = mRequestQueue.pop(sead::MessageQueue::BlockType::Blocking);

    if (bufferIndex == -1) {
        return;
    }

    mThreadBufferIndex = bufferIndex;
    f32 x;
    f32 z;
    f32 dirX;
    f32 dirZ;
    setSeaCenterPosition(0, &x, &z, &dirX, &dirZ);
    WavePatchSetPosition(0, x, z, dirX, dirZ);
    const sead::LookAtCamera& camera = mActorSceneInfo->cameraDirector->getLookAtMain();
    sead::Vector3f diff = camera.getAt() - camera.getPos();
    f32 invLength = 1.0f / nerd::sqrt(diff.squaredLength());
    sead::Vector3f dir(invLength * diff.x, invLength * diff.y, invLength * diff.z);
    mOcean.update(camera.getPos(), dir);
    mGenerator.update(mOcean.getTime());
    updateWavePatch();
    const agl::VertexBuffer* vertexBuffer = mVertexBuffers[mThreadBufferIndex];
    vertexBuffer->flushCPUCache(0, vertexBuffer->getBufferSize());
    mResultQueue.push(mThreadBufferIndex, sead::MessageQueue::BlockType::Blocking);
}

/**
 * Gets the center of the sea, which follows the player.
 * @param unused unused
 * @param pX output X position
 * @param pZ output Z position
 * @param pDirX output X of the horizontal front direction
 * @param pDirZ output Z of the horizontal front direction
 */
void OceanWaterIndirect::setSeaCenterPosition(long unused, f32* pX, f32* pZ, f32* pDirX,
                                              f32* pDirZ) {
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
 * Moves a wave patch, snapped to the grid.
 * @param index wave patch, clamped to the existing ones
 * @param x X position
 * @param z Z position
 * @param dirX unused
 * @param dirZ unused
 */
void OceanWaterIndirect::WavePatchSetPosition(long index, f32 x, f32 z, f32 dirX, f32 dirZ) {
    if (index < 0) {
        index = 0;
    } else if (index >= mWavePatchNum) {
        index = mWavePatchNum - 1;
    }

    mWavePatches[index].mX = static_cast<s64>(x) & ~63;
    mWavePatches[index].mZ = static_cast<s64>(z) & ~63;
}

/**
 * Updates the heights and the vertices of every wave patch.
 */
void OceanWaterIndirect::updateWavePatch() {
    if (mWavePatchNum == 0) {
        return;
    }

    for (u64 i = 0; i < mWavePatchNum; i++) {
        mWavePatches[i].mVertexBlock = &mVertexBlocks[mThreadBufferIndex];
    }

    for (u64 i = 0; i < mWavePatchNum; i++) {
        WavePatch* patch = &mWavePatches[i];
        wavePatchCalcXZ(patch);
        waveOceanUpdateWavePatch(patch);
        waveGenUpdateWavePatch(patch);
        wavePatchCalcNormal(patch);
        wavePatchExtendBorder(patch);
    }
}

/**
 * Updates the water simulation (unused).
 * @param rate unused
 */
void OceanWaterIndirect::updateWaterSimultion(f32 rate) {}

/**
 * Starts the water thread once every actor is placed.
 */
void OceanWaterIndirect::initAfterPlacement() {
    mSeaY = al::getTrans(this).y;
    mFrame = 0;
    mPlayer = al::getPlayerActor(this, 0);
    mSeaOffset = mSeaY;
    al::tryFindAreaObj(this, "OceanWaveGenerator");
    mThread->start();
    mRequestQueue.push(mThreadBufferIndex, sead::MessageQueue::BlockType::Blocking);
    mCurrentVertexAttributes = &mVertexAttributes[mBufferIndex];
}

/**
 * Reads the water parameters of the stage.
 * @param rIter water parameter file
 * @param pName unused
 */
void OceanWaterIndirect::initFromYaml(const al::ByamlIter& rIter, const char* pName) {
    rIter.tryGetIntByKey(&mReflectionType, "ReflectionType");
    rIter.tryGetIntByKey(&mFresnelType, "FresnelType");
    rIter.tryGetFloatByKey(&mReflectionRayMaxStep, "ReflectionRayMaxStep");
    rIter.tryGetFloatByKey(&mReflectionRayScale.x, "ReflectionRayStartScale");
    rIter.tryGetFloatByKey(&mReflectionRayScale.y, "ReflectionRayScaleFactor");
    initModeFromYaml("NormalMode", mModes[0], rIter);
    initModeFromYaml("DisasterMode", mModes[1], rIter);
}

/**
 * Reads the look of one mode.
 * @param pName mode name
 * @param rData look to fill
 * @param rIter water parameters
 */
void OceanWaterIndirect::initModeFromYaml(const char* pName, OceanData& rData,
                                          const al::ByamlIter& rIter) {
    al::ByamlIter iter;

    if (!rIter.tryGetIterByKey(&iter, pName)) {
        iter = rIter;
    }

    initColorFromYaml("Ambient", rData.mAmbient, iter);
    initMaterialFromYaml("Albedo", rData.mAlbedoUV, rData.mAlbedoUVRot, rData.mAlbedoIntensity,
                         iter);
    initMaterialFromYaml("NormalMap1", rData.mNormalMap1UV, rData.mNormalMap1UVRot,
                         rData.mNormalMap1Intensity, iter);
    initMaterialFromYaml("NormalMap2", rData.mNormalMap2UV, rData.mNormalMap2UVRot,
                         rData.mNormalMap2Intensity, iter);
    initRefractionFromYaml("Refraction", rData.mRefractionColor, rData.mRefractionFadeHeight,
                           iter);
    initReflectionFromYaml("Reflection", rData, iter);
}

/**
 * Reads a color.
 * @param pName key of the color
 * @param rColor color to fill
 * @param rIter iterator holding the color
 */
void OceanWaterIndirect::initColorFromYaml(const char* pName, sead::Color4f& rColor,
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
void OceanWaterIndirect::initMaterialFromYaml(const char* pName, sead::Vector2f& rUV,
                                              sead::Vector2f& rUVScroll,
                                              const al::ByamlIter& rIter) {
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
void OceanWaterIndirect::initMaterialFromYaml(const char* pName, sead::Vector4f& rUV,
                                              f32& rUVRot, f32& rIntensity,
                                              const al::ByamlIter& rIter) {
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
 * @param rFadeHeight fade height to fill
 * @param rIter iterator holding the refraction
 */
void OceanWaterIndirect::initRefractionFromYaml(const char* pName, sead::Color4f& rColor,
                                                f32& rFadeHeight, const al::ByamlIter& rIter) {
    al::ByamlIter iter;
    rIter.tryGetIterByKey(&iter, pName);
    al::ByamlIter colorIter;
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
void OceanWaterIndirect::initReflectionFromYaml(const char* pName, OceanData& rData,
                                                const al::ByamlIter& rIter) {
    al::ByamlIter iter;

    if (!rIter.tryGetIterByKey(&iter, pName)) {
        return;
    }

    iter.tryGetFloatByKey(&rData.mReflectionBias, "Bias");
    iter.tryGetFloatByKey(&rData.mReflectionBlend, "Blend");
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
bool OceanWaterIndirect::createWave(const al::LiveActor* pActor, const al::OceanWaveInfo* pInfo) {
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
 * Gets the height of the sea.
 * @param rPos unused
 * @return The sea height.
 */
f32 OceanWaterIndirect::getY(const sead::Vector3f& rPos) {
    return mSeaY;
}

/**
 * Stops the water thread.
 */
void OceanWaterIndirect::kill() {
    if (mThread != nullptr) {
        mRequestQueue.push(-1, sead::MessageQueue::BlockType::Blocking);
        mThread->quit(false);
        mThread->waitDone();
    }
}

/**
 * Appears and starts the appear action.
 */
void OceanWaterIndirect::appear() {
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
bool OceanWaterIndirect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
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
void OceanWaterIndirect::draw() const {
    al::LiveActor::draw();

    if (al::isDead(this)) {
        return;
    }

    pushDebugGroup("WATER");
    sead::GraphicsContext context;
    context.setDepthEnable(true, true);
    context.setDepthFunc(2);
    context.setCullingMode(0);
    context.setPolygonMode(mPolygonModeFront, mPolygonModeBack);
    context.setBlendEnable(0, false);
    context.setBlendEquationA(0, 1);
    context.apply(al::GameFrameworkNx::getDrawContext());

    if (mRenderMode == OceanWater::RenderMode_Indirect) {
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

    if (mRenderMode == OceanWater::RenderMode_Deferred) {
        sead::GraphicsContext deferredContext;
        deferredContext.setDepthEnable(true, true);
        deferredContext.setDepthFunc(2);
        deferredContext.setCullingMode(0);
        deferredContext.setPolygonMode(mPolygonModeFront, mPolygonModeBack);
        deferredContext.apply(al::GameFrameworkNx::getDrawContext());
        mDeferredShader->getVariation(0)->activate(al::GameFrameworkNx::getAglDrawContext(), true);
        mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
        callDisplayList(mDisplayList);
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

    resetWaterUVSize();
    const agl::ShaderProgram* program = mShader->searchVariation(2, macros, values);

    if (program == nullptr) {
        return;
    }

    bindUniformBlock(nullptr);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    bindTextures(program);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());

    if (mWaveMode <= 1) {
        callDisplayList(mDisplayList);
    }

    if (mWaveMode == 0 || mWaveMode == 2) {
        mMeshVertexAttributes[mBufferIndex].activate(al::GameFrameworkNx::getAglDrawContext());
        s32 vertexNum = mMeshVertexBlocks[mBufferIndex].getSize() / sizeof(OceanVertex);

        if (vertexNum != 0) {
            nvnCommandBufferDrawArrays(
                agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext()),
                NVN_DRAW_PRIMITIVE_TRIANGLES, 0, vertexNum);
        }
    }

    if (mWaveMode == 3) {
        mGridVertexAttributes[mBufferIndex].activate(al::GameFrameworkNx::getAglDrawContext());
        u32 indexNum = mIndexNums[mBufferIndex];

        if (indexNum != 0) {
            const agl::IndexStream& indexStream = mIndexStreams[mBufferIndex];
            NVNdrawPrimitive primitive = indexStream.getPrimitiveType();
            NVNcommandBuffer* commandBuffer =
                agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext());
            u64 address = nvnBufferGetAddress(indexStream.getNvnBuffer());
            nvnCommandBufferDrawElements(commandBuffer, primitive,
                                         static_cast<NVNindexType>(indexStream.getFormat()),
                                         indexNum, address);
        }
    }

    popDebugGroup();
}

/**
 * Draws the normals of the water surface.
 */
void OceanWaterIndirect::drawNormals() const {
    pushDebugGroup("Water Normals Indirect");
    getSceneInfo();
    resetWaterUVSize();
    const agl::ShaderProgram* program = mNormalsShader->getVariation(0);
    bindUniformBlock(nullptr);

    {
        agl::SamplerLocation normalLocation("cNormal");
        normalLocation.search(*program);
        agl::SamplerLocation normal2Location("cNormal2");
        normal2Location.search(*program);
        agl::TextureSampler normalSampler;
        agl::TextureSampler normal2Sampler;
        normalSampler.applyTextureData(mTextures[1]);
        setupWaterSampler(&normalSampler);
        normalSampler.activate(al::GameFrameworkNx::getAglDrawContext(), normalLocation, -1,
                               false);
        normal2Sampler.applyTextureData(mTextures[2]);
        setupWaterSampler(&normal2Sampler);
        normal2Sampler.activate(al::GameFrameworkNx::getAglDrawContext(), normal2Location, -1,
                                false);
        program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
        mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
        callDisplayList(mDisplayList);
    }

    popDebugGroup();
}

/**
 * Draws the screen space reflections of the water surface.
 */
void OceanWaterIndirect::drawReflections() const {
    pushDebugGroup("Water Reflections");
    getSceneInfo();
    resetWaterUVSize();
    const agl::ShaderProgram* program = mReflectionsShader->getVariation(0);
    bindUniformBlock(nullptr);
    bindTextures(program);
    agl::SamplerLocation normalLocation("cNormal");
    normalLocation.search(*program);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mDisplayList);
    popDebugGroup();
}

/**
 * Composes the water surface into a render buffer.
 * @param pRenderBuffer render buffer to draw into
 */
void OceanWaterIndirect::drawCompose(const agl::RenderBuffer* pRenderBuffer) const {
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
    bindUniformBlock(nullptr);
    bindTextures(program);
    agl::SamplerLocation sceneLocation("cScene");
    sceneLocation.search(*program);
    program->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mDisplayList);
    popDebugGroup();
}

/**
 * Draws the water surface into the G-buffer.
 */
void OceanWaterIndirect::drawDeffered() const {
    sead::GraphicsContext context;
    context.setDepthEnable(true, true);
    context.setDepthFunc(2);
    context.setCullingMode(0);
    context.setPolygonMode(mPolygonModeFront, mPolygonModeBack);
    context.apply(al::GameFrameworkNx::getDrawContext());
    mDeferredShader->getVariation(0)->activate(al::GameFrameworkNx::getAglDrawContext(), true);
    mCurrentVertexAttributes->activate(al::GameFrameworkNx::getAglDrawContext());
    callDisplayList(mDisplayList);
}

/**
 * Fills and binds the uniform block of the water shaders.
 * @param pBlock unused, the water uniform block is always used
 */
void OceanWaterIndirect::bindUniformBlock(al::UniformBlock* pBlock) const {
    al::UniformBlockSetter setter(mUniformBlock, 0);
    mUniformBlock->setValueRef(0, cIndirectLightParam);
    mUniformBlock->setValueRef(1, cIndirectLightPos);
    mUniformBlock->setValueRef(2, cIndirectLightColor);
    mUniformBlock->setValue(7, mFrame * (1.0f / 60.0f));
    f32 rate = calculateLerp();
    OceanData data = OceanData::lerp(mModes[mFromModeIndex], mModes[mToModeIndex], rate);
    mUniformBlock->setValueRef(4, data.mAlbedoUV);
    mUniformBlock->setValue(5, data.mAlbedoUVRot * sead::Mathf::deg2rad(1.0f));
    mUniformBlock->setValueRef(8, data.mNormalMap1UV);
    mUniformBlock->setValue(9, data.mNormalMap1UVRot * sead::Mathf::deg2rad(1.0f));
    mUniformBlock->setValue(10, data.mNormalMap1Intensity);
    mUniformBlock->setValueRef(11, data.mNormalMap2UV);
    mUniformBlock->setValue(12, data.mNormalMap2UVRot * sead::Mathf::deg2rad(1.0f));
    mUniformBlock->setValue(13, data.mNormalMap2Intensity);
    mUniformBlock->setValue(14, 0.025f);
    mUniformBlock->setValue(15, mSeaY);
    mUniformBlock->setValueRef(16, cIndirectWaveParam);
    mUniformBlock->setValue(18, 284u);
    mUniformBlock->setValue(19, 0.87f);
    mUniformBlock->setValueRef(20, cIndirectDeepColor);
    mUniformBlock->setValueRef(21, data.mRefractionColor);
    mUniformBlock->setValue(22, data.mRefractionFadeHeight);
    mUniformBlock->setValueRef(23, cWaterUVSizeIndirect);
    mUniformBlock->setValueRef(24, cIndirectWhiteColor);
    mUniformBlock->setValueRef(
        25, getCameraDirector_RS()->getSceneCameraInfo()->mLookAtCamera->getPos());
    mUniformBlock->setValue(26, data.mReflectionDistance);
    mUniformBlock->setValue(27, data.mReflectionBias);
    mUniformBlock->setValueRef(3, data.mAmbient);
    mUniformBlock->setValueRef(6, data.mAlbedoColor);
    mUniformBlock->setValue(28, data.mReflectionBlend);
    mUniformBlock->setValue(29, data.mReflectionDepthCutoff);
    mUniformBlock->setValue(30, data.mCubeMapReflectionFactor);
    mUniformBlock->setValue(31, data.mCubeMapReflectionBlend);
    mUniformBlock->setValue(32, mReflectionRayMaxStep);
    mUniformBlock->setValueRef(33, mReflectionRayScale);
    agl::ShaderLocation location;
    location.setLocation(agl::cShaderType_Vertex, 2);
    location.setLocation(agl::cShaderType_Fragment, 2);
    location.setLocation(agl::cShaderType_Geometry, 2);
    mUniformBlock->activate(al::GameFrameworkNx::getAglDrawContext(),
                            agl::ShaderLocation(location));
}

/**
 * Binds the textures of the water shaders.
 * @param pProgram shader program to bind to
 */
void OceanWaterIndirect::bindTextures(const agl::ShaderProgram* pProgram) const {
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
    agl::SamplerLocation noiseLocation("cNoise");
    noiseLocation.search(*pProgram);
    const agl::TextureData* indirectTexture =
        getSceneInfo()->graphicsSystemInfo->getShaderEnvTextureKeeper()->getIndirectTexture();
    getSceneInfo()->graphicsSystemInfo->getDrawGBufferArray()->activateSamplerNearestAlbedo(
        sceneLocation);
    getSceneInfo()->graphicsSystemInfo->getDrawGBufferArray()->activateSamplerNearestDepthView(
        viewDepthLocation);
    agl::TextureSampler baseSampler;
    agl::TextureSampler normalSampler;
    agl::TextureSampler normal2Sampler;
    agl::TextureSampler noiseSampler;
    agl::TextureSampler indirectSampler;
    indirectSampler.applyTextureData(*indirectTexture);
    setupWaterSampler(&baseSampler);
    indirectSampler.activate(al::GameFrameworkNx::getAglDrawContext(),
                             al::getSamplerLocationIndirect(), -1, false);
    baseSampler.applyTextureData(mTextures[0]);
    setupWaterSampler(&baseSampler);
    baseSampler.activate(al::GameFrameworkNx::getAglDrawContext(), baseLocation, -1, false);
    normalSampler.applyTextureData(mTextures[1]);
    setupWaterSampler(&normalSampler);
    normalSampler.activate(al::GameFrameworkNx::getAglDrawContext(), normalLocation, -1, false);
    normal2Sampler.applyTextureData(mTextures[2]);
    setupWaterSampler(&normal2Sampler);
    normal2Sampler.activate(al::GameFrameworkNx::getAglDrawContext(), normal2Location, -1, false);
    noiseSampler.applyTextureData(mTextures[3]);
    setupWaterSampler(&noiseSampler);
    noiseSampler.activate(al::GameFrameworkNx::getAglDrawContext(), noiseLocation, -1, false);
    al::CubeMapDirector* cubeMapDirector = getSceneInfo()->graphicsSystemInfo->getCubeMapDirector();
    al::ShaderFresnelTextureKeeper* fresnelKeeper =
        getSceneInfo()->graphicsSystemInfo->getShaderEnvTextureKeeper()->getFresnelTextureKeeper();
    cubeMapDirector->activateCubeMapTexture(-1, 0, 0, false);
    fresnelKeeper->activateFresnelTexture(mFresnelType, false);
}

/**
 * Receives the vertices the water thread finished, and updates the player wake.
 */
void OceanWaterIndirect::movement() {
    if (!mThread->isDone()) {
        mBufferIndex = mResultQueue.pop(sead::MessageQueue::BlockType::Blocking);
        mThreadBufferIndex = (mBufferIndex + 1) % 3;
        mRequestQueue.push(mThreadBufferIndex, sead::MessageQueue::BlockType::Blocking);
    }

    mCurrentVertexAttributes = &mVertexAttributes[mBufferIndex];
    mFrame++;
    al::LiveActor::movement();

    if (mPlayer != nullptr) {
        bool isInWater = al::getTrans(mPlayer).y <= mSeaY;

        if (isInWater && !sIsWakeAttachedIndirect) {
            sIsWakeAttachedIndirect = true;
            mOcean.attachWakeTo(mPlayer);
        } else if (!isInWater && sIsWakeAttachedIndirect) {
            sIsWakeAttachedIndirect = false;
            mOcean.attachWakeTo(nullptr);
        } else {
            mOcean.attachWakeTo(sIsWakeAttachedIndirect ? mPlayer : nullptr);
        }
    }

    mUniformBlock->swap();
}

/**
 * Fills the vertex buffers of the wave patches with a regular grid.
 * @param minX lower X bound
 * @param minZ lower Z bound
 * @param maxX upper X bound
 * @param maxZ upper Z bound
 */
void OceanWaterIndirect::setupVertices(f32 sizeX, f32 sizeZ, f32 width, f32 depth) {
    f32 stepX = width / sizeX;
    u32 vertexNum = (sizeX + 1.0f) * (sizeZ + 1.0f);
    f32 halfWidth = width * 0.5f;
    f32 halfDepth = depth * 0.5f;
    f32 invSizeX = 1.0f / sizeX;
    f32 invSizeZ = 1.0f / sizeZ;
    f32 stepZ = depth / sizeZ;

    for (s64 b = 0; b < 3; b++) {
        agl::GPUMemBlock<OceanVertex>& block = mVertexBlocks[b];
        block.allocBuffer_(vertexNum * sizeof(OceanVertex), al::getCurrentHeap(), 8,
                           agl::MemoryAttribute::Default);
        agl::GPUMemAddr<OceanVertex> vertices(block, 0);
        u32 num = 0;

        for (s32 x = 0; x <= sizeX; x++) {
            f32 y = mSeaY;
            s32 z = 0;

            for (; z <= sizeZ; z++) {
                s32 index = num + z;
                getBufferPtr(block)[index].mUV = {invSizeX * x, invSizeZ * z};
                getBufferPtr(block)[index].mNormal = {0.0f, 1.0f, 0.0f};
                getBufferPtr(block)[index].mColor = {1.0f, 0.0f, 1.0f, 1.0f};
                getBufferPtr(block)[index].mBinormal = {1.0f, 0.0f, 0.0f};
                getBufferPtr(block)[index].mTangent = {0.0f, 0.0f, 1.0f};
                getBufferPtr(block)[index].mPos = {(stepX * x - halfWidth) * 100.0f, y,
                                                    (halfDepth - stepZ * z) * 100.0f};
            }

            num += z;
        }

        mVertexBuffers[b] = new agl::VertexBuffer();
        mVertexBuffers[b]->setUpBuffer(agl::ConstGPUMemVoidAddr(block, 0), sizeof(OceanVertex),
                                       num * sizeof(OceanVertex));
        setupStreams(mVertexBuffers[b], &mVertexAttributes[b]);

        agl::GPUMemBlock<OceanVertex>& meshBlock = mMeshVertexBlocks[b];
        meshBlock.allocBuffer(6, al::getCurrentHeap(), 8, agl::MemoryAttribute::Default);
        agl::GPUMemAddr<OceanVertex> meshVertices(meshBlock, 0);
        getBufferPtr(meshBlock)[0].mPos = {-1.0f, 0.0f, 1.0f};
        getBufferPtr(meshBlock)[0].mUV = {0.0f, 0.0f};
        getBufferPtr(meshBlock)[1].mPos = {-1.0f, 0.0f, -1.0f};
        getBufferPtr(meshBlock)[1].mUV = {0.0f, 1.0f};
        getBufferPtr(meshBlock)[2].mPos = {1.0f, 0.0f, 1.0f};
        getBufferPtr(meshBlock)[2].mUV = {1.0f, 0.0f};
        getBufferPtr(meshBlock)[3].mPos = {1.0f, 0.0f, 1.0f};
        getBufferPtr(meshBlock)[3].mUV = {1.0f, 0.0f};
        getBufferPtr(meshBlock)[4].mPos = {-1.0f, 0.0f, -1.0f};
        getBufferPtr(meshBlock)[4].mUV = {0.0f, 1.0f};
        getBufferPtr(meshBlock)[5].mPos = {1.0f, 0.0f, -1.0f};
        getBufferPtr(meshBlock)[5].mUV = {1.0f, 1.0f};

        for (s32 t = 0; t < static_cast<s32>(meshBlock.getSize() / sizeof(OceanVertex)); t += 3) {
            getBufferPtr(meshBlock)[t].mUV *= 0.1f;
            getBufferPtr(meshBlock)[t + 1].mUV *= 0.1f;
            getBufferPtr(meshBlock)[t + 2].mUV *= 0.1f;
            OceanVertex& vertex0 = getBufferPtr(meshBlock)[t];
            OceanVertex& vertex1 = getBufferPtr(meshBlock)[t + 1];
            OceanVertex& vertex2 = getBufferPtr(meshBlock)[t + 2];
            calcFaceNormal(&vertex0, vertex1, vertex2);
            calcFaceNormal(&vertex1, vertex2, vertex0);
            calcFaceNormal(&vertex2, vertex0, vertex1);
            sead::Vector3f edge1 = vertex1.mPos - vertex0.mPos;
            sead::Vector3f edge2 = vertex2.mPos - vertex0.mPos;
            f32 du1 = vertex1.mUV.x - vertex0.mUV.x;
            f32 dv1 = vertex1.mUV.y - vertex0.mUV.y;
            f32 du2 = vertex2.mUV.x - vertex0.mUV.x;
            f32 dv2 = vertex2.mUV.y - vertex0.mUV.y;
            f32 rate = 1.0f / (du1 * dv2 - dv1 * du2);
            vertex2.mTangent = (edge1 * dv2 - edge2 * dv1) * rate;
            vertex1.mTangent = vertex2.mTangent;
            vertex0.mTangent = vertex1.mTangent;
            vertex2.mBinormal = (edge2 * du1 - edge1 * du2) * rate;
            vertex1.mBinormal = vertex2.mBinormal;
            vertex0.mBinormal = vertex1.mBinormal;
        }

        mMeshVertexBuffers[b] = new agl::VertexBuffer();
        mMeshVertexBuffers[b]->setUpBuffer(agl::ConstGPUMemVoidAddr(meshBlock, 0),
                                           sizeof(OceanVertex),
                                           static_cast<s32>(meshBlock.getSize()));
        setupStreams(mMeshVertexBuffers[b], &mMeshVertexAttributes[b]);

        agl::GPUMemBlock<OceanVertex>& gridBlock = mGridVertexBlocks[b];
        gridBlock.allocBuffer_(vertexNum * sizeof(OceanVertex), al::getCurrentHeap(), 8,
                               agl::MemoryAttribute::Default);
        agl::GPUMemAddr<OceanVertex> gridVertices(gridBlock, 0);
        mGridVertexBuffers[b] = new agl::VertexBuffer();
        mGridVertexBuffers[b]->setUpBuffer(agl::ConstGPUMemVoidAddr(gridBlock, 0),
                                           sizeof(OceanVertex), gridBlock.getSize());
        setupStreams(mGridVertexBuffers[b], &mGridVertexAttributes[b]);
    }
}

/**
 * Fills the index buffers of the wave patches.
 * @param sizeX number of columns
 * @param sizeZ number of rows
 */
void OceanWaterIndirect::setupIndices(f32 sizeX, f32 sizeZ) {
    s32 rowSize = sizeX + 1.0f;
    s32 stripNum = rowSize - 1;
    s32 halfRowNum = rowSize / 2;

    if (stripNum > 0) {
        mStripStreams.tryAllocBuffer(stripNum, nullptr);
        mStripBlocks.tryAllocBuffer(stripNum, nullptr);
    }

    sead::Graphics::instance()->lockDrawContext();
    agl::DrawContext drawContext;
    drawContext.setCommandBuffer(&mDisplayList);
    mCommandBlock.allocBuffer(0x2000, nullptr, 8, agl::MemoryAttribute::Default);
    agl::GPUMemAddr<u8> commands(mCommandBlock, 0);
    mDisplayList.beginDisplayListBuffer(agl::GPUMemAddr<u8>(mCommandBlock, 0),
                                        static_cast<s32>(mCommandBlock.getSize()), true);

    if (rowSize > 1) {
        s32 strip = 0;

        for (s32 row = 0; row < halfRowNum; row++) {
            const RowSpan& span = mRowSpans[row];
            u32 start = row * rowSize + span.mStart;
            setupStrip(&mStripBlocks[strip], &mStripStreams[strip], start, start + rowSize,
                       span.mLength);
            drawIndexStream(&drawContext, mStripStreams[strip]);
            strip++;

            if (row != halfRowNum - 1) {
                s32 bottomRow = rowSize - 1 - row;
                const RowSpan& bottomSpan = mRowSpans[bottomRow];
                u32 bottomStart = bottomRow * rowSize + bottomSpan.mStart;
                setupStrip(&mStripBlocks[strip], &mStripStreams[strip], bottomStart - rowSize,
                           bottomStart, bottomSpan.mLength);
                drawIndexStream(&drawContext, mStripStreams[strip]);
                strip++;
            }
        }
    }

    mDisplayList.endDisplayList();
    u32 triangleNum = (sizeX + sizeX) * sizeZ;
    s32 columnSize = sizeZ + 1.0f;

    for (u64 b = 0; b < 3; b++) {
        agl::GPUMemBlock<u32>& block = mIndexBlocks[b];
        block.allocBuffer_(triangleNum * 3 * sizeof(u32), nullptr, 8,
                           agl::MemoryAttribute::Default);
        agl::GPUMemAddr<u32> indices(block, 0);
        s32 num = 0;

        for (s32 x = 0; x < sizeX; x++) {
            for (s32 z = 0; z < sizeZ; z++) {
                u32 index0 = x * columnSize + z;
                u32 index2 = (x + 1) * columnSize + z;
                getBufferPtr(block)[num] = index0;
                getBufferPtr(block)[num + 1] = index0 + 1;
                getBufferPtr(block)[num + 2] = index2;
                getBufferPtr(block)[num + 3] = index0 + 1;
                getBufferPtr(block)[num + 4] = index2 + 1;
                getBufferPtr(block)[num + 5] = index2;
                num += 6;
            }
        }

        mIndexStreams[b].setUpStream(agl::GPUMemAddr<u32>(block, 0), num);
        mIndexNums[b] = 0;
    }

    sead::Graphics::instance()->unlockDrawContext();
}

/**
 * Computes the positions of the points of a wave patch.
 * @param pPatch wave patch
 */
void OceanWaterIndirect::wavePatchCalcXZ(WavePatch* pPatch) {
    const sead::LookAtCamera& camera = mActorSceneInfo->cameraDirector->getLookAtMain();
    mActorSceneInfo->cameraDirector->getProjectionMain();
    const sead::Vector3f& cameraPos = camera.getPos();
    sead::Vector3f pos(cameraPos.x, cameraPos.y, cameraPos.z);
    sead::Vector3f dir = camera.getAt() - cameraPos;
    dir *= 1.0f / nerd::sqrt(dir.squaredLength());
    f32 halfFovy = al::getFovy(this, 0) * 0.5f;
    pPatch->mOriginX = pos.x;
    pPatch->mOriginZ = pos.z;
    pPatch->mCenterX = pos.x;
    pPatch->mCenterZ = pos.z;
    f32 tanHalfFovy = sinf(halfFovy) / cosf(halfFovy);
    f32 scale = 1.0f;

    if (dir.y > -0.95f) {
        sead::Vector3f up(0.0f, 1.0f, 0.0f);
        up -= dir * dir.dot(up);
        up *= 1.0f / nerd::sqrt(up.squaredLength());
        f32 bottomY = dir.y + tanHalfFovy * up.y * -0.8f;

        if (bottomY < -0.05f) {
            f32 rate = (pos.y - mSeaOffset) / -bottomY;
            f32 centerX = pos.x + (dir.x - tanHalfFovy * up.x * 0.8f) * rate;
            f32 centerZ = pos.z + (dir.z - tanHalfFovy * up.z * 0.8f) * rate;
            f32 blend = 0.0f;

            if (dir.y < -0.1f) {
                f32 downRate = (pos.y - mSeaOffset) / -dir.y;
                f32 tilt = (-0.1f - dir.y) / 0.85f;
                blend = tilt * tilt;
                centerX += blend * (pos.x + dir.x * downRate - centerX);
                centerZ += blend * (pos.z + dir.z * downRate - centerZ);
            }

            sead::Vector3f front = {dir.x, 0.0f, dir.z};
            front *= 1.0f / nerd::sqrt(front.squaredLength());
            f32 height = pos.y - mSeaOffset;
            f32 dist = sqrtf((pos.x - centerX) * (pos.x - centerX) + height * height +
                             (pos.z - centerZ) * (pos.z - centerZ));
            scale = (blend * -0.5f + 1.0f) * (dist / 400.0f);
            f32 offset = 1.0f - blend;
            f32 size = pPatch->mSize;
            pPatch->mCenterX = centerX;
            pPatch->mCenterZ = centerZ;
            pPatch->mOriginX = centerX + offset * front.x * size * 64.0f * 0.3f;
            pPatch->mOriginZ = centerZ + offset * front.z * size * 64.0f * 0.3f;
            scale = sead::Mathf::clamp(scale, 0.125f, 4.0f);
        }
    } else {
        f32 rate = (pos.y - mSeaOffset) / -dir.y;
        f32 centerX = pos.x + dir.x * rate;
        f32 centerZ = pos.z + dir.z * rate;
        pPatch->mOriginX = centerX;
        pPatch->mOriginZ = centerZ;
        pPatch->mCenterX = centerX;
        pPatch->mCenterZ = centerZ;
        f32 height = pos.y - mSeaOffset;
        f32 dist = sqrtf((pos.z - centerZ) * (pos.z - centerZ) +
                         ((pos.x - centerX) * (pos.x - centerX) + height * height));
        scale = dist * 0.5f / 400.0f;
        scale = sead::Mathf::clamp(scale, 0.125f, 4.0f);
    }

    gLODScaleMinIndirect = tanHalfFovy * scale / 0.5625f;
    gLODScaleMaxIndirect = tanHalfFovy * (scale + scale) / 0.5625f;
    gLODDistMinIndirect = 0.0f;
    gLODDistMaxIndirect = 1000.0f;

    if (dir.y < -0.75f) {
        f32 tilt = (-0.75f - dir.y) / 0.2f;
        tilt = tilt > 1.0f ? 1.0f : tilt * tilt;
        gLODScaleMaxIndirect += tilt * (gLODScaleMinIndirect - gLODScaleMaxIndirect);
    }

    gLODRatioMultIndirect = (gLODScaleMaxIndirect - gLODScaleMinIndirect) / 1000000.0f;
    gLODDistMinSqIndirect = 0.0f;
    gLODDistMaxSqIndirect = 1000000.0f;
    pPatch->mMinX = pPatch->mCenterX;
    pPatch->mMaxX = pPatch->mCenterX;
    pPatch->mMinZ = pPatch->mCenterZ;
    pPatch->mMaxZ = pPatch->mCenterZ;

    f32 halfExtent = pPatch->mSize / 2 * 64;
    s64 startX = pPatch->mOriginX - halfExtent;
    s64 startZ = pPatch->mOriginZ - halfExtent;
    agl::GPUMemBlock<OceanVertex>* block = pPatch->mVertexBlock;

    for (s64 z = 0; z < pPatch->mSize; z++) {
        s64 start = mRowSpans[z].mStart;
        s64 length = mRowSpans[z].mLength;
        s64 posZ = startZ + z * 64;
        s32 index = start + z * pPatch->mSize;
        OceanVertex* vertex = &getBufferPtr(*block)[index];
        vertex->mPos.y = 0.0f;
        placeLodVertex(vertex, pPatch, start * 64 + startX, posZ);
        growPatchBounds(pPatch, vertex->mPos);

        for (s64 x = start + 1; x < start + length; x++) {
            index++;
            vertex = &getBufferPtr(*block)[index];
            vertex->mPos.y = 0.0f;
            placeLodVertex(vertex, pPatch, x * 64 + startX, posZ);
        }

        growPatchBounds(pPatch, getBufferPtr(*block)[index].mPos);
    }
}

/**
 * Adds the ocean waves to a wave patch (unused).
 * @param pPatch wave patch
 */
void OceanWaterIndirect::waveOceanUpdateWavePatch(WavePatch* pPatch) {
    if (mWaveMode == 1) {
        mOcean.updateWavePatch(pPatch, mSeaOffset);
    }
}

/**
 * Adds the generated waves to a wave patch (unused).
 * @param pPatch wave patch
 */
void OceanWaterIndirect::waveGenUpdateWavePatch(WavePatch* pPatch) {
    mGenerator.updateWavePatch(pPatch);
}

/**
 * Computes the normals of a wave patch.
 * @param pPatch wave patch
 */
void OceanWaterIndirect::wavePatchCalcNormal(WavePatch* pPatch) {
    for (s64 z = 1; z < pPatch->mSize - 1; z++) {
        s64 start = mRowSpans[z].mStart;
        s64 length = mRowSpans[z].mLength;

        for (s64 x = start + 1; x < start + length - 1; x++) {
            s32 size = pPatch->mSize;
            s32 index = z * size + x;
            OceanVertex& vertex = getBufferPtr(*pPatch->mVertexBlock)[index];
            const OceanVertex& left = getBufferPtr(*pPatch->mVertexBlock)[index - 1];
            const OceanVertex& right = getBufferPtr(*pPatch->mVertexBlock)[index + 1];
            const OceanVertex& up = getBufferPtr(*pPatch->mVertexBlock)[index - size];
            const OceanVertex& down = getBufferPtr(*pPatch->mVertexBlock)[index + size];
            f32 slopeX = (left.mPos.y - right.mPos.y) * 0.5f * (1.0f / 64.0f);
            f32 slopeZ = (up.mPos.y - down.mPos.y) * 0.5f * (1.0f / 64.0f);
            s64 cellX = slopeX * 31.0f;
            s64 cellZ = slopeZ * 31.0f;
            cellX = cellX < 0 ? -cellX : cellX;
            cellZ = cellZ < 0 ? -cellZ : cellZ;
            cellX = cellX > 63 ? 63 : cellX;
            cellZ = cellZ > 63 ? 63 : cellZ;
            f32 scale = mNormalScales[cellZ][cellX];
            vertex.mNormal = {slopeX * scale, scale, slopeZ * scale};
        }
    }
}

/**
 * Extends the heights of a wave patch beyond its border.
 * @param pPatch wave patch
 */
void OceanWaterIndirect::wavePatchExtendBorder(WavePatch* pPatch) {
    for (s64 i = 0; i < mBorders[0].mNum; i++) {
        s32 index = mBorders[0].mIndices[i];
        OceanVertex& vertex = getBufferPtr(*pPatch->mVertexBlock)[index];
        f32 x = vertex.mPos.x;
        vertex.mPos.x = pPatch->mCenterX + (x - pPatch->mCenterX) * 4.0f;
        f32 z = vertex.mPos.z;
        vertex.mPos.z = pPatch->mCenterZ + (z - pPatch->mCenterZ) * 4.0f;
        vertex.mPos.y = mSeaOffset;
        vertex.mNormal = {0.0f, 1.0f, 0.0f};
        vertex.mColor = {0.0f, 0.0f, 0.0f, 0.0f};
    }
}

/**
 * Sets up the vertex streams of an OceanVertex vertex buffer.
 * @param pBuffer vertex buffer
 * @param pAttribute vertex attribute to create
 */
void OceanWaterIndirect::setupStreams(agl::VertexBuffer* pBuffer,
                                      agl::VertexAttribute* pAttribute) {
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
 * Fills the index buffer of a triangle strip between two rows of vertices.
 * @param pBlock index buffer to allocate and fill
 * @param pStream index stream to set up
 * @param start first vertex of the first row
 * @param nextStart first vertex of the second row
 * @param length number of vertices taken from each row
 */
void OceanWaterIndirect::setupStrip(agl::GPUMemBlock<u32>* pBlock, agl::IndexStream* pStream,
                                    u32 start, u32 nextStart, s32 length) {
    pBlock->allocBuffer(length * 2, nullptr, 8, agl::MemoryAttribute::Default);
    agl::GPUMemAddr<u32> indices(*pBlock, 0);

    for (s32 i = 0; i < length; i++) {
        getBufferPtr(*pBlock)[i * 2] = start + i;
        getBufferPtr(*pBlock)[i * 2 + 1] = nextStart + i;
    }

    pStream->setUpStream(agl::GPUMemAddr<u32>(*pBlock, 0), pBlock->getSize() / sizeof(u32));
    pStream->setPrimitiveType(NVN_DRAW_PRIMITIVE_TRIANGLE_STRIP);
}

/**
 * Calculates the normals of the vertices (unused).
 * @param rVertices vertices
 */
void OceanWaterIndirect::calculateNormals(agl::GPUMemBlock<OceanVertex>& rVertices) {}

/**
 * Calculates the UVs of the vertices (unused).
 */
void OceanWaterIndirect::calculateUVs() {}

/**
 * Updates the positions of the wave patches (unused).
 */
void OceanWaterIndirect::updateWaterPatchXZ() {}

/**
 * Queues the grid cells of a view frustum slice.
 * @param rQueue grid queue
 * @param rPos camera position
 * @param rDir camera direction
 * @param rNear near distance
 * @param rFar far distance
 * @param rMin lower corner of the grid
 * @param rMax upper corner of the grid
 * @param level subdivision level of the cells
 */
s32 OceanWaterIndirect::markGridPoints(GridPointQueue& rQueue, const sead::Vector3f& rStart,
                                       const sead::Vector3f& rEnd, const f32& rNear,
                                       const f32& rFar, const sead::Vector3f& rMin,
                                       const sead::Vector3f& rMax, u8 level) {
    f32 length = nerd::sqrt((rEnd - rStart).squaredLength());
    nerd::sqrt((rMin - rMax).squaredLength());
    mActorSceneInfo->cameraDirector->getLookAtMain();
    f32 step = cGridCellSize / length;
    f32 rate = 0.0f;
    s32 count = 0;
    f32 next = rate;

    do {
        rate = sead::Mathf::min(next + rate, 1.0f);
        f32 x = ((1.0f - rate) * rStart.x + rate * rEnd.x) / cGridCellSize;
        f32 z = ((1.0f - rate) * rStart.z + rate * rEnd.z) / cGridCellSize;
        const GridPoint point = {static_cast<s32>(floorf(x)), static_cast<s32>(floorf(z)), level};

        if (mVisitedGridPoints.find(point) == nullptr) {
            rQueue.tryBirth(point);
            mVisitedGridPoints.insert(point);
            count++;
        }

        next = step;
    } while (rate < 1.0f);

    return count;
}

/**
 * Adds a quad of the water mesh, stitching it to its neighbours.
 */
void OceanWaterIndirect::addQuad(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                                 const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3,
                                 bool isEdge, s32* pEdgeIndices, u32* pIndices, u32& rIndexNum,
                                 u32* pIndices2, u32& rIndexNum2, u32& rIndexNum3,
                                 s32* pEdgeIndices2, u32 level, s32 side, u32* pIndices3) {}

/**
 * Adds a vertex to the water mesh.
 * @param rPos vertex position
 * @param rColor vertex color
 * @return The index of the vertex.
 */
u32 OceanWaterIndirect::addVertex(const sead::Vector3f& rPos, const sead::Color4f& rColor) {
    u32 bufferIndex = mThreadBufferIndex;
    s32 index = mVertexNums[bufferIndex]++;
    OceanVertex& vertex = getBufferPtr(mGridVertexBlocks[bufferIndex])[index];
    vertex.mPos = rPos;
    vertex.mColor = rColor;
    vertex.mNormal = {0.0f, 1.0f, 0.0f};
    return index;
}

/**
 * Adds an index to the index buffer the water thread fills.
 * @param index vertex index
 */
inline void OceanWaterIndirect::addIndex(u32 index) {
    u32 bufferIndex = mThreadBufferIndex;
    s32 num = mIndexNums[bufferIndex]++;
    getBufferPtr(mIndexBlocks[bufferIndex])[num] = index;
}

/**
 * Adds a triangle to the water mesh.
 * @param index0 first vertex
 * @param index1 second vertex
 * @param index2 third vertex
 */
void OceanWaterIndirect::addTriangle(u32 index0, u32 index1, u32 index2) {
    addIndex(index0);
    addIndex(index1);
    addIndex(index2);
}

/**
 * Subdivides a quad of the water mesh.
 */
void OceanWaterIndirect::subdivide(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                                   const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3,
                                   u32* pIndices, u8 maxDepth, u8 depth) {
    if (depth >= maxDepth) {
        return;
    }

    f32 y = mSeaY;
    sead::Vector3f center = {rCorner0.x + (rCorner2.x - rCorner0.x) * 0.5f, y,
                             rCorner0.z + (rCorner1.z - rCorner0.z) * 0.5f};
    sead::Vector3f left = {rCorner0.x, y, center.z};
    sead::Vector3f top = {center.x, y, rCorner0.z};
    sead::Vector3f right = {rCorner1.x + (rCorner3.x - rCorner1.x) * 0.5f, y, rCorner1.z};
    sead::Vector3f bottom = {rCorner2.x, y, rCorner2.z + (rCorner3.z - rCorner2.z) * 0.5f};
    u32 centerIndex = addVertex(center, sead::Color4f::cMagenta);
    u32 leftIndex = addVertex(left, sead::Color4f::cMagenta);
    u32 topIndex = addVertex(top, sead::Color4f::cMagenta);
    u32 rightIndex = addVertex(right, sead::Color4f::cMagenta);
    u32 bottomIndex = addVertex(bottom, sead::Color4f::cMagenta);
    addTriangle(pIndices[0], leftIndex, centerIndex);
    addTriangle(pIndices[0], centerIndex, topIndex);
    addTriangle(leftIndex, pIndices[1], rightIndex);
    addTriangle(leftIndex, rightIndex, centerIndex);
    addTriangle(topIndex, centerIndex, bottomIndex);
    addTriangle(topIndex, bottomIndex, pIndices[3]);
    addTriangle(centerIndex, rightIndex, pIndices[2]);
    addTriangle(centerIndex, pIndices[2], bottomIndex);
    u32 indices[4] = {pIndices[0], leftIndex, centerIndex, topIndex};
    subdivide(rCorner0, left, top, center, indices, maxDepth, depth + 1);
    indices[0] = leftIndex;
    indices[1] = pIndices[1];
    indices[2] = rightIndex;
    indices[3] = centerIndex;
    subdivide(left, rCorner1, center, right, indices, maxDepth, depth + 1);
    indices[0] = topIndex;
    indices[1] = centerIndex;
    indices[2] = bottomIndex;
    indices[3] = pIndices[3];
    subdivide(top, center, rCorner2, bottom, indices, maxDepth, depth + 1);
    indices[0] = centerIndex;
    indices[1] = rightIndex;
    indices[2] = pIndices[2];
    indices[3] = bottomIndex;
    subdivide(center, right, bottom, rCorner3, indices, maxDepth, depth + 1);
}

/**
 * Finds the grid cells in the view frustum and builds the water mesh from them.
 */
void OceanWaterIndirect::findAllFrustumCells() {
    const sead::LookAtCamera& camera = mActorSceneInfo->cameraDirector->getLookAtMain();
    mActorSceneInfo->cameraDirector->getProjectionMain();
    f32 halfFovy = al::getFovy(this, 0) * 0.5f;
    mActorSceneInfo->cameraDirector->getSceneCameraInfo()->getViewAt(0)->getFar();
    f32 near = getCameraDirector_RS()->getSceneCameraInfo()->getViewAt(0)->getNear();
    f32 aspect = getCameraDirector_RS()->getSceneCameraInfo()->getViewAt(0)->getAspect();
    f32 tanHalfFovy = tanf(halfFovy);
    tanf(halfFovy);
    sead::Vector3f dir = camera.getAt() - camera.getPos();
    dir *= 1.0f / nerd::sqrt(dir.squaredLength());
    sead::Vector3f side;
    side.setCross(camera.getUp(), dir);
    side *= 1.0f / nerd::sqrt(side.squaredLength());
    sead::Vector3f up;
    up.setCross(dir, side);
    f32 invUpLength = 1.0f / nerd::sqrt(up.squaredLength());
    mGridQueue.clear();
    mVisitedGridPoints.clear();
    mVertexNums[mThreadBufferIndex] = 0;
    mIndexNums[mThreadBufferIndex] = 0;
    f32 halfHeight = near * tanHalfFovy;
    up *= invUpLength;

    for (s64 i = 0;
         i < static_cast<s32>(mGridVertexBlocks[mThreadBufferIndex].getSize() / sizeof(OceanVertex));
         i++) {
        getBufferPtr(mGridVertexBlocks[mThreadBufferIndex])[i].mPos = {0.0f, 0.0f, 0.0f};
    }

    f32 halfWidth = aspect * halfHeight;
    const sead::Vector3f& pos = camera.getPos();
    sead::Vector3f nearCenter = pos + dir * near;
    f32 farTestY = dir.y * 3300.0f + nearCenter.y;
    sead::Vector3f nearBottom = nearCenter - up * halfHeight;
    sead::Vector3f nearTop = nearCenter + up * halfHeight;
    sead::Vector3f bottomDir = nearBottom - pos;
    bottomDir *= 1.0f / nerd::sqrt(bottomDir.squaredLength());
    sead::Vector3f topDir = nearTop - pos;
    f32 topLength = nerd::sqrt(topDir.squaredLength());
    f32 seaY = mSeaY;
    sead::Vector3f farPoint = {0.0f, seaY, 0.0f};
    f32 bottomRate = (seaY - pos.y) / bottomDir.y;
    sead::Vector3f nearPoint = {pos.x + bottomDir.x * bottomRate, seaY,
                                pos.z + bottomDir.z * bottomRate};

    if (farTestY > seaY) {
        farPoint.x = dir.x * 30000.0f + pos.x;
        farPoint.z = pos.z + dir.z * 30000.0f;
    } else {
        topDir *= 1.0f / topLength;
        f32 topRate = (seaY - pos.y) / topDir.y;
        farPoint.x = pos.x + topDir.x * topRate;
        farPoint.z = pos.z + topDir.z * topRate;
    }

    nerd::sqrt((farPoint - nearPoint).squaredLength());
    f32 bottomLength = nerd::sqrt((nearBottom - pos).squaredLength());
    f32 topCornerLength = nerd::sqrt((nearTop - pos).squaredLength());
    f32 nearLength = nerd::sqrt((nearPoint - pos).squaredLength());
    f32 farLength = nerd::sqrt((farPoint - pos).squaredLength());
    sead::Vector3f nearSide = side * (halfWidth * nearLength / bottomLength) * 1.1f;
    sead::Vector3f farSide = side * (halfWidth * farLength / topCornerLength) * 1.1f;
    sead::Vector3f nearLeft = nearPoint - nearSide;
    sead::Vector3f nearRight = nearSide + nearPoint;
    sead::Vector3f farLeft = farPoint - farSide;
    sead::Vector3f farRight = farSide + farPoint;
    f32 unused;
    markGridPoints(mGridQueue, nearLeft, nearRight, unused, unused, nearPoint, nearPoint, 0);
    markGridPoints(mGridQueue, farLeft, farRight, unused, unused, farPoint, farPoint, 3);
    markGridPoints(mGridQueue, nearLeft, farLeft, unused, unused, nearPoint, farPoint, 0);
    markGridPoints(mGridQueue, nearRight, farRight, unused, unused, nearPoint, farPoint, 0);
}

/**
 * Adds the player wake to a wave patch (unused).
 * @param pPatch wave patch
 */
void OceanWaterIndirect::waveWakeUpdateWavePatch(WavePatch* pPatch) {
    mOcean.updateWakePatch(pPatch);
}

/**
 * Computes the normals of a wave patch (unused).
 * @param pPatch wave patch
 */
void OceanWaterIndirect::wavePatchCalcNormal_new(WavePatch* pPatch) {}

/**
 * Gets the blend rate between the normal and the disaster look, starting a new blend when
 * the graphics area changes.
 * @return The blend rate.
 */
f32 OceanWaterIndirect::calculateLerp() const {
    al::GraphicsAreaDirector* areaDirector =
        getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector();
    al::CurrentGraphicsAreaParam param;
    areaDirector->getCurrentGraphicsAreaParam(&param,
                                              al::GraphicsAreaParamType::CubeMapCapturePoint);

    if (param.mIsNoParam) {
        mIsLerping = true;
        u32 index = mFromModeIndex;
        mFromModeIndex = mToModeIndex;
        mToModeIndex = index;
    } else if (!mIsLerping) {
        return 0.0f;
    }

    if (param.mRate < 0.5f) {
        if (param.mRate + 1.0f / param._14 >= 0.5f) {
            mIsLerping = false;
            return 0.0f;
        }

        return (0.5f - param.mRate) * 2.0f;
    }

    return (param.mRate - 0.5f) * 2.0f;
}

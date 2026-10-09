#include "Library/Shadow/ShadowMaskDrawer.hpp"

#include <arm_neon.h>
#include <attributes.h>
#include <common/aglDrawContext.h>
#include <common/aglShaderLocation.h>
#include <common/aglShaderProgram.h>
#include <common/aglTextureData.h>
#include <common/aglUniformBlock.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <mc/seadCoreInfo.h>
#include <mc/seadJob.h>
#include <mc/seadJobQueue.h>
#include <mc/seadWorkerMgr.h>
#include <postfx/aglPostFxUtil.h>
#include <utility/aglPrimitiveShape.h>

#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Memory/Util.hpp"
#include "Library/Model/SimpleModelEnv.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/GBufferArray.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Shadow/ShadowDirector.hpp"
#include "Library/Shadow/ShadowKeeper.hpp"

namespace al {

/**
 * Settings shared by the shadow mask keepers.
 */
struct ShadowMaskSetting {
    ShadowMaskSetting()
        : mDebugPos(50.0f, 360.0f), mDebugSize(840.0f, 60.0f), mDebugSubSize(460.0f, 60.0f) {
        mCoreMask = sead::CoreIdMask(sead::CoreId::cMain, sead::CoreId::cSub2);
    }

    sead::CoreIdMask mCoreMask;
    sead::Vector2f mDebugPos;
    sead::Vector2f mDebugSize;
    sead::Vector2f mDebugSubSize;
};

HIDDEN ShadowMaskSetting gShadowMaskSetting;

}  // namespace al

namespace {

typedef sead::Job1<al::ShadowMaskKeeper, s32> ShadowMaskJob;


const agl::ShaderMode cDrawShaderMode = static_cast<agl::ShaderMode>(4);

/**
 * Multiplies a 3x4 matrix (extended with a (0, 0, 0, 1) row) by a 4x4 matrix.
 * @param rOut Result.
 * @param rA Left matrix.
 * @param rB Right matrix.
 */
inline void multiplyMtx34x44(sead::Matrix44f& rOut, const sead::Matrix34f& rA,
                             const sead::Matrix44f& rB) {
    const float32x4_t a0 = vld1q_f32(rA.m[0]);
    const float32x4_t a1 = vld1q_f32(rA.m[1]);
    const float32x4_t a2 = vld1q_f32(rA.m[2]);

    const float32x4_t b0 = vld1q_f32(rB.m[0]);
    const float32x4_t b1 = vld1q_f32(rB.m[1]);
    const float32x4_t b2 = vld1q_f32(rB.m[2]);
    const float32x4_t b3 = vld1q_f32(rB.m[3]);

    float32x4_t c0 = vmulq_laneq_f32(b0, a0, 0);
    c0 = vfmaq_laneq_f32(c0, b1, a0, 1);
    c0 = vfmaq_laneq_f32(c0, b2, a0, 2);
    c0 = vfmaq_laneq_f32(c0, b3, a0, 3);

    float32x4_t c1 = vmulq_laneq_f32(b0, a1, 0);
    c1 = vfmaq_laneq_f32(c1, b1, a1, 1);
    c1 = vfmaq_laneq_f32(c1, b2, a1, 2);
    c1 = vfmaq_laneq_f32(c1, b3, a1, 3);

    float32x4_t c2 = vmulq_laneq_f32(b0, a2, 0);
    c2 = vfmaq_laneq_f32(c2, b1, a2, 1);
    c2 = vfmaq_laneq_f32(c2, b2, a2, 2);
    c2 = vfmaq_laneq_f32(c2, b3, a2, 3);

    vst1q_f32(rOut.m[0], c0);
    vst1q_f32(rOut.m[1], c1);
    vst1q_f32(rOut.m[2], c2);
    vst1q_f32(rOut.m[3], b3);
}

const al::UniformBlockLayout cSphereLayout[] = {
    {0, agl::UniformBlock::cType_Vec4, 4}, {1, agl::UniformBlock::cType_Vec3, 1},
    {2, agl::UniformBlock::cType_Float, 1}, {3, agl::UniformBlock::cType_Vec4, 1},
    {4, agl::UniformBlock::cType_Float, 1}, {5, agl::UniformBlock::cType_Float, 1},
};

const al::UniformBlockLayout cCylinderLayout[] = {
    {0, agl::UniformBlock::cType_Vec4, 4}, {1, agl::UniformBlock::cType_Vec4, 4},
    {2, agl::UniformBlock::cType_Vec4, 1}, {3, agl::UniformBlock::cType_Float, 1},
    {4, agl::UniformBlock::cType_Float, 1}, {5, agl::UniformBlock::cType_Float, 1},
    {6, agl::UniformBlock::cType_Float, 1},
};

const al::UniformBlockLayout cCubeLayout[] = {
    {0, agl::UniformBlock::cType_Vec4, 4},  {1, agl::UniformBlock::cType_Vec4, 4},
    {2, agl::UniformBlock::cType_Vec4, 1},  {3, agl::UniformBlock::cType_Float, 1},
    {4, agl::UniformBlock::cType_Float, 1}, {5, agl::UniformBlock::cType_Float, 1},
    {6, agl::UniformBlock::cType_Float, 1}, {7, agl::UniformBlock::cType_Float, 1},
    {8, agl::UniformBlock::cType_Vec2, 1},  {9, agl::UniformBlock::cType_Vec2, 1},
    {10, agl::UniformBlock::cType_Vec2, 1}, {11, agl::UniformBlock::cType_Int, 1},
};

const al::UniformBlockLayout cCommonLayout[] = {
    {0, agl::UniformBlock::cType_Float, 1},
    {1, agl::UniformBlock::cType_Vec4, 1},
};

/**
 * Shader macro names or values of a primitive shader variation.
 */
struct PrimMacroTable {
    const char* mStrings[4];
};

const PrimMacroTable cPrimMacros = {
    {"SM_PRIM_TYPE", "SM_EDGE_TYPE", "RENDER_TYPE", "SM_DUMP_TYPE"}};
const PrimMacroTable cSphereValues = {{"1", "0", "0", "1"}};
const PrimMacroTable cCylinderValues = {{"2", "0", "0", "1"}};
const PrimMacroTable cCubeValues = {{"3", "0", "0", "1"}};
const PrimMacroTable cTexCubeValues = {{"4", "0", "0", "0"}};

const char* const cDrawerNames[] = {
    "シャドウマスク[ブロック]",
    "シャドウマスク[アイテム]",
    "シャドウマスク[地形オブジェ]",
    "シャドウマスク[敵]",
    "シャドウマスク[プレイヤー]",
    "シャドウマスク[モデルライト後]",
    "シャドウマスク[地形オブジェ]",
    "シャドウマスク[敵]",
    "シャドウマスク[プレイヤー]",
    "シャドウマスク[モデルライト後]",
    "シャドウマスク[モデルライト後]",
    "シャドウマスク[地形オブジェ]",
    "シャドウマスク[地形と水オブジェ]",
    "シャドウマスク[敵]",
    "シャドウマスク[プレイヤー]",
    "シャドウマスク[モデルライト後]",
    "シャドウマスク[プレイヤー]",
};

}  // namespace

namespace al {

/**
 * Initializes the shadow intensity parameters.
 */
void ShadowMaskParam::init() {
    mBlockIntensity.init(180, "BlockIntensity", "ブロックのシャドウ強度", "Min=0, Max=255", this);
    mItemIntensity.init(200, "ItemIntensity", "アイテムのシャドウ強度", "Min=0, Max=255", this);
    mMapObjIntensity.init(180, "MapObjIntensity", "地形オブジェのシャドウ強度", "Min=0, Max=255",
                          this);
    mEnemyIntensity.init(200, "EnemyIntensity", "敵のシャドウ強度", "Min=0, Max=255", this);
    mPlayerIntensity.init(200, "PlayerIntensity", "プレイヤーのシャドウ強度", "Min=0, Max=255",
                          this);
}

/**
 * Compares the shadow intensity parameters.
 * @param rOther Parameters to compare with.
 * @return Whether all parameters are equal.
 */
bool ShadowMaskParam::operator==(const ShadowMaskParam& rOther) const {
    return *mBlockIntensity == *rOther.mBlockIntensity &&
           *mItemIntensity == *rOther.mItemIntensity &&
           *mMapObjIntensity == *rOther.mMapObjIntensity &&
           *mEnemyIntensity == *rOther.mEnemyIntensity &&
           *mPlayerIntensity == *rOther.mPlayerIntensity;
}

/**
 * Copies the shadow intensity parameters.
 * @param rOther Parameters to copy.
 * @return This.
 */
ShadowMaskParam& ShadowMaskParam::operator=(const ShadowMaskParam& rOther) {
    *mBlockIntensity = *rOther.mBlockIntensity;
    *mItemIntensity = *rOther.mItemIntensity;
    *mMapObjIntensity = *rOther.mMapObjIntensity;
    *mEnemyIntensity = *rOther.mEnemyIntensity;
    *mPlayerIntensity = *rOther.mPlayerIntensity;
    return *this;
}

/**
 * Interpolates the shadow intensity parameters.
 * @param rParamA Start parameters.
 * @param rParamB End parameters.
 * @param rate Interpolation rate.
 */
void ShadowMaskParam::interp(const ShadowMaskParam& rParamA, const ShadowMaskParam& rParamB,
                             f32 rate) {
    *mBlockIntensity = lerpValueNew(*rParamA.mBlockIntensity, *rParamB.mBlockIntensity, rate);
    *mItemIntensity = lerpValueNew(*rParamA.mItemIntensity, *rParamB.mItemIntensity, rate);
    *mMapObjIntensity = lerpValueNew(*rParamA.mMapObjIntensity, *rParamB.mMapObjIntensity, rate);
    *mEnemyIntensity = lerpValueNew(*rParamA.mEnemyIntensity, *rParamB.mEnemyIntensity, rate);
    *mPlayerIntensity = lerpValueNew(*rParamA.mPlayerIntensity, *rParamB.mPlayerIntensity, rate);
}

/**
 * Creates the keeper and the worker threads updating the shadow masks.
 * @param bufferNum Number of GPU buffers.
 * @param pInfo Graphics system info.
 */
ShadowMaskKeeper::ShadowMaskKeeper(u8 bufferNum, GraphicsSystemInfo* pInfo)
    : GraphicsParamRequestInterpKeeper<ShadowMaskParam>(pInfo, 17, "ShadowMask", "aglsdw_mask",
                                                        nullptr),
      mSystemInfo(pInfo), mBufferNum(bufferNum) {
    mEvenTargetMasks.allocBuffer(5116, nullptr);
    mMasks.allocBuffer(5116, nullptr);

    for (s32 i = 0; i < 5; i++) {
        for (s32 j = 0; j < cCategoryNum; j++) {
            mDeclareCount[i][j] = 0;
        }
    }

    sead::WorkerMgr::InitializeArg arg;
    mWorkerMgr = new sead::WorkerMgr();
    mWorkerMgr->initialize(arg);

    mUpdateJobQueue = new sead::FixedSizeJQ();
    mUpdateJobQueue->initialize(2, nullptr);
    mUpdateJobQueue->setGranularity(1);

    mGpuBufferJobQueue = new sead::FixedSizeJQ();
    mGpuBufferJobQueue->initialize(2, nullptr);
    mGpuBufferJobQueue->setGranularity(1);

    for (s32 i = 0; i < 2; i++) {
        mUpdateJobQueue->enque(new ShadowMaskJob(this, &ShadowMaskKeeper::updateMultiCore, i));
        mGpuBufferJobQueue->enque(
            new ShadowMaskJob(this, &ShadowMaskKeeper::updateGpuBufferMultiCore, i));
    }
}

/**
 * Updates the shadow masks assigned to a core.
 * @param coreIndex Index of the core.
 */
void ShadowMaskKeeper::updateMultiCore(s32 coreIndex) {
    s32 maskNum = mMaskLastIndex + 1;
    s32 start = maskNum * coreIndex / 2;
    s32 end = maskNum * (coreIndex + 1) / 2;

    for (s32 i = start; i < end; i++) {
        mMasks.unsafeAt(i)->updateMulti();
    }
}

/**
 * Writes the uniform blocks of the primitives assigned to a core.
 * @param coreIndex Index of the core.
 */
void ShadowMaskKeeper::updateGpuBufferMultiCore(s32 coreIndex) {
    for (s32 i = 0; i < cCategoryNum; i++) {
        s32 start = mSpherePrimInfos[i].size() * coreIndex / 2;
        s32 end = mSpherePrimInfos[i].size() * (coreIndex + 1) / 2;
        UniformBlockArray& sphereBlocks = mSphereBlocks[i][mUpdateBufferIndex];

        for (s32 j = start; j < end; j++) {
            const PrimInfo* info = mSpherePrimInfos[i].at(j);
            setUniformSphere(sphereBlocks.at(j), *info, *mViewMtx, *mProjMtx);
        }

        start = mCylinderPrimInfos[i].size() * coreIndex / 2;
        end = mCylinderPrimInfos[i].size() * (coreIndex + 1) / 2;
        UniformBlockArray& cylinderBlocks = mCylinderBlocks[i][mUpdateBufferIndex];

        for (s32 j = start; j < end; j++) {
            const PrimInfo* info = mCylinderPrimInfos[i].at(j);
            setUniformCylinder(cylinderBlocks.at(j), *info, *mViewMtx, *mProjMtx);
        }

        start = mCubePrimInfos[i].size() * coreIndex / 2;
        end = mCubePrimInfos[i].size() * (coreIndex + 1) / 2;
        UniformBlockArray& cubeBlocks = mCubeBlocks[i][mUpdateBufferIndex];

        for (s32 j = start; j < end; j++) {
            const PrimInfo* info = mCubePrimInfos[i].at(j);
            setUniformCube(cubeBlocks.at(j), *info, *mViewMtx, *mProjMtx);
        }
    }
}

/**
 * Deletes the worker threads, the uniform blocks and the drawers.
 */
ShadowMaskKeeper::~ShadowMaskKeeper() {
    mGpuBufferJobQueue->finalize();

    if (mGpuBufferJobQueue != nullptr) {
        delete mGpuBufferJobQueue;
        mGpuBufferJobQueue = nullptr;
    }

    mUpdateJobQueue->finalize();

    if (mUpdateJobQueue != nullptr) {
        delete mUpdateJobQueue;
        mUpdateJobQueue = nullptr;
    }

    mWorkerMgr->finalize();

    if (mWorkerMgr != nullptr) {
        delete mWorkerMgr;
        mWorkerMgr = nullptr;
    }

    for (Context& context : mContexts) {
        if (context.mUniformBlock != nullptr) {
            delete context.mUniformBlock;
            context.mUniformBlock = nullptr;
        }
    }

    mContexts.freeBuffer();

    for (UniformBlockBuffer& blocks : mCubeBlocks) {
        for (UniformBlockArray& array : blocks) {
            while (!array.isEmpty()) {
                delete array.popBack();
            }

            array.freeBuffer();
        }

        blocks.freeBuffer();
    }

    for (UniformBlockBuffer& blocks : mCylinderBlocks) {
        for (UniformBlockArray& array : blocks) {
            while (!array.isEmpty()) {
                delete array.popBack();
            }

            array.freeBuffer();
        }

        blocks.freeBuffer();
    }

    for (UniformBlockBuffer& blocks : mSphereBlocks) {
        for (UniformBlockArray& array : blocks) {
            while (!array.isEmpty()) {
                delete array.popBack();
            }

            array.freeBuffer();
        }

        blocks.freeBuffer();
    }

    for (PrimInfoArray& infos : mCubePrimInfos) {
        infos.freeBuffer();
    }

    for (PrimInfoArray& infos : mCylinderPrimInfos) {
        infos.freeBuffer();
    }

    for (PrimInfoArray& infos : mSpherePrimInfos) {
        infos.freeBuffer();
    }

    mMasks.freeBuffer();
    mEvenTargetMasks.freeBuffer();

    while (!mDrawers.isEmpty()) {
        delete mDrawers.popBack();
    }

    mDrawers.freeBuffer();
}

/**
 * Gets the shadow intensity of a draw category.
 * @param category Draw category.
 * @return Shadow intensity.
 */
u8 ShadowMaskKeeper::getShadowIntensity(s32 category) const {
    const ShadowMaskParam& param = getCurrentParam();
    s32 intensity = 0;

    switch (category) {
    case ShadowMaskDrawCategory::Block:
        intensity = param.getBlockIntensity();
        break;
    case ShadowMaskDrawCategory::Item:
        intensity = param.getItemIntensity();
        break;
    case ShadowMaskDrawCategory::MapObj:
        intensity = param.getMapObjIntensity();
        break;
    case ShadowMaskDrawCategory::Enemy:
        intensity = param.getEnemyIntensity();
        break;
    case ShadowMaskDrawCategory::Player:
    case ShadowMaskDrawCategory::PlayerDecoration:
        intensity = param.getPlayerIntensity();
        break;
    default:
        break;
    }

    return intensity;
}

/**
 * Allocates the primitive buffers and uniform blocks for the declared shadow masks.
 */
void ShadowMaskKeeper::endInit() {
    for (s32 i = 0; i < cCategoryNum; i++) {
        s32 sphereNum = mDeclareCount[ShadowMaskType::Sphere - 1][i];
        mSpherePrimInfos[i].allocBuffer(sead::Mathi::max(sphereNum, 1), nullptr);

        s32 cylinderNum = mDeclareCount[ShadowMaskType::Cylinder - 1][i] +
                          mDeclareCount[ShadowMaskType::CastOvalCylinder - 1][i];
        mCylinderPrimInfos[i].allocBuffer(sead::Mathi::max(cylinderNum, 1), nullptr);

        s32 cubeNum = mDeclareCount[ShadowMaskType::Cube - 1][i] +
                      mDeclareCount[ShadowMaskType::CastInterpolateCube - 1][i];
        mCubePrimInfos[i].allocBuffer(sead::Mathi::max(cubeNum, 1), nullptr);

        mSphereBlocks[i].tryAllocBuffer(mBufferNum, nullptr);
        mCylinderBlocks[i].tryAllocBuffer(mBufferNum, nullptr);
        mCubeBlocks[i].tryAllocBuffer(mBufferNum, nullptr);
    }

    for (s32 i = 0; i < cCategoryNum; i++) {
        for (s32 j = 0; j < mBufferNum; j++) {
            mSphereBlocks[i][j].allocBuffer(mSpherePrimInfos[i].capacity(), nullptr);
            UniformBlockArray& sphereBlocks = mSphereBlocks[i][j];

            for (s32 k = 0; k < sphereBlocks.capacity(); k++) {
                sphereBlocks.pushBack(createUniformBlock(cSphereLayout, 6, getCurrentHeap(), 2));
            }

            mCylinderBlocks[i][j].allocBuffer(mCylinderPrimInfos[i].capacity(), nullptr);
            UniformBlockArray& cylinderBlocks = mCylinderBlocks[i][j];

            for (s32 k = 0; k < cylinderBlocks.capacity(); k++) {
                cylinderBlocks.pushBack(
                    createUniformBlock(cCylinderLayout, 7, getCurrentHeap(), 2));
            }

            mCubeBlocks[i][j].allocBuffer(mCubePrimInfos[i].capacity(), nullptr);
            UniformBlockArray& cubeBlocks = mCubeBlocks[i][j];

            for (s32 k = 0; k < cubeBlocks.capacity(); k++) {
                cubeBlocks.pushBack(createUniformBlock(cCubeLayout, 12, getCurrentHeap(), 2));
            }
        }
    }

    s32 primNum = 0;

    for (s32 i = 0; i < cCategoryNum; i++) {
        for (s32 j = 0; j < 5; j++) {
            primNum += mDeclareCount[j][i];
        }
    }

    initialize(primNum + 4, mBufferNum);
    GraphicsParamRequestInterpKeeper<ShadowMaskParam>::endInit();
}

/**
 * Clears the primitives and creates the draw contexts.
 * @param primNum Maximum number of primitives.
 * @param bufferNum Number of GPU buffers.
 */
void ShadowMaskKeeper::initialize(u32 primNum, u8 bufferNum) {
    for (s32 i = 0; i < cCategoryNum; i++) {
        mSpherePrimInfos[i].clear();
        mCylinderPrimInfos[i].clear();
        mCubePrimInfos[i].clear();
    }

    clearRequest();
    mPrimNum = primNum;

    mSphereAttribute.create(1, getCurrentHeap());
    mSphereAttribute.setVertexStream(
        0, &agl::utl::PrimitiveShape::instance()->getSphereVertexBuffer(), 0);
    mSphereAttribute.setUp();

    mCylinderAttribute.create(1, getCurrentHeap());
    mCylinderAttribute.setVertexStream(
        0, &agl::utl::PrimitiveShape::instance()->getCylinderVertexBuffer(), 0);
    mCylinderAttribute.setUp();

    mCubeAttribute.create(1, getCurrentHeap());
    mCubeAttribute.setVertexStream(
        0, &agl::utl::PrimitiveShape::instance()->getCubeVertexBuffer(), 0);
    mCubeAttribute.setUp();

    mContexts.tryAllocBuffer(bufferNum, getCurrentHeap());

    for (Context& context : mContexts) {
        context.mRenderBuffer.setRenderTargetColor(&context.mRenderTarget);
        context.mTexture = nullptr;
        context.mUniformBlock = createUniformBlock(cCommonLayout, 2, getCurrentHeap(), 2);
    }
}

/**
 * Removes all primitives and clears the parameter requests.
 */
void ShadowMaskKeeper::clear() {
    for (s32 i = 0; i < cCategoryNum; i++) {
        mSpherePrimInfos[i].clear();
        mCylinderPrimInfos[i].clear();
        mCubePrimInfos[i].clear();
    }

    clearRequest();
}

/**
 * Updates the parameter requests and all registered shadow masks.
 */
void ShadowMaskKeeper::updateShadowMask() {
    updateRequest();

    if (mMaskLastIndex > 0) {
        for (s32 i = 0; i <= mEvenTargetMaskLastIndex; i++) {
            mEvenTargetMasks.unsafeAt(i)->update();
        }

        mUpdateJobQueue->rewind();
        mWorkerMgr->pushJobQueue(mUpdateJobQueue, gShadowMaskSetting.mCoreMask,
                                 sead::SyncType::cCore, sead::JobQueuePushType::cForward);
        mWorkerMgr->run();
        mWorkerMgr->sync();

        for (s32 i = 0; i <= mMaskLastIndex; i++) {
            mMasks.unsafeAt(i)->addMulti();
        }
    } else {
        for (s32 i = 0; i <= mEvenTargetMaskLastIndex; i++) {
            mEvenTargetMasks.unsafeAt(i)->update();
        }

        for (s32 i = 0; i <= mMaskLastIndex; i++) {
            mMasks.unsafeAt(i)->update();
        }
    }
}

/**
 * Writes the uniform block of a sphere primitive.
 * @param pBlock Uniform block.
 * @param rInfo Primitive.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 */
void ShadowMaskKeeper::setUniformSphere(agl::UniformBlock* pBlock, const PrimInfo& rInfo,
                                        const sead::Matrix34f& rViewMtx,
                                        const sead::Matrix44f& rProjMtx) {
    sead::Vector3f scale;
    calcMtxScale(&scale, rInfo.mMtx);

    sead::Matrix44f modelView;
    multiplyMtx34x44(modelView, rViewMtx, sead::Matrix44f(rInfo.mMtx));
    sead::Matrix44f modelViewProj;
    agl::pfx::detail::multiplyMtx44(modelViewProj, rProjMtx, modelView);

    UniformBlock* block = static_cast<UniformBlock*>(pBlock);
    pBlock->setCurrentBufferIndex(mGpuBufferIndex);
    pBlock->setData(0, &modelViewProj, 0, 4);

    sead::Vector3f trans;
    rInfo.mMtx.getTranslation(trans);
    trans.setMul(rViewMtx, trans);
    pBlock->setData(1, &trans, 0, 1);

    block->setValue(2, scale.x > 0.0f ? 1.0f / (scale.x * 0.5f) : 1000000.0f);
    pBlock->setData(3, &rInfo.mColor, 0, 1);
    block->setValue(4, rInfo.mExp.x);
    block->setValue(5, rInfo.mIntensity);
}

/**
 * Writes the uniform block of a cylinder primitive.
 * @param pBlock Uniform block.
 * @param rInfo Primitive.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 */
void ShadowMaskKeeper::setUniformCylinder(agl::UniformBlock* pBlock, const PrimInfo& rInfo,
                                          const sead::Matrix34f& rViewMtx,
                                          const sead::Matrix44f& rProjMtx) {
    sead::Matrix44f modelView;
    multiplyMtx34x44(modelView, rViewMtx, sead::Matrix44f(rInfo.mMtx));
    sead::Matrix44f modelViewProj;
    agl::pfx::detail::multiplyMtx44(modelViewProj, rProjMtx, modelView);

    UniformBlock* block = static_cast<UniformBlock*>(pBlock);
    pBlock->setCurrentBufferIndex(mGpuBufferIndex);
    pBlock->setData(0, &modelViewProj, 0, 4);

    sead::Matrix44f invModelView;
    invModelView.setInverse(modelView);
    pBlock->setData(1, &invModelView, 0, 4);
    pBlock->setData(2, &rInfo.mColor, 0, 1);

    block->setValue(3, rInfo.mExp.x);
    block->setValue(4, rInfo.mExp.y);
    block->setValue(5, rInfo.mDistYBase);
    block->setValue(6, rInfo.mIntensity);
}

/**
 * Writes the uniform block of a cube primitive.
 * @param pBlock Uniform block.
 * @param rInfo Primitive.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 */
void ShadowMaskKeeper::setUniformCube(agl::UniformBlock* pBlock, const PrimInfo& rInfo,
                                      const sead::Matrix34f& rViewMtx,
                                      const sead::Matrix44f& rProjMtx) {
    sead::Matrix44f modelView;
    multiplyMtx34x44(modelView, rViewMtx, sead::Matrix44f(rInfo.mMtx));
    sead::Matrix44f modelViewProj;
    agl::pfx::detail::multiplyMtx44(modelViewProj, rProjMtx, modelView);

    UniformBlock* block = static_cast<UniformBlock*>(pBlock);
    pBlock->setCurrentBufferIndex(mGpuBufferIndex);
    pBlock->setData(0, &modelViewProj, 0, 4);

    sead::Matrix44f invModelView;
    invModelView.setInverse(modelView);
    pBlock->setData(1, &invModelView, 0, 4);
    pBlock->setData(2, &rInfo.mColor, 0, 1);

    block->setValue(3, rInfo.mExp.x);
    block->setValue(4, rInfo.mExp.y);
    block->setValue(5, rInfo.mExp.z);
    block->setValue(6, rInfo.mDistYBase);
    block->setValue(7, rInfo.mIntensity);
    pBlock->setData(8, &rInfo.mTexOffset, 0, 1);
    pBlock->setData(9, &rInfo.mTexScale, 0, 1);
    pBlock->setData(10, &rInfo.mTexRange, 0, 1);
    block->setValue(11, rInfo.mIsTexFlip);
}

/**
 * Registers a shadow mask to be updated every frame.
 * @param pMask Shadow mask.
 */
void ShadowMaskKeeper::registerShadowMask(ShadowMaskBase* pMask) {
    if (pMask->mIsRegistered) {
        return;
    }

    if (pMask->_e8) {
        if (mEvenTargetMaskLastIndex < mEvenTargetMasks.size() - 1) {
            mEvenTargetMaskLastIndex++;
            mEvenTargetMasks.replace(mEvenTargetMaskLastIndex, pMask);
        } else {
            mEvenTargetMasks.pushBack(pMask);
            mEvenTargetMaskLastIndex = mEvenTargetMasks.size() - 1;
        }
    } else {
        if (mMaskLastIndex < mMasks.size() - 1) {
            mMaskLastIndex++;
            mMasks.replace(mMaskLastIndex, pMask);
        } else {
            mMasks.pushBack(pMask);
            mMaskLastIndex = mMasks.size() - 1;
        }
    }

    pMask->mIsRegistered = true;
}

/**
 * Removes a registered shadow mask.
 * @param pMask Shadow mask.
 */
void ShadowMaskKeeper::removeShadowMask(ShadowMaskBase* pMask) {
    if (pMask->_e8) {
        for (s32 i = 0; i <= mEvenTargetMaskLastIndex; i++) {
            if (mEvenTargetMasks[i] == pMask) {
                ShadowMaskBase* last = mEvenTargetMasks[mEvenTargetMaskLastIndex];
                mEvenTargetMaskLastIndex--;
                mEvenTargetMasks.replace(i, last);
                pMask->mIsRegistered = false;
                return;
            }
        }
    } else {
        for (s32 i = 0; i <= mMaskLastIndex; i++) {
            if (mMasks[i] == pMask) {
                ShadowMaskBase* last = mMasks[mMaskLastIndex];
                mMaskLastIndex--;
                mMasks.replace(i, last);
                pMask->mIsRegistered = false;
                return;
            }
        }
    }
}

/**
 * Creates a drawer for every draw category.
 * @param pDirector Execute director.
 */
void ShadowMaskKeeper::init(ExecuteDirector* pDirector) {
    mDrawers.allocBuffer(cCategoryNum, nullptr);

    for (s32 i = 0; i < cCategoryNum; i++) {
        mDrawers.pushBack(new ShadowMaskDrawer(this, i, pDirector));
    }
}

/**
 * Sets the shadow mask shader to all drawers.
 * @param pShaderHolder Shader holder.
 */
void ShadowMaskKeeper::initShader(ShaderHolder* pShaderHolder) {
    const agl::ShaderProgram* program = pShaderHolder->getShaderProgram("ShadowMask");

    if (program == nullptr) {
        return;
    }

    for (s32 i = 0; i < cCategoryNum; i++) {
        mDrawers.unsafeAt(i)->mShaderProgram = program;
    }
}

/**
 * Adds a sphere primitive.
 * @param rMtx Primitive matrix.
 * @param rColor Shadow color.
 * @param exp Falloff exponent.
 * @param intensity Shadow intensity.
 * @param category Draw category.
 */
void ShadowMaskKeeper::addSphere(const sead::Matrix34f& rMtx, const sead::Color4f& rColor, f32 exp,
                                 f32 intensity, s32 category) {
    PrimInfoArray& infos = mSpherePrimInfos[category];

    if (infos.isFull()) {
        return;
    }

    PrimInfo* info = infos.birthBack();
    info->mType = cPrimType_Sphere;
    info->mMtx = rMtx;
    info->mColor = rColor;
    info->mExp.set(exp, exp, exp);
    info->_74 = 0.0f;
    info->mDistYBase = 0.0f;
    info->_7c = 0;
    info->mCategory = category;
    info->mIntensity = intensity;
}

/**
 * Adds a cylinder primitive.
 * @param rMtx Primitive matrix.
 * @param rColor Shadow color.
 * @param expXZ Horizontal falloff exponent.
 * @param expY Vertical falloff exponent.
 * @param intensity Shadow intensity.
 * @param distYBase Vertical base distance.
 * @param category Draw category.
 */
void ShadowMaskKeeper::addCylinder(const sead::Matrix34f& rMtx, const sead::Color4f& rColor,
                                   f32 expXZ, f32 expY, f32 intensity, f32 distYBase,
                                   s32 category) {
    PrimInfoArray& infos = mCylinderPrimInfos[category];

    if (infos.isFull()) {
        return;
    }

    PrimInfo* info = infos.birthBack();
    info->mType = cPrimType_Cylinder;
    info->mMtx = rMtx;
    info->mColor = rColor;
    info->mExp.set(expXZ, expY, expXZ);
    info->_74 = 0.0f;
    info->mDistYBase = distYBase;
    info->_7c = 0;
    info->mCategory = category;
    info->mIntensity = intensity;
}

/**
 * Adds an oval cylinder primitive cast along its direction.
 * @param rMtx Primitive matrix.
 * @param rColor Shadow color.
 * @param expXZ Horizontal falloff exponent.
 * @param expY Vertical falloff exponent.
 * @param intensity Shadow intensity.
 * @param distYBase Vertical base distance.
 * @param category Draw category.
 */
void ShadowMaskKeeper::addCastOvalCylinder(const sead::Matrix34f& rMtx,
                                           const sead::Color4f& rColor, f32 expXZ, f32 expY,
                                           f32 intensity, f32 distYBase, s32 category) {
    PrimInfoArray& infos = mCylinderPrimInfos[category];

    if (infos.isFull()) {
        return;
    }

    PrimInfo* info = infos.birthBack();
    info->mType = cPrimType_OvalCylinder;
    info->mMtx = rMtx;
    info->mColor = rColor;
    info->mExp.set(expXZ, expY, expXZ);
    info->_74 = 0.0f;
    info->mDistYBase = distYBase;
    info->_7c = 0;
    info->mCategory = category;
    info->mIntensity = intensity;
}

/**
 * Adds a cube primitive.
 * @param rMtx Primitive matrix.
 * @param rColor Shadow color.
 * @param rExp Falloff exponents.
 * @param intensity Shadow intensity.
 * @param distYBase Vertical base distance.
 * @param category Draw category.
 * @param pSampler Projected texture sampler (optional).
 * @param isCube Whether the cube is drawn as a plain cube.
 * @param rTexOffset Projected texture offset.
 * @param rTexScale Projected texture scale.
 * @param rTexRange Projected texture range.
 * @param texType Projected texture type.
 * @param isTexFlip Whether the projected texture is flipped.
 */
void ShadowMaskKeeper::addCube(const sead::Matrix34f& rMtx, const sead::Color4f& rColor,
                               const sead::Vector3f& rExp, f32 intensity, f32 distYBase,
                               s32 category, agl::TextureSampler* pSampler, bool isCube,
                               const sead::Vector2f& rTexOffset, const sead::Vector2f& rTexScale,
                               const sead::Vector2f& rTexRange, s32 texType, bool isTexFlip) {
    PrimInfoArray& infos = mCubePrimInfos[category];

    if (infos.isFull()) {
        return;
    }

    PrimInfo* info = infos.birthBack();
    info->mType = isCube ? cPrimType_Cube : cPrimType_InterpolateCube;
    info->mMtx = rMtx;
    info->mColor = rColor;
    info->mExp = rExp;
    info->_74 = 0.0f;
    info->mDistYBase = distYBase;
    info->_7c = 0;
    info->mCategory = category;
    info->mIntensity = intensity;
    info->mSampler = pSampler;
    info->mTexOffset = rTexOffset;
    info->mTexScale = rTexScale;
    info->mTexRange = rTexRange;
    info->mTexType = texType;
    info->mIsTexFlip = isTexFlip;
}

/**
 * Adds a cube primitive cast with interpolation.
 * @param rMtx Primitive matrix.
 * @param rColor Shadow color.
 * @param rExp Falloff exponents.
 * @param intensity Shadow intensity.
 * @param distYBase Vertical base distance.
 * @param category Draw category.
 */
void ShadowMaskKeeper::addCastInterpolateCube(const sead::Matrix34f& rMtx,
                                              const sead::Color4f& rColor,
                                              const sead::Vector3f& rExp, f32 intensity,
                                              f32 distYBase, s32 category) {
    PrimInfoArray& infos = mCubePrimInfos[category];

    if (infos.isFull()) {
        return;
    }

    PrimInfo* info = infos.birthBack();
    info->mType = cPrimType_InterpolateCube;
    info->mMtx = rMtx;
    info->mColor = rColor;
    info->mExp = rExp;
    info->_74 = 0.0f;
    info->mDistYBase = distYBase;
    info->_7c = 0;
    info->mCategory = category;
    info->mIntensity = intensity;
}

/**
 * Writes the uniform block shared by all primitives of a buffer.
 * @param bufferIndex Index of the GPU buffer.
 * @param width Render target width.
 * @param height Render target height.
 */
void ShadowMaskKeeper::setUniformCommon(s32 bufferIndex, s32 width, s32 height) {
    Context& context = mContexts[bufferIndex];
    sead::Vector4f screen(0.0f, 0.0f, -0.5f / width, -0.5f / height);
    context.mUniformBlock->setCurrentBufferIndex(mGpuBufferIndex);
    context.mUniformBlock->setValue(0, mShadowRate);
    context.mUniformBlock->setData(1, &screen, 0, 1);
}

/**
 * Writes the uniform blocks of all primitives.
 * @param bufferIndex Index of the GPU buffer.
 * @param pViewMtx View matrix.
 * @param pProjMtx Projection matrix.
 */
void ShadowMaskKeeper::updateGpuBuffer(s32 bufferIndex, const sead::Matrix34f* pViewMtx,
                                       const sead::Matrix44f* pProjMtx) {
    if (mMaskLastIndex > 0) {
        mUpdateBufferIndex = bufferIndex;
        mViewMtx = pViewMtx;
        mProjMtx = pProjMtx;
        mGpuBufferJobQueue->rewind();
        mWorkerMgr->pushJobQueue(mGpuBufferJobQueue, gShadowMaskSetting.mCoreMask,
                                 sead::SyncType::cCore, sead::JobQueuePushType::cForward);
        mWorkerMgr->run();
        mWorkerMgr->sync();
    } else {
        for (s32 i = 0; i < cCategoryNum; i++) {
            UniformBlockArray& sphereBlocks = mSphereBlocks[i][bufferIndex];
            s32 sphereNum = mSpherePrimInfos[i].size();

            for (s32 j = 0; j < sphereNum; j++) {
                const PrimInfo* info = mSpherePrimInfos[i].at(j);
                setUniformSphere(sphereBlocks.at(j), *info, *pViewMtx, *pProjMtx);
            }

            UniformBlockArray& cylinderBlocks = mCylinderBlocks[i][bufferIndex];
            s32 cylinderNum = mCylinderPrimInfos[i].size();

            for (s32 j = 0; j < cylinderNum; j++) {
                const PrimInfo* info = mCylinderPrimInfos[i].at(j);
                setUniformCylinder(cylinderBlocks.at(j), *info, *pViewMtx, *pProjMtx);
            }

            UniformBlockArray& cubeBlocks = mCubeBlocks[i][bufferIndex];
            s32 cubeNum = mCubePrimInfos[i].size();

            for (s32 j = 0; j < cubeNum; j++) {
                const PrimInfo* info = mCubePrimInfos[i].at(j);
                setUniformCube(cubeBlocks.at(j), *info, *pViewMtx, *pProjMtx);
            }
        }
    }

    flush(bufferIndex, false);
}

/**
 * Flushes the CPU cache of all uniform blocks of a buffer.
 * @param bufferIndex Index of the GPU buffer.
 * @param isForce Unused.
 */
void ShadowMaskKeeper::flush(s32 bufferIndex, bool isForce) {
    mContexts[bufferIndex].mUniformBlock->flushCurrentBuffer();

    for (s32 i = 0; i < cCategoryNum; i++) {
        UniformBlockArray& sphereBlocks = mSphereBlocks[i][bufferIndex];
        s32 sphereNum = mSpherePrimInfos[i].size();

        for (s32 j = 0; j < sphereNum; j++) {
            sphereBlocks.at(j)->flushCurrentBuffer();
        }

        UniformBlockArray& cylinderBlocks = mCylinderBlocks[i][bufferIndex];
        s32 cylinderNum = mCylinderPrimInfos[i].size();

        for (s32 j = 0; j < cylinderNum; j++) {
            cylinderBlocks.at(j)->flushCurrentBuffer();
        }

        UniformBlockArray& cubeBlocks = mCubeBlocks[i][bufferIndex];
        s32 cubeNum = mCubePrimInfos[i].size();

        for (s32 j = 0; j < cubeNum; j++) {
            cubeBlocks.at(j)->flushCurrentBuffer();
        }
    }
}

/**
 * Swaps the GPU buffer written by the uniform blocks.
 */
void ShadowMaskKeeper::swapGpuBuffer() {
    mGpuBufferIndex = 1 - mGpuBufferIndex;
}

/**
 * Selects the shader variation of a primitive type.
 * @param pKeeper Keeper.
 * @param pProgram Shader program.
 * @param pMacros Macro names.
 * @param pValues Macro values of the primitive type.
 * @param category Draw category.
 * @return Shader variation.
 */
ALWAYS_INLINE static const agl::ShaderProgram* searchPrimProgram(const ShadowMaskKeeper* pKeeper,
                                                                 const agl::ShaderProgram* pProgram,
                                                                 const char** pMacros,
                                                                 const char** pValues,
                                                                 s32 category) {
    switch (pKeeper->getRenderType()) {
    case 0:
        pValues[2] = isShadowMrt(category) ? "1" : "0";
        break;
    case 1:
        pValues[2] = "2";
        break;
    default:
        break;
    }

    return pProgram->searchVariation(4, pMacros, pValues);
}

/**
 * Draws the sphere primitives of a draw category.
 * @param bufferIndex Index of the GPU buffer.
 * @param rContext Draw context of the buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawPrimSphere(s32 bufferIndex, Context& rContext,
                                                 const agl::ShaderProgram* pProgram,
                                                 const agl::TextureData& rDepth,
                                                 const sead::Matrix34f& rViewMtx,
                                                 const sead::Matrix44f& rProjMtx,
                                                 agl::ShaderMode mode, s32 category) {
    s32 num = mSpherePrimInfos[category].size();

    if (num == 0) {
        return mode;
    }

    PrimMacroTable macros = cPrimMacros;
    PrimMacroTable values = cSphereValues;
    const agl::ShaderProgram* program =
        searchPrimProgram(this, pProgram, macros.mStrings, values.mStrings, category);
    agl::DrawContext* drawContext = GameFrameworkNx::getAglDrawContext();
    mSphereAttribute.activate(drawContext);
    program->activate(GameFrameworkNx::getAglDrawContext(), true);

    agl::SamplerLocation depthLocation("cLinearDepth");
    depthLocation.search(*program);
    rContext.mLinearDepthSampler.activate(GameFrameworkNx::getAglDrawContext(), depthLocation,
                                          -1, false);

    agl::UniformBlockLocation commonLocation("Common");
    agl::UniformBlockLocation primLocation("SphereBlock");
    commonLocation.search(*program);
    primLocation.search(*program);
    rContext.mUniformBlock->activate(GameFrameworkNx::getAglDrawContext(), commonLocation);
    mSystemInfo->getSimpleModelEnv()->prepareModelDraw(bufferIndex);
    UniformBlockArray& blocks = mSphereBlocks[category][bufferIndex];
    agl::utl::PrimitiveShape* shape = agl::utl::PrimitiveShape::instance();

    for (s32 i = 0; i < num; i++) {
        blocks.at(i)->activate(GameFrameworkNx::getAglDrawContext(), primLocation);
        agl::pfx::detail::drawIndexStream(GameFrameworkNx::getAglDrawContext(),
                                          shape->getSphereIndexStream(1));
    }

    return mode;
}

/**
 * Draws the cylinder primitives of a draw category.
 * @param bufferIndex Index of the GPU buffer.
 * @param rContext Draw context of the buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawPrimCylinder(s32 bufferIndex, Context& rContext,
                                                   const agl::ShaderProgram* pProgram,
                                                   const agl::TextureData& rDepth,
                                                   const sead::Matrix34f& rViewMtx,
                                                   const sead::Matrix44f& rProjMtx,
                                                   agl::ShaderMode mode, s32 category) {
    s32 num = mCylinderPrimInfos[category].size();

    if (num == 0) {
        return mode;
    }

    PrimMacroTable macros = cPrimMacros;
    PrimMacroTable values = cCylinderValues;
    const agl::ShaderProgram* program =
        searchPrimProgram(this, pProgram, macros.mStrings, values.mStrings, category);
    agl::DrawContext* drawContext = GameFrameworkNx::getAglDrawContext();
    mCylinderAttribute.activate(drawContext);
    program->activate(GameFrameworkNx::getAglDrawContext(), true);

    agl::SamplerLocation depthLocation("cLinearDepth");
    depthLocation.search(*program);
    rContext.mLinearDepthSampler.activate(GameFrameworkNx::getAglDrawContext(), depthLocation,
                                          -1, false);

    agl::UniformBlockLocation commonLocation("Common");
    agl::UniformBlockLocation primLocation("CylinderBlock");
    commonLocation.search(*program);
    primLocation.search(*program);
    rContext.mUniformBlock->activate(GameFrameworkNx::getAglDrawContext(), commonLocation);
    mSystemInfo->getSimpleModelEnv()->prepareModelDraw(bufferIndex);
    UniformBlockArray& blocks = mCylinderBlocks[category][bufferIndex];
    agl::utl::PrimitiveShape* shape = agl::utl::PrimitiveShape::instance();

    for (s32 i = 0; i < num; i++) {
        blocks.at(i)->activate(GameFrameworkNx::getAglDrawContext(), primLocation);
        agl::pfx::detail::drawIndexStream(GameFrameworkNx::getAglDrawContext(),
                                          shape->getCylinderTriangleIndexStream(1));
    }

    return mode;
}

/**
 * Draws the untextured cube primitives of a draw category.
 * @param bufferIndex Index of the GPU buffer.
 * @param rContext Draw context of the buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawPrimCube(s32 bufferIndex, Context& rContext,
                                               const agl::ShaderProgram* pProgram,
                                               const agl::TextureData& rDepth,
                                               const sead::Matrix34f& rViewMtx,
                                               const sead::Matrix44f& rProjMtx,
                                               agl::ShaderMode mode, s32 category) {
    s32 num = mCubePrimInfos[category].size();

    if (num == 0) {
        return mode;
    }

    PrimMacroTable macros = cPrimMacros;
    PrimMacroTable values = cCubeValues;
    const agl::ShaderProgram* program =
        searchPrimProgram(this, pProgram, macros.mStrings, values.mStrings, category);
    agl::DrawContext* drawContext = GameFrameworkNx::getAglDrawContext();
    mCubeAttribute.activate(drawContext);
    program->activate(GameFrameworkNx::getAglDrawContext(), true);

    agl::SamplerLocation depthLocation("cLinearDepth");
    depthLocation.search(*program);
    rContext.mLinearDepthSampler.activate(GameFrameworkNx::getAglDrawContext(), depthLocation,
                                          -1, false);

    agl::UniformBlockLocation commonLocation("Common");
    agl::UniformBlockLocation primLocation("CubeBlock");
    commonLocation.search(*program);
    primLocation.search(*program);
    rContext.mUniformBlock->activate(GameFrameworkNx::getAglDrawContext(), commonLocation);
    mSystemInfo->getSimpleModelEnv()->prepareModelDraw(bufferIndex);
    UniformBlockArray& blocks = mCubeBlocks[category][bufferIndex];
    agl::utl::PrimitiveShape* shape = agl::utl::PrimitiveShape::instance();

    for (s32 i = 0; i < num; i++) {
        const PrimInfo* info = mCubePrimInfos[category].at(i);

        if (info->mSampler != nullptr || info->mType == cPrimType_Cube) {
            continue;
        }

        blocks.at(i)->activate(GameFrameworkNx::getAglDrawContext(), primLocation);
        agl::pfx::detail::drawIndexStream(GameFrameworkNx::getAglDrawContext(),
                                          shape->getCubeIndexStream());
    }

    return mode;
}

/**
 * Draws the textured cube primitives of a draw category.
 * @param bufferIndex Index of the GPU buffer.
 * @param rContext Draw context of the buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @param isCube Whether to draw the plain textured cubes.
 * @param texType Projected texture type to draw.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawPrimTexCube(s32 bufferIndex, Context& rContext,
                                                  const agl::ShaderProgram* pProgram,
                                                  const agl::TextureData& rDepth,
                                                  const sead::Matrix34f& rViewMtx,
                                                  const sead::Matrix44f& rProjMtx,
                                                  agl::ShaderMode mode, s32 category, bool isCube,
                                                  s32 texType) {
    s32 num = mCubePrimInfos[category].size();

    if (num == 0) {
        return mode;
    }

    PrimMacroTable macros = cPrimMacros;
    PrimMacroTable values = cTexCubeValues;

    switch (getRenderType()) {
    case 0:
        values.mStrings[2] = isShadowMrt(category) ? "1" : "0";
        break;
    case 1:
        values.mStrings[2] = "2";
        break;
    default:
        break;
    }

    if (isCube) {
        values.mStrings[0] = "5";

        if (texType == 1) {
            values.mStrings[3] = "1";
        }
    }

    const agl::ShaderProgram* program =
        pProgram->searchVariation(4, macros.mStrings, values.mStrings);
    agl::DrawContext* drawContext = GameFrameworkNx::getAglDrawContext();
    mCubeAttribute.activate(drawContext);
    program->activate(GameFrameworkNx::getAglDrawContext(), true);

    agl::SamplerLocation depthLocation("cLinearDepth");
    depthLocation.search(*program);
    rContext.mLinearDepthSampler.activate(GameFrameworkNx::getAglDrawContext(), depthLocation,
                                          -1, false);

    agl::SamplerLocation texLocation("cProjTex");
    texLocation.search(*program);

    agl::UniformBlockLocation commonLocation("Common");
    agl::UniformBlockLocation primLocation("CubeBlock");
    commonLocation.search(*program);
    primLocation.search(*program);
    rContext.mUniformBlock->activate(GameFrameworkNx::getAglDrawContext(), commonLocation);
    mSystemInfo->getSimpleModelEnv()->prepareModelDraw(bufferIndex);
    UniformBlockArray& blocks = mCubeBlocks[category][bufferIndex];
    agl::utl::PrimitiveShape* shape = agl::utl::PrimitiveShape::instance();

    for (s32 i = 0; i < num; i++) {
        const PrimInfo* info = mCubePrimInfos[category].at(i);

        if (info->mSampler == nullptr) {
            continue;
        }

        if ((info->mType == cPrimType_Cube) != isCube) {
            continue;
        }

        if (isCube && info->mTexType != texType) {
            continue;
        }

        info->mSampler->activate(GameFrameworkNx::getAglDrawContext(), texLocation, -1, false);
        blocks.at(i)->activate(GameFrameworkNx::getAglDrawContext(), primLocation);
        agl::pfx::detail::drawIndexStream(GameFrameworkNx::getAglDrawContext(),
                                          shape->getCubeIndexStream());
    }

    return mode;
}

/**
 * Draws the plain cube primitives of a draw category.
 * @param bufferIndex Index of the GPU buffer.
 * @param rContext Draw context of the buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawPrimTex2Cube(s32 bufferIndex, Context& rContext,
                                                   const agl::ShaderProgram* pProgram,
                                                   const agl::TextureData& rDepth,
                                                   const sead::Matrix34f& rViewMtx,
                                                   const sead::Matrix44f& rProjMtx,
                                                   agl::ShaderMode mode, s32 category) {
    return drawPrimTexCube(bufferIndex, rContext, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                           category, true, 0);
}

/**
 * Draws the plain cube primitives of a draw category with the debug dump enabled.
 * @param bufferIndex Index of the GPU buffer.
 * @param rContext Draw context of the buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawPrimTex2CubeDump(s32 bufferIndex, Context& rContext,
                                                       const agl::ShaderProgram* pProgram,
                                                       const agl::TextureData& rDepth,
                                                       const sead::Matrix34f& rViewMtx,
                                                       const sead::Matrix44f& rProjMtx,
                                                       agl::ShaderMode mode, s32 category) {
    return drawPrimTexCube(bufferIndex, rContext, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                           category, true, 1);
}

/**
 * Sets the view of all drawers.
 * @param viewIndex Index of the view.
 * @param pGBufferArray G-buffers of the view.
 * @param pViewMtx View matrix.
 * @param pProjMtx Projection matrix.
 */
void ShadowMaskKeeper::setViewInfo(s32 viewIndex, GBufferArray* pGBufferArray,
                                   const sead::Matrix34f* pViewMtx,
                                   const sead::Matrix44f* pProjMtx) {
    s32 drawerNum = mDrawers.size();

    for (s32 i = 0; i < drawerNum; i++) {
        ShadowMaskDrawer* drawer = mDrawers.unsafeAt(i);
        drawer->mViewIndex = viewIndex;
        drawer->mGBufferArray = pGBufferArray;
        drawer->mViewMtx = pViewMtx;
        drawer->mProjMtx = pProjMtx;
    }
}

/**
 * Overrides the linear depth texture of all drawers.
 * @param pTexture Linear depth texture.
 */
void ShadowMaskKeeper::overrideLinearDepthTexture(agl::TextureData* pTexture) {
    s32 drawerNum = mDrawers.size();

    for (s32 i = 0; i < drawerNum; i++) {
        mDrawers.unsafeAt(i)->mLinearDepthTexture = pTexture;
    }
}

/**
 * Overrides the color texture of all drawers.
 * @param pTexture Color texture.
 */
void ShadowMaskKeeper::overrideColorTexture(agl::TextureData* pTexture) {
    s32 drawerNum = mDrawers.size();

    for (s32 i = 0; i < drawerNum; i++) {
        mDrawers.unsafeAt(i)->mColorTexture = pTexture;
    }
}

/**
 * Checks whether any primitive is drawn.
 * @return Whether any primitive is drawn.
 */
bool ShadowMaskKeeper::isExistDrawShadowMask() const {
    for (s32 i = 0; i < cCategoryNum; i++) {
        if (isExistPrim(i)) {
            return true;
        }
    }

    return false;
}

/**
 * Checks whether any primitive of a draw category is drawn.
 * @param category Draw category.
 * @return Whether any primitive of the category is drawn.
 */
bool ShadowMaskKeeper::isExistDrawShadowMaskCategory(s32 category) const {
    return isExistPrim(category);
}

/**
 * Draws all primitives of a draw category.
 * @param bufferIndex Index of the GPU buffer.
 * @param pProgram Shader program.
 * @param rDepth Linear depth texture.
 * @param rViewMtx View matrix.
 * @param rProjMtx Projection matrix.
 * @param mode Current shader mode.
 * @param category Draw category.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::draw(s32 bufferIndex, const agl::ShaderProgram* pProgram,
                                       const agl::TextureData& rDepth,
                                       const sead::Matrix34f& rViewMtx,
                                       const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                       s32 category) {
    if (!mIsDrawIntensity && isShadowIntensity(category)) {
        return mode;
    }

    Context& context = mContexts[bufferIndex];
    context.mLinearDepthSampler.applyTextureData(rDepth);
    context.mLinearDepthSampler.setMagFilter(0);
    context.mLinearDepthSampler.setMinFilter(0);

    mode = drawPrimSphere(bufferIndex, context, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                          category);
    mode = drawPrimCylinder(bufferIndex, context, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                            category);
    mode = drawPrimCube(bufferIndex, context, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                        category);
    mode = drawPrimTexCube(bufferIndex, context, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                           category, false, 1);
    mode = drawPrimTexCube(bufferIndex, context, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                           category, true, 0);
    return drawPrimTexCube(bufferIndex, context, pProgram, rDepth, rViewMtx, rProjMtx, mode,
                           category, true, 1);
}

/**
 * Applies the light scale of the light scale drawer to the albedo G-buffer.
 * @param mode Current shader mode.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskKeeper::drawLightScaleToAlbedo(agl::ShaderMode mode) const {
    return mDrawers[ShadowMaskDrawCategory::LightScale]->drawLightScaleToAlbedoGBuffer(mode);
}

}  // namespace al

/**
 * Gets the shadow mask keeper of the scene an actor belongs to.
 * @param pActor Actor.
 * @return Shadow mask keeper.
 */
al::ShadowMaskKeeper* ShadowMaskFunction::getShadowMaskKeeper(const al::LiveActor* pActor) {
    return pActor->getSceneInfo()->graphicsSystemInfo
        ->mShadowDirector->getShadowMaskKeeper();
}

namespace al {

/**
 * Creates a drawer for a draw category.
 * @param pKeeper Keeper.
 * @param drawCategory Draw category.
 * @param pDirector Execute director.
 */
ShadowMaskDrawer::ShadowMaskDrawer(ShadowMaskKeeper* pKeeper, s32 drawCategory,
                                   ExecuteDirector* pDirector)
    : mKeeper(pKeeper), mDrawCategory(drawCategory) {
    const char* name = "対応していないシャドウマスクの描画順です";

    if (u32(drawCategory) < ShadowMaskKeeper::cCategoryNum) {
        name = cDrawerNames[drawCategory];
    }

    registerExecutorUser(this, pDirector, name);
}

/**
 * Draws the shadow masks of the draw category into the G-buffers.
 */
void ShadowMaskDrawer::draw() const {
    if (!mKeeper->isExistDrawShadowMask()) {
        return;
    }

    const agl::TextureData* target = nullptr;
    const agl::TextureData* lightBuffer = nullptr;

    if (mKeeper->getRenderType() != 0) {
        target = mColorTexture;

        if (target == nullptr) {
            target = mGBufferArray->getGBufAlbedoTex();
        }
    } else {
        switch (mDrawCategory) {
        case ShadowMaskDrawCategory::Block:
        case ShadowMaskDrawCategory::Item:
        case ShadowMaskDrawCategory::MapObj:
        case ShadowMaskDrawCategory::Enemy:
        case ShadowMaskDrawCategory::Player:
        case ShadowMaskDrawCategory::MapObjAO:
        case ShadowMaskDrawCategory::EnemyAO:
        case ShadowMaskDrawCategory::PlayerAO:
        case ShadowMaskDrawCategory::AllAO:
        case ShadowMaskDrawCategory::PlayerDecoration:
            target = mGBufferArray->getGBufAlbedoTex();
            break;
        case ShadowMaskDrawCategory::LightScaleLight:
        case ShadowMaskDrawCategory::MapObjAOSO:
        case ShadowMaskDrawCategory::MapObjAndWaterAOSO:
        case ShadowMaskDrawCategory::EnemyAOSO:
        case ShadowMaskDrawCategory::PlayerAOSO:
        case ShadowMaskDrawCategory::AllAOSO:
            target = mGBufferArray->getGBufAlbedoTex();
            lightBuffer = mGBufferArray->getGBufLightBufferTex();
            break;
        case ShadowMaskDrawCategory::LightScale:
            target = mGBufferArray->getGBufLightBufferTex();
            break;
        default:
            break;
        }
    }

    drawToTextureData(target, lightBuffer, cDrawShaderMode);
}

/**
 * Draws the shadow masks of the draw category into the given textures.
 * @param pTarget Main target texture.
 * @param pLightBuffer Light buffer texture (optional).
 * @param mode Current shader mode.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskDrawer::drawToTextureData(const agl::TextureData* pTarget,
                                                    const agl::TextureData* pLightBuffer,
                                                    agl::ShaderMode mode) const {
    if (!mKeeper->isExistDrawShadowMaskCategory(mDrawCategory)) {
        return mode;
    }

    agl::RenderTargetColor target;
    agl::RenderTargetColor lightBuffer;
    target.applyTextureData(*pTarget);

    if (pLightBuffer != nullptr) {
        lightBuffer.applyTextureData(*pLightBuffer);
    }

    s32 width = pTarget->getWidth(0);
    s32 height = pTarget->getHeight(0);
    agl::RenderBuffer renderBuffer;
    renderBuffer.setVirtualSize(sead::Vector2f(width, height));
    renderBuffer.setPhysicalArea(sead::BoundBox2f(0.0f, 0.0f, width, height));
    renderBuffer.setRenderTargetColorNullAll();
    renderBuffer.setRenderTargetColor(&target, 0);

    if (pLightBuffer != nullptr) {
        renderBuffer.setRenderTargetColor(&lightBuffer, 1);
    }

    sead::GraphicsContext context;
    context.setDepthEnable(true, false);
    context.setBlendEnableMask(0);
    context.setColorMask(0u);
    context.setDepthFunc(5);
    s32 targetNum = pLightBuffer != nullptr ? 2 : 1;

    for (s32 i = 0; i < targetNum; i++) {
        context.setBlendEnable(i, true);
        context.setBlendFactorSrcRGB(i, 9);
        context.setBlendFactorSrcA(i, 2);
        context.setBlendFactorDstRGB(i, 1);
        context.setBlendFactorDstA(i, 2);
        context.setBlendEquationRGB(i, 1);
        context.setBlendEquationA(i, 5);

        if (isShadowIntensity(mDrawCategory)) {
            context.setColorMask(i, false, false, false, true);
        } else if (isShadowLightScale(mDrawCategory)) {
            context.setColorMask(i, true, true, true, true);
        } else {
            context.setColorMask(i, true, true, true, false);
        }
    }

    context.setCullingMode(1);
    context.apply(GameFrameworkNx::getDrawContext());
    renderBuffer.bind(GameFrameworkNx::getDrawContext());
    sead::Viewport viewport(renderBuffer);
    viewport.apply(GameFrameworkNx::getDrawContext(), renderBuffer);

    const agl::TextureData* depth = mLinearDepthTexture;

    if (depth == nullptr) {
        depth = mGBufferArray->getGBufDepthViewTex();
    }

    mode = mKeeper->draw(mViewIndex, mShaderProgram, *depth, *mViewMtx, *mProjMtx, mode,
                         mDrawCategory);
    target.invalidateGPUCache(GameFrameworkNx::getAglDrawContext());

    if (pLightBuffer != nullptr) {
        lightBuffer.invalidateGPUCache(GameFrameworkNx::getAglDrawContext());
    }

    mGBufferArray->bindRenderBufferAndContextMRT();
    return mode;
}

/**
 * Applies the light scale to the albedo G-buffer.
 * @param mode Current shader mode.
 * @return Shader mode after drawing.
 */
agl::ShaderMode ShadowMaskDrawer::drawLightScaleToAlbedoGBuffer(agl::ShaderMode mode) const {
    return drawToTextureData(mGBufferArray->getGBufAlbedoTex(), nullptr, mode);
}

}  // namespace al

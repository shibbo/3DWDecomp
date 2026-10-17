#pragma once

#include <basis/seadTypes.h>
#include <common/aglDisplayList.h>
#include <common/aglGPUMemBlock.h>
#include <common/aglIndexStream.h>
#include <common/aglVertexAttribute.h>
#include <common/aglVertexBuffer.h>
#include <math/seadVector.h>

namespace al {
class CameraViewInfo;
class SceneObjHolder;
}  // namespace al

/**
 * @brief Adaptive quadtree grid of flat triangles used to draw large water surfaces.
 *
 * The grid is built every frame around the camera frustum for up to two height tiers and
 * uploaded into double buffered vertex buffers and display lists.
 */
namespace WaveGrid {

constexpr s32 cTierNum = 2;
constexpr s32 cBufferNum = 2;
constexpr s32 cNodeMax = 4000;
constexpr s32 cPatchMax = 4000;
constexpr s32 cTriangleMax = 16000;
constexpr s32 cVertexMax = cTriangleMax * 3;
constexpr s32 cNeighborNum = 8;
constexpr s32 cLodNum = 3;

/**
 * @brief Axis aligned box on the XZ plane.
 */
struct AABB {
    f32 mMaxX;
    f32 mMaxZ;
    f32 mMinX;
    f32 mMinZ;
};

static_assert(sizeof(AABB) == 0x10);

/**
 * @brief One node of the subdivision quadtree.
 *
 * Neighbors are stored as two slots per side, since a side can touch two smaller nodes.
 */
struct QuadPotential {
    QuadPotential();
    QuadPotential(s32 index, s32 level, AABB aabb, sead::Vector3f center, f32 halfSize,
                  bool isSubdividable, bool isForceSubdivide, bool isLeaf, f32 height,
                  QuadPotential* pNext, QuadPotential* pPrev);
    QuadPotential(s32 index, s32 level, const AABB& rAABB, f32 height);
    QuadPotential(s32 index, s32 level, const sead::Vector3f& rCenter, f32 halfSize);
    QuadPotential(const AABB& rAABB, f32 height);

    s32 mIndex;                                // 0x00
    s32 mLevel;                                // 0x04
    AABB mAABB;                                // 0x08
    sead::Vector3f mCenter;                    // 0x18
    f32 mHalfSize;                             // 0x24
    bool mIsSubdividable;                      // 0x28
    bool mIsForceSubdivide;                    // 0x29
    bool mIsLeaf;                              // 0x2a
    f32 mHeight;                               // 0x2c
    QuadPotential* mNeighbors[cNeighborNum];   // 0x30
    QuadPotential* mNext;                      // 0x70
    QuadPotential* mPrev;                      // 0x78
};

static_assert(sizeof(QuadPotential) == 0x80);

/**
 * @brief Leaf node flattened for triangle generation.
 */
struct Patch {
    s32 mType;
    sead::Vector3f mCenter;
    f32 mHalfSize;
};

static_assert(sizeof(Patch) == 0x14);

/**
 * @brief Vertex as it is stored in the vertex buffers.
 */
struct Vertex {
    f32 x;
    f32 y;
    f32 z;
};

static_assert(sizeof(Vertex) == 0xc);

struct Triangle {
    Vertex mVertices[3];
};

static_assert(sizeof(Triangle) == 0x24);

/**
 * @brief Visible region of one tier: the frustum footprint on the tier plane.
 */
struct TierView {
    sead::Vector3f mPoints[5];  // 0x00
    u32 mPointNum;              // 0x3c
    sead::Vector3f mCenter;     // 0x40
    AABB mAABB;                 // 0x4c
    AABB mGridAABB;             // 0x5c
    bool mIsVisible = false;    // 0x6c
};

static_assert(sizeof(TierView) == 0x70);

/**
 * @brief Subdivision distances and minimum node sizes.
 */
struct MeshData {
    al::SceneObjHolder* mSceneObjHolder;  // 0x00
    void* _8;                             // 0x08
    f32 mLodDistance[cLodNum];            // 0x10
    f32 mLodMinSize[cLodNum];             // 0x1c
};

static_assert(sizeof(MeshData) == 0x28);

/**
 * @brief Generated geometry of one tier.
 */
struct TierData {
    QuadPotential mNodes[cNodeMax];   // 0x00000
    u32 mNodeNum = 0;                 // 0x7d000
    QuadPotential* mHead = nullptr;   // 0x7d008
    QuadPotential* mTail = nullptr;   // 0x7d010
    Patch mPatches[cPatchMax];        // 0x7d018
    u32 mPatchNum = 0;                // 0x90898
    Triangle mTriangles[cTriangleMax];  // 0x9089c
    u32 mTriangleNum = 0;             // 0x11d29c
};

static_assert(sizeof(TierData) == 0x11d2a0);

/**
 * @brief GPU side resources, double buffered per tier.
 */
struct RenderData {
    void* _0;                                                                // 0x0000
    agl::GPUMemBlock<Vertex> mVertexBlocks[cTierNum][cBufferNum];    // 0x0008
    agl::VertexBuffer* mVertexBuffers[cTierNum][cBufferNum];                 // 0x00e8
    agl::VertexAttribute mVertexAttributes[cTierNum][cBufferNum];            // 0x0108
    agl::GPUMemBlock<u32> mIndexBlocks[cTierNum][cBufferNum];                // 0x0888
    agl::IndexStream mIndexStreams[cTierNum][cBufferNum];                    // 0x0968
    agl::DisplayList mDisplayLists[cTierNum][cBufferNum];                    // 0x0ae8
    agl::GPUMemBlock<u8> mCommandBlocks[cTierNum][cBufferNum];               // 0x1468
};

static_assert(sizeof(RenderData) == 0x1548);

/**
 * @brief Whole state of a wave grid.
 */
struct Data {
    f32 mTierHeights[cTierNum] = {0.0f, 2501.0f};  // 0x000000
    sead::Vector3f mCameraPos;              // 0x000008
    sead::Vector3f mCameraAt;               // 0x000014
    sead::Vector3f mFrustumPoints[8];       // 0x000020
    TierView mTierViews[cTierNum];          // 0x000080
    bool mIsSingleTier = false;             // 0x000160
    MeshData mMeshData;                     // 0x000168
    TierData mTiers[cTierNum];              // 0x000190
    void* _23a6d0;                          // 0x23a6d0
    RenderData mRenderData;                 // 0x23a6d8
};

static_assert(offsetof(Data, mRenderData) == 0x23a6d8);

void InitRenderData(RenderData& rRenderData);
void InitMeshData(MeshData& rMeshData, const al::CameraViewInfo& rCameraViewInfo,
                  al::SceneObjHolder* pSceneObjHolder);
void EnableMultipleTiers(Data& rData, bool isEnable);
void Generate(Data& rData, const al::CameraViewInfo& rCameraViewInfo, u32 bufferIndex);
void FlushCPU(Data& rData, u32 bufferIndex);
bool IsTierVisible(const Data& rData, u32 tier);
u32 TierTriCount(const Data& rData, u32 tier);
void DisplayTier(const Data& rData, u32 tier, u32 bufferIndex);
void Free(Data& rData);

}  // namespace WaveGrid

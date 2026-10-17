#include "MapObj/WaveGrid.hpp"

#include <cfloat>
#include <cmath>
#include <new>
#include <common/aglDrawContext.h>
#include <common/aglGPUMemAddr.h>
#include <driver/aglGraphicsDriverMgr.h>
#include <gfx/seadGraphics.h>
#include <nvn/nvn_FuncPtrInline.h>

#include "Library/Clipping/ViewFrustum.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Memory/Util.hpp"
#include "Library/Projection/Projection.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"

namespace {

using namespace WaveGrid;

/**
 * Patch types: the product of one factor per side, the smaller factor is used when the
 * neighbor on that side is subdivided further and the patch needs an extra edge vertex.
 */
enum PatchType : s32 {
    cPatchType_Fine0Fine1 = 24 * 3 * 6 * 8,
    cPatchType_Fine0Fine3 = 24 * 4 * 6 * 7,
    cPatchType_Fine0 = 24 * 4 * 6 * 8,
    cPatchType_Fine1Fine2 = 48 * 3 * 5 * 8,
    cPatchType_Fine2Fine3 = 48 * 4 * 5 * 7,
    cPatchType_Fine1 = 48 * 3 * 6 * 8,
    cPatchType_Fine2 = 48 * 4 * 5 * 8,
    cPatchType_Fine3 = 48 * 4 * 6 * 7,
};

/**
 * Get the CPU mapping of a GPU memory block.
 * @param rBlock block to map
 * @return first element of the block
 */
template <typename T>
T* getBufferPtr(const agl::GPUMemBlock<T>& rBlock) {
    return reinterpret_cast<T*>(
        static_cast<u8*>(nvnMemoryPoolMap(rBlock.getMemoryPool()->getDriverPool())) +
        rBlock.getByteOffset());
}

/**
 * Checks whether two boxes overlap.
 * @param rA first box
 * @param rB second box
 * @return true if the boxes overlap
 */
inline bool isOverlap(const AABB& rA, const AABB& rB) {
    if (rA.mMinX > rB.mMaxX) {
        return false;
    }

    if (rA.mMaxX < rB.mMinX) {
        return false;
    }

    if (rA.mMinZ > rB.mMaxZ) {
        return false;
    }

    if (rA.mMaxZ < rB.mMinZ) {
        return false;
    }

    return true;
}

/**
 * Checks whether a node is close enough to the view center to be subdivided.
 * @param rMeshData subdivision distances
 * @param rView visible region of the tier
 * @param pNode node to check
 * @return true if the node has to be subdivided
 */
inline bool isNeedSubdivide(const MeshData& rMeshData, const TierView& rView,
                            const QuadPotential* pNode) {
    if (!pNode->mIsSubdividable) {
        return false;
    }

    if (!isOverlap(pNode->mAABB, rView.mAABB)) {
        return false;
    }

    for (s32 i = cLodNum - 1; i >= 0; i--) {
        f32 size = pNode->mHalfSize * 2.0f;
        f32 lodDistance = rMeshData.mLodDistance[i];
        sead::Vector3f diff(rView.mCenter.x, pNode->mCenter.y, rView.mCenter.z);
        diff -= pNode->mCenter;

        if (diff.length() < size + lodDistance &&
            pNode->mAABB.mMaxX - pNode->mAABB.mMinX > rMeshData.mLodMinSize[i]) {
            return true;
        }
    }

    return false;
}

/**
 * Replaces the links of a neighbor side after a node was split.
 * @param pNode split node
 * @param side first neighbor slot of the side
 * @param opposite first neighbor slot of the opposite side
 * @param pChildA child touching the first slot
 * @param pChildB child touching the second slot
 */
inline void relinkSide(QuadPotential* pNode, s32 side, s32 opposite, QuadPotential* pChildA,
                       QuadPotential* pChildB) {
    if (pNode->mNeighbors[side] != nullptr) {
        bool isSameNeighbor = pNode->mNeighbors[side] == pNode->mNeighbors[side + 1];
        pNode->mNeighbors[side]->mNeighbors[opposite] = pChildA;

        if (!isSameNeighbor) {
            pNode->mNeighbors[side]->mNeighbors[opposite + 1] = pChildA;
            pNode->mNeighbors[side + 1]->mNeighbors[opposite] = pChildB;
        }

        pNode->mNeighbors[side + 1]->mNeighbors[opposite + 1] = pChildB;
    }
}

/**
 * Walks the node list until a node has to be subdivided.
 * @param rMeshData subdivision distances
 * @param rTier tier of the nodes
 * @param rView visible region of the tier
 * @param pNode first node to check
 * @return node to subdivide, nullptr if the end of the list was reached
 */
inline QuadPotential* findSubdivideNode(const MeshData& rMeshData, TierData& rTier,
                                        const TierView& rView, QuadPotential* pNode) {
    QuadPotential* node = pNode;
    while (node != nullptr) {
        node->mIsLeaf = true;

        if (node->mIsForceSubdivide || isNeedSubdivide(rMeshData, rView, node)) {
            bool isNeighborSubdivided = false;
            for (s32 i = 0; i < cNeighborNum; i++) {
                QuadPotential* neighbor = node->mNeighbors[i];

                if (neighbor != nullptr && node->mLevel > neighbor->mLevel &&
                    isOverlap(node->mAABB, rView.mAABB)) {
                    neighbor->mIsForceSubdivide = true;
                    isNeighborSubdivided = true;
                }
            }

            if (!isNeighborSubdivided) {
                return node;
            }

            node->mIsForceSubdivide = true;
            node = rTier.mHead;
        } else {
            node->mIsSubdividable = false;
            node = node->mNext;
        }
    }

    return nullptr;
}

/**
 * Builds the quadtree of one tier.
 * @param rData grid data
 * @param rTier tier to build
 * @param rView visible region of the tier
 * @param height tier height
 * @return false if the node pool overflowed
 */
inline bool buildTree(Data& rData, TierData& rTier, const TierView& rView, f32 height) {
    new (&rTier.mNodes[0]) QuadPotential(0, 0, rView.mGridAABB, height);
    rTier.mNodeNum = 1;
    rTier.mHead = &rTier.mNodes[0];
    rTier.mTail = &rTier.mNodes[0];

    QuadPotential* node = rTier.mHead;
    while (true) {
        node = findSubdivideNode(rData.mMeshData, rTier, rView, node);
        if (node == nullptr) {
            break;
        }

        u32 base = rTier.mNodeNum;
        if (base + 4 >= cNodeMax) {
            return false;
        }

        rTier.mNodeNum = base + 4;

        QuadPotential* child0 = &rTier.mNodes[base];
        QuadPotential* child1 = &rTier.mNodes[base + 1];
        QuadPotential* child2 = &rTier.mNodes[base + 2];
        QuadPotential* child3 = &rTier.mNodes[base + 3];

        f32 quarter = node->mHalfSize * 0.5f;
        new (child0) QuadPotential(rTier.mNodeNum - 4, node->mLevel + 1,
                                   node->mCenter + sead::Vector3f(quarter, 0.0f, quarter), quarter);
        new (child1) QuadPotential(rTier.mNodeNum - 3, node->mLevel + 1,
                                   node->mCenter + sead::Vector3f(-quarter, 0.0f, quarter), quarter);
        new (child2) QuadPotential(rTier.mNodeNum - 2, node->mLevel + 1,
                                   node->mCenter + sead::Vector3f(quarter, 0.0f, -quarter), quarter);
        new (child3) QuadPotential(rTier.mNodeNum - 1, node->mLevel + 1,
                                   node->mCenter + sead::Vector3f(-quarter, 0.0f, -quarter),
                                   quarter);

        child0->mNeighbors[0] = node->mNeighbors[0];
        child0->mNeighbors[1] = node->mNeighbors[0];
        child0->mNeighbors[2] = node->mNeighbors[2];
        child0->mNeighbors[3] = node->mNeighbors[2];
        child1->mNeighbors[4] = node->mNeighbors[4];
        child1->mNeighbors[5] = node->mNeighbors[4];
        child1->mNeighbors[2] = node->mNeighbors[3];
        child1->mNeighbors[3] = node->mNeighbors[3];
        child2->mNeighbors[0] = node->mNeighbors[1];
        child2->mNeighbors[1] = node->mNeighbors[1];
        child2->mNeighbors[6] = node->mNeighbors[6];
        child2->mNeighbors[7] = node->mNeighbors[6];
        child3->mNeighbors[4] = node->mNeighbors[5];
        child3->mNeighbors[5] = node->mNeighbors[5];
        child3->mNeighbors[6] = node->mNeighbors[7];
        child3->mNeighbors[7] = node->mNeighbors[7];

        relinkSide(node, 0, 4, child0, child2);
        relinkSide(node, 2, 6, child0, child1);
        relinkSide(node, 4, 0, child1, child3);
        relinkSide(node, 6, 2, child2, child3);

        child0->mNeighbors[4] = child1;
        child0->mNeighbors[5] = child1;
        child0->mNeighbors[6] = child2;
        child0->mNeighbors[7] = child2;
        child1->mNeighbors[0] = child0;
        child1->mNeighbors[1] = child0;
        child1->mNeighbors[6] = child3;
        child1->mNeighbors[7] = child3;
        child2->mNeighbors[2] = child0;
        child2->mNeighbors[3] = child0;
        child2->mNeighbors[4] = child3;
        child2->mNeighbors[5] = child3;
        child3->mNeighbors[2] = child1;
        child3->mNeighbors[3] = child1;
        child3->mNeighbors[0] = child2;
        child3->mNeighbors[1] = child2;

        rTier.mTail->mNext = child0;
        child0->mNext = child1;
        child0->mPrev = rTier.mTail;
        child1->mNext = child2;
        child1->mPrev = child0;
        child2->mNext = child3;
        child2->mPrev = child1;
        child3->mNext = nullptr;
        child3->mPrev = child2;
        rTier.mTail = child3;

        QuadPotential* nextNode;
        if (node->mPrev != nullptr) {
            nextNode = node->mPrev;
        } else if (node->mNext != nullptr) {
            nextNode = node->mNext;
        } else {
            nextNode = child0;
        }

        s32 index = node->mIndex;

        if (rTier.mHead == node) {
            rTier.mHead = node->mNext;
        }

        if (node->mPrev != nullptr) {
            node->mPrev->mNext = node->mNext;
        }

        if (node->mNext != nullptr) {
            node->mNext->mPrev = node->mPrev;
        }

        if (rTier.mTail == node) {
            rTier.mTail = node->mPrev;
        }

        QuadPotential* slot = &rTier.mNodes[index];
        QuadPotential* last = &rTier.mNodes[rTier.mNodeNum - 1];
        for (s32 i = 0; i < cNeighborNum; i++) {
            if (last->mNeighbors[i] != nullptr) {
                for (s32 j = 0; j < cNeighborNum; j++) {
                    if (last->mNeighbors[i]->mNeighbors[j] != nullptr &&
                        last->mNeighbors[i]->mNeighbors[j] == last) {
                        last->mNeighbors[i]->mNeighbors[j] = slot;
                    }
                }
            }
        }

        rTier.mNodes[index] = rTier.mNodes[rTier.mNodeNum - 1];
        slot->mIndex = index;
        rTier.mNodeNum--;

        if (rTier.mTail != nullptr && rTier.mTail->mIndex == rTier.mNodeNum) {
            rTier.mTail = slot;
        }

        if (rTier.mHead != nullptr && rTier.mHead->mIndex == rTier.mNodeNum) {
            rTier.mHead = slot;
        }

        if (node->mPrev != nullptr) {
            node->mPrev->mNext = node;
        }

        node = nextNode;
    }

    return true;
}

/**
 * Converts a position to a vertex.
 * @param rPos position
 * @return vertex at the position
 */
inline Vertex toVertex(const sead::Vector3f& rPos) {
    return {rPos.x, rPos.y, rPos.z};
}

/**
 * Gets the corner at +X/+Z of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getCornerPP(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(rPatch.mHalfSize, 0.0f, rPatch.mHalfSize));
}

/**
 * Gets the corner at -X/+Z of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getCornerMP(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(-rPatch.mHalfSize, 0.0f, rPatch.mHalfSize));
}

/**
 * Gets the corner at +X/-Z of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getCornerPM(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(rPatch.mHalfSize, 0.0f, -rPatch.mHalfSize));
}

/**
 * Gets the corner at -X/-Z of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getCornerMM(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(-rPatch.mHalfSize, 0.0f, -rPatch.mHalfSize));
}

/**
 * Gets the middle of the +X edge of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getEdgeP0(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(rPatch.mHalfSize, 0.0f, 0.0f));
}

/**
 * Gets the middle of the -X edge of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getEdgeM0(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(-rPatch.mHalfSize, 0.0f, 0.0f));
}

/**
 * Gets the middle of the +Z edge of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getEdge0P(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(0.0f, 0.0f, rPatch.mHalfSize));
}

/**
 * Gets the middle of the -Z edge of a patch.
 * @param rPatch patch
 * @return vertex position
 */
inline Vertex getEdge0M(const Patch& rPatch) {
    return toVertex(rPatch.mCenter + sead::Vector3f(0.0f, 0.0f, -rPatch.mHalfSize));
}

/**
 * Gets the type factor of one patch side.
 * @param rNode leaf node
 * @param side first neighbor slot of the side
 * @param fine factor if the neighbor is subdivided further
 * @param coarse factor otherwise
 * @return side factor
 */
inline s32 getSideFactor(const QuadPotential& rNode, s32 side, s32 fine, s32 coarse) {
    if (rNode.mNeighbors[side] != nullptr && rNode.mNeighbors[side]->mLevel > rNode.mLevel) {
        return fine;
    }

    return coarse;
}

/**
 * Flattens the leaf nodes of a tier into patches.
 * @param rTier tier to process
 */
inline void makePatches(TierData& rTier) {
    rTier.mPatchNum = 0;

    u32 nodeNum = rTier.mNodeNum;
    for (u32 i = 0; i < nodeNum; i++) {
        const QuadPotential& node = rTier.mNodes[i];

        if (!node.mIsLeaf) {
            continue;
        }

        Patch& patch = rTier.mPatches[rTier.mPatchNum++];
        s32 factor0 = getSideFactor(node, 0, 24, 48);
        s32 factor1 = getSideFactor(node, 2, 3, 4);
        s32 factor2 = getSideFactor(node, 4, 5, 6);
        s32 factor3 = getSideFactor(node, 6, 7, 8);
        patch.mType = factor0 * factor1 * factor2 * factor3;
        patch.mCenter.x =
            node.mAABB.mMinX + (node.mAABB.mMaxX - node.mAABB.mMinX) * 0.5f;
        patch.mCenter.y = node.mHeight;
        patch.mCenter.z =
            node.mAABB.mMinZ + (node.mAABB.mMaxZ - node.mAABB.mMinZ) * 0.5f;
        patch.mHalfSize = (node.mAABB.mMaxX - node.mAABB.mMinX) * 0.5f;
    }
}

/**
 * Records the draw call of one tier into its display list.
 * @param rData grid data
 * @param tier tier to record
 * @param bufferIndex buffer to record into
 */
inline void recordDisplayList(Data& rData, u32 tier, u32 bufferIndex) {
    RenderData& renderData = rData.mRenderData;

    sead::Graphics::instance()->lockDrawContext();
    {
        agl::DrawContext context;
        agl::DisplayList* displayList = &renderData.mDisplayLists[tier][bufferIndex];
        context.setCommandBuffer(displayList);
        const agl::GPUMemBlock<u8>& commandBlock = renderData.mCommandBlocks[tier][bufferIndex];
        displayList->beginDisplayListBuffer(agl::GPUMemAddr<u8>(commandBlock, 0),
                                            static_cast<s32>(commandBlock.getSize()), true);

        u32 triangleNum = rData.mTiers[tier].mTriangleNum;
        u32 count = triangleNum == 0 ? 3 : triangleNum * 3;
        agl::IndexStream& indexStream = renderData.mIndexStreams[tier][bufferIndex];
        indexStream.setCount(count);

        if (count != 0) {
            NVNdrawPrimitive primitive = indexStream.getPrimitiveType();
            NVNcommandBuffer* commandBuffer = agl::driver::getNvnCommandBuffer(&context);
            NVNbufferAddress address = nvnBufferGetAddress(indexStream.getNvnBuffer());
            nvnCommandBufferDrawElements(commandBuffer, primitive,
                                         NVNindexType(indexStream.getFormat()), count, address);
        }

        displayList->endDisplayList();
    }
    sead::Graphics::instance()->unlockDrawContext();
}

/**
 * Calculates the footprint of the camera frustum on a tier plane.
 * @param rData grid data
 * @param rView visible region to fill
 * @param rFrustum camera frustum
 * @param height tier height
 */
inline void calcViewPoints(Data& rData, TierView& rView, const al::ViewFrustum& rFrustum,
                           f32 height) {
    al::ViewFrustum::PlaneIntersection intersection;
    intersection.mPointNum = 0;
    sead::Plane3f plane(sead::Vector3f(0.0f, 1.0f, 0.0f), height + 0.0f);
    rFrustum.calcPlaneIntersection(&intersection, &plane);

    for (s32 i = 0; i < 4; i++) {
        if (!intersection.mIsIntersectEdge[i]) {
            const sead::Vector3f& farPoint = rData.mFrustumPoints[i + 4];
            intersection.mPoints[i].set(farPoint.x, height, farPoint.z);
            intersection.mIsIntersectEdge[i] = true;
            intersection.mPointNum++;
        }
    }

    sead::Vector3f cameraDir = rData.mCameraAt - rData.mCameraPos;
    rView.mPointNum = 0;
    for (u32 i = 0; i < intersection.mPointNum; i++) {
        const sead::Vector3f& point = intersection.mPoints[i];

        if (cameraDir.dot(point - rData.mCameraPos) > 0.0f) {
            rView.mPoints[rView.mPointNum++] = point;
        } else {
            rView.mPoints[rView.mPointNum] = rData.mFrustumPoints[i + 4];
            rView.mPointNum++;
        }
    }
}

/**
 * Calculates the visible region of every tier from the camera frustum.
 * @param rData grid data
 * @param rCameraViewInfo camera to build the grid for
 */
inline void calcTierViews(Data& rData, const al::CameraViewInfo& rCameraViewInfo) {
    al::ViewFrustum frustum;
    const sead::LookAtCamera& camera = rCameraViewInfo.getLookAtCam();
    frustum.calcFrustumFromViewMtx(
        camera.getMatrix(), rCameraViewInfo.getProjection().getFovy() * 57.29578f,
        rCameraViewInfo.getAspect(), rCameraViewInfo.getNear(),
        sead::Mathf::clampMin(rCameraViewInfo.getFar(), 100000.0f));

    rData.mCameraPos = rCameraViewInfo.getLookAtCam().getPos();
    rData.mCameraAt = rCameraViewInfo.getLookAtCam().getAt();
    frustum.calcFrustumPoints(reinterpret_cast<al::ViewFrustum::Points*>(rData.mFrustumPoints));

    for (s32 tier = 0; tier < cTierNum; tier++) {
        TierView& view = rData.mTierViews[tier];
        view.mIsVisible = true;

        f32 height = rData.mTierHeights[tier];
        if (tier != 0 && rData.mIsSingleTier) {
            view.mIsVisible = false;
            continue;
        }

        if (rData.mCameraPos.y < height) {
            view.mIsVisible = false;
            continue;
        }

        calcViewPoints(rData, view, frustum, height);

        if (!view.mIsVisible) {
            continue;
        }

        view.mCenter = rData.mCameraPos;

        f32 extend[4];
        extend[0] = 800.0f;
        extend[1] = 200.0f;
        extend[2] = 800.0f;
        extend[3] = 200.0f;
        sead::Vector3f sum(0.0f, 0.0f, 0.0f);
        for (u32 i = 0; i < view.mPointNum; i++) {
            sum += view.mPoints[i];
        }

        sead::Vector3f centroid = sum * (1.0f / view.mPointNum);
        for (u32 i = 0; i < view.mPointNum; i++) {
            sead::Vector3f dir = view.mPoints[i] - centroid;
            dir.normalize();
            view.mPoints[i] = view.mPoints[i] + dir * extend[i];
        }

        AABB& aabb = view.mAABB;
        aabb.mMinX = FLT_MAX;
        aabb.mMinZ = FLT_MAX;
        aabb.mMaxX = -FLT_MAX;
        aabb.mMaxZ = -FLT_MAX;
        for (u32 i = 0; i < view.mPointNum; i++) {
            if (view.mPoints[i].x > aabb.mMaxX) {
                aabb.mMaxX = view.mPoints[i].x;
            }

            if (view.mPoints[i].z > aabb.mMaxZ) {
                aabb.mMaxZ = view.mPoints[i].z;
            }

            if (view.mPoints[i].x < aabb.mMinX) {
                aabb.mMinX = view.mPoints[i].x;
            }

            if (view.mPoints[i].z < aabb.mMinZ) {
                aabb.mMinZ = view.mPoints[i].z;
            }
        }

        f32 sizeX = aabb.mMaxX - aabb.mMinX;
        f32 sizeZ = aabb.mMaxZ - aabb.mMinZ;
        f32 size = sizeX < sizeZ ? sizeZ : sizeX;
        f32 cellSize = exp2(static_cast<f64>(ceilf(logf(size / 100.0f) / 0.6931472f))) * 100.0;

        view.mGridAABB.mMinX = floorf(aabb.mMinX / cellSize) * cellSize;
        view.mGridAABB.mMinZ = floorf(aabb.mMinZ / cellSize) * cellSize;
        view.mGridAABB.mMaxX = cellSize * 2.0f + view.mGridAABB.mMinX;
        view.mGridAABB.mMaxZ = cellSize * 2.0f + view.mGridAABB.mMinZ;
    }
}

}  // namespace

namespace WaveGrid {

/**
 * Creates an unused node.
 */
QuadPotential::QuadPotential()
    : mIndex(-1), mLevel(0), mAABB(), mCenter(0.0f, 0.0f, 0.0f), mHalfSize(0.0f),
      mIsSubdividable(true), mIsForceSubdivide(false), mIsLeaf(true), mHeight(0.0f),
      mNeighbors(), mNext(nullptr), mPrev(nullptr) {}

/**
 * Creates a node from all of its values.
 * @param index index in the node pool
 * @param level subdivision depth
 * @param aabb bounds on the XZ plane
 * @param center center position
 * @param halfSize half of the edge length
 * @param isSubdividable whether the node may be subdivided
 * @param isForceSubdivide whether the node has to be subdivided
 * @param isLeaf whether the node is a leaf
 * @param height surface height
 * @param pNext next node in the processing list
 * @param pPrev previous node in the processing list
 */
QuadPotential::QuadPotential(s32 index, s32 level, AABB aabb, sead::Vector3f center,
                             f32 halfSize, bool isSubdividable, bool isForceSubdivide,
                             bool isLeaf, f32 height, QuadPotential* pNext,
                             QuadPotential* pPrev)
    : mIndex(index), mLevel(level), mAABB(aabb), mCenter(center), mHalfSize(halfSize),
      mIsSubdividable(isSubdividable), mIsForceSubdivide(isForceSubdivide), mIsLeaf(isLeaf),
      mHeight(height), mNext(pNext), mPrev(pPrev) {
    for (s32 i = 0; i < cNeighborNum; i++) {
        mNeighbors[i] = nullptr;
    }
}

/**
 * Creates a node covering a box.
 * @param index index in the node pool
 * @param level subdivision depth
 * @param rAABB bounds on the XZ plane
 * @param height surface height
 */
QuadPotential::QuadPotential(s32 index, s32 level, const AABB& rAABB, f32 height)
    : QuadPotential(index, level, rAABB,
                    sead::Vector3f(rAABB.mMinX + (rAABB.mMaxX - rAABB.mMinX) * 0.5f, height,
                                   rAABB.mMinZ + (rAABB.mMaxZ - rAABB.mMinZ) * 0.5f),
                    (rAABB.mMaxX - rAABB.mMinX) * 0.5f, true, false, true, height, nullptr,
                    nullptr) {}

/**
 * Creates a square node around a center.
 * @param index index in the node pool
 * @param level subdivision depth
 * @param rCenter center position
 * @param halfSize half of the edge length
 */
QuadPotential::QuadPotential(s32 index, s32 level, const sead::Vector3f& rCenter, f32 halfSize)
    : QuadPotential(index, level,
                    {rCenter.x + halfSize, rCenter.z + halfSize, rCenter.x - halfSize,
                     rCenter.z - halfSize},
                    rCenter, halfSize, true, false, true, rCenter.y, nullptr, nullptr) {}

/**
 * Creates a root node covering a box.
 * @param rAABB bounds on the XZ plane
 * @param height surface height
 */
QuadPotential::QuadPotential(const AABB& rAABB, f32 height) : QuadPotential(0, 0, rAABB, height) {}

/**
 * Allocates the vertex buffers, index buffers and display lists of both tiers.
 * @param rRenderData render data to initialize
 */
void InitRenderData(RenderData& rRenderData) {
    for (s32 tier = 0; tier < cTierNum; tier++) {
        for (s32 i = 0; i < cBufferNum; i++) {
            agl::GPUMemBlock<Vertex>& vertexBlock = rRenderData.mVertexBlocks[tier][i];
            vertexBlock.allocBuffer(cVertexMax, al::getCurrentHeap(), 8,
                                    agl::MemoryAttribute::Default);
            agl::GPUMemAddr<Vertex> vertexAddr(vertexBlock, 0);

            for (s32 j = 0; j < cVertexMax; j++) {
                getBufferPtr(vertexBlock)[j] = {-100000.0f, 0.0f, 100000.0f};
            }

            agl::VertexBuffer* vertexBuffer = new agl::VertexBuffer();
            rRenderData.mVertexBuffers[tier][i] = vertexBuffer;
            vertexBuffer->setUpBuffer(agl::ConstGPUMemVoidAddr(vertexBlock, 0),
                                      sizeof(Vertex), sizeof(Vertex) * cVertexMax);
            rRenderData.mVertexBuffers[tier][i]->setUpStream(0, agl::VertexStreamFormat(0x22),
                                                             0, false);

            agl::VertexAttribute& attribute = rRenderData.mVertexAttributes[tier][i];
            attribute.create(1, nullptr);
            attribute.setVertexStream(0, rRenderData.mVertexBuffers[tier][i], 0);
            attribute.setUp();
        }

        for (s32 i = 0; i < cBufferNum; i++) {
            agl::GPUMemBlock<u32>& indexBlock = rRenderData.mIndexBlocks[tier][i];
            indexBlock.allocBuffer(cVertexMax, nullptr, 8, agl::MemoryAttribute::Default);
            agl::GPUMemAddr<u32> indexAddr(indexBlock, 0);

            u32* indices = getBufferPtr(indexBlock);
            for (u32 j = 0; j < cVertexMax; j++) {
                indices[j] = j;
            }

            agl::DisplayList* displayList = &rRenderData.mDisplayLists[tier][i];
            agl::IndexStream& indexStream = rRenderData.mIndexStreams[tier][i];
            agl::GPUMemBlock<u8>& commandBlock = rRenderData.mCommandBlocks[tier][i];
            commandBlock.allocBuffer(0x2000, nullptr, 8, agl::MemoryAttribute::Default);
            agl::GPUMemAddr<u8> commandAddr(commandBlock, 0);

            sead::Graphics::instance()->lockDrawContext();
            {
                agl::DrawContext context;
                context.setCommandBuffer(displayList);
                displayList->beginDisplayListBuffer(agl::GPUMemAddr<u8>(commandBlock, 0),
                                                    static_cast<s32>(commandBlock.getSize()),
                                                    true);
                indexStream.setUpStream(agl::GPUMemAddr<u32>(indexBlock, 0), cVertexMax);
                indexStream.setPrimitiveType(NVN_DRAW_PRIMITIVE_TRIANGLES);
                displayList->endDisplayList();
            }
            sead::Graphics::instance()->unlockDrawContext();
        }
    }
}

/**
 * Sets up the default subdivision distances.
 * @param rMeshData mesh data to initialize
 * @param rCameraViewInfo camera of the grid
 * @param pSceneObjHolder scene object holder
 */
void InitMeshData(MeshData& rMeshData, const al::CameraViewInfo& rCameraViewInfo,
                  al::SceneObjHolder* pSceneObjHolder) {
    rMeshData.mSceneObjHolder = pSceneObjHolder;
    rMeshData._8 = nullptr;
    rCameraViewInfo.getFar();
    rMeshData.mLodMinSize[0] = 100.0f;
    rMeshData.mLodDistance[0] = 2000.0f;
    rMeshData.mLodMinSize[1] = 200.0f;
    rMeshData.mLodMinSize[2] = 400.0f;
    rMeshData.mLodDistance[1] = 4000.0f;
    rMeshData.mLodDistance[2] = 8000.0f;
}

/**
 * Enables or disables the second tier.
 * @param rData grid data
 * @param isEnable whether both tiers are generated
 */
void EnableMultipleTiers(Data& rData, bool isEnable) {
    rData.mIsSingleTier = !isEnable;
}

/**
 * Builds the grid of all visible tiers for the current camera and records the draw calls.
 * @param rData grid data
 * @param rCameraViewInfo camera to build the grid for
 * @param bufferIndex buffer to write the geometry into
 */
void Generate(Data& rData, const al::CameraViewInfo& rCameraViewInfo, u32 bufferIndex) {
    calcTierViews(rData, rCameraViewInfo);

    for (s32 tier = 0; tier < cTierNum; tier++) {
        TierData& tierData = rData.mTiers[tier];
        tierData.mNodeNum = 0;

        f32 height = rData.mTierHeights[tier];
        if (rData.mCameraPos.y < height || !rData.mTierViews[tier].mIsVisible) {
            continue;
        }

        if (!buildTree(rData, tierData, rData.mTierViews[tier], height)) {
            break;
        }
    }

    for (s32 tier = 0; tier < cTierNum; tier++) {
        makePatches(rData.mTiers[tier]);
    }

    for (s32 tier = 0; tier < cTierNum; tier++) {
        TierData& tierData = rData.mTiers[tier];
        u32 triangleNum = 0;

        u32 patchNum = tierData.mPatchNum;
        for (u32 i = 0; i < patchNum; i++) {
            const Patch& patch = tierData.mPatches[i];
            Triangle* triangles = tierData.mTriangles;

            switch (patch.mType) {
            case cPatchType_Fine0Fine3:
                triangles[triangleNum].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum].mVertices[1] = getEdgeP0(patch);
                triangles[triangleNum].mVertices[2] = getEdge0M(patch);
                triangles[triangleNum + 1].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum + 1].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum + 1].mVertices[2] = getEdgeP0(patch);
                triangles[triangleNum + 2].mVertices[0] = getCornerMM(patch);
                triangles[triangleNum + 2].mVertices[1] = getCornerMP(patch);
                triangles[triangleNum + 2].mVertices[2] = getEdge0M(patch);
                triangles[triangleNum + 3].mVertices[0] = getEdge0M(patch);
                triangles[triangleNum + 3].mVertices[1] = getEdgeP0(patch);
                triangles[triangleNum + 3].mVertices[2] = getCornerPM(patch);
                triangleNum += 4;
                break;
            case cPatchType_Fine1:
                triangles[triangleNum].mVertices[0] = getEdge0P(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getCornerPM(patch);
                triangles[triangleNum + 1].mVertices[0] = getEdge0P(patch);
                triangles[triangleNum + 1].mVertices[1] = getCornerPM(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerMM(patch);
                triangles[triangleNum + 2].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum + 2].mVertices[1] = getEdge0P(patch);
                triangles[triangleNum + 2].mVertices[2] = getCornerMM(patch);
                triangleNum += 3;
                break;
            case cPatchType_Fine1Fine2:
                triangles[triangleNum].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum].mVertices[1] = getEdge0P(patch);
                triangles[triangleNum].mVertices[2] = getCornerPM(patch);
                triangles[triangleNum + 1].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum + 1].mVertices[1] = getEdge0P(patch);
                triangles[triangleNum + 1].mVertices[2] = getEdgeM0(patch);
                triangles[triangleNum + 2].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum + 2].mVertices[1] = getCornerPM(patch);
                triangles[triangleNum + 2].mVertices[2] = getCornerMM(patch);
                triangles[triangleNum + 3].mVertices[0] = getEdge0P(patch);
                triangles[triangleNum + 3].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum + 3].mVertices[2] = getCornerPM(patch);
                triangleNum += 4;
                break;
            case cPatchType_Fine3:
                triangles[triangleNum].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getEdge0M(patch);
                triangles[triangleNum + 1].mVertices[0] = getEdge0M(patch);
                triangles[triangleNum + 1].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerPM(patch);
                triangles[triangleNum + 2].mVertices[0] = getCornerMM(patch);
                triangles[triangleNum + 2].mVertices[1] = getCornerMP(patch);
                triangles[triangleNum + 2].mVertices[2] = getEdge0M(patch);
                triangleNum += 3;
                break;
            case cPatchType_Fine0Fine1:
                triangles[triangleNum].mVertices[0] = getEdge0P(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getEdgeP0(patch);
                triangles[triangleNum + 1].mVertices[0] = getEdge0P(patch);
                triangles[triangleNum + 1].mVertices[1] = getEdgeP0(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerMM(patch);
                triangles[triangleNum + 2].mVertices[0] = getCornerMM(patch);
                triangles[triangleNum + 2].mVertices[1] = getEdgeP0(patch);
                triangles[triangleNum + 2].mVertices[2] = getCornerPM(patch);
                triangles[triangleNum + 3].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum + 3].mVertices[1] = getEdge0P(patch);
                triangles[triangleNum + 3].mVertices[2] = getCornerMM(patch);
                triangleNum += 4;
                break;
            case cPatchType_Fine2Fine3:
                triangles[triangleNum].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getEdge0M(patch);
                triangles[triangleNum + 1].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum + 1].mVertices[1] = getCornerMP(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerPP(patch);
                triangles[triangleNum + 2].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum + 2].mVertices[1] = getEdge0M(patch);
                triangles[triangleNum + 2].mVertices[2] = getCornerMM(patch);
                triangles[triangleNum + 3].mVertices[0] = getEdge0M(patch);
                triangles[triangleNum + 3].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum + 3].mVertices[2] = getCornerPM(patch);
                triangleNum += 4;
                break;
            case cPatchType_Fine0:
                triangles[triangleNum].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getEdgeP0(patch);
                triangles[triangleNum + 1].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum + 1].mVertices[1] = getEdgeP0(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerMM(patch);
                triangles[triangleNum + 2].mVertices[0] = getEdgeP0(patch);
                triangles[triangleNum + 2].mVertices[1] = getCornerPM(patch);
                triangles[triangleNum + 2].mVertices[2] = getCornerMM(patch);
                triangleNum += 3;
                break;
            case cPatchType_Fine2:
                triangles[triangleNum].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getEdgeM0(patch);
                triangles[triangleNum + 1].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum + 1].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerPM(patch);
                triangles[triangleNum + 2].mVertices[0] = getEdgeM0(patch);
                triangles[triangleNum + 2].mVertices[1] = getCornerPM(patch);
                triangles[triangleNum + 2].mVertices[2] = getCornerMM(patch);
                triangleNum += 3;
                break;
            default:
                triangles[triangleNum].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum].mVertices[1] = getCornerPP(patch);
                triangles[triangleNum].mVertices[2] = getCornerPM(patch);
                triangles[triangleNum + 1].mVertices[0] = getCornerMP(patch);
                triangles[triangleNum + 1].mVertices[1] = getCornerPM(patch);
                triangles[triangleNum + 1].mVertices[2] = getCornerMM(patch);
                triangleNum += 2;
                break;
            }
        }

        tierData.mTriangleNum = triangleNum;
    }

    for (s32 tier = 0; tier < cTierNum; tier++) {
        const TierData& tierData = rData.mTiers[tier];
        Vertex* vertices = getBufferPtr(rData.mRenderData.mVertexBlocks[tier][bufferIndex]);

        u32 vertexNum = 0;
        for (u32 i = 0; i < tierData.mTriangleNum; i++) {
            const Triangle& triangle = tierData.mTriangles[i];
            vertices[vertexNum] = triangle.mVertices[0];
            vertices[vertexNum + 1] = triangle.mVertices[1];
            vertices[vertexNum + 2] = triangle.mVertices[2];
            vertexNum += 3;
        }

        recordDisplayList(rData, tier, bufferIndex);
    }
}

/**
 * Flushes the CPU cache of the written geometry and display lists.
 * @param rData grid data
 * @param bufferIndex buffer to flush
 */
void FlushCPU(Data& rData, u32 bufferIndex) {
    for (s32 tier = 0; tier < cTierNum; tier++) {
        if (!rData.mTierViews[tier].mIsVisible) {
            continue;
        }

        const agl::DisplayList& displayList = rData.mRenderData.mDisplayLists[tier][bufferIndex];
        if (displayList.getValidSize() == 0) {
            continue;
        }

        u64 size = rData.mTiers[tier].mTriangleNum * sizeof(Triangle);
        rData.mRenderData.mVertexBuffers[tier][bufferIndex]->flushCPUCache(0, size);
        displayList.getBuffer().flushCPUCache(displayList.getValidSize());
    }
}

/**
 * Checks whether a tier was generated this frame.
 * @param rData grid data
 * @param tier tier to check
 * @return true if the tier is visible
 */
bool IsTierVisible(const Data& rData, u32 tier) {
    if (tier < cTierNum) {
        return rData.mTierViews[tier].mIsVisible;
    }

    return false;
}

/**
 * Gets the number of triangles of a tier.
 * @param rData grid data
 * @param tier tier to check
 * @return triangle count
 */
u32 TierTriCount(const Data& rData, u32 tier) {
    return rData.mTiers[tier].mTriangleNum;
}

/**
 * Draws one tier with its recorded display list.
 * @param rData grid data
 * @param tier tier to draw
 * @param bufferIndex buffer to draw
 */
void DisplayTier(const Data& rData, u32 tier, u32 bufferIndex) {
    const RenderData& renderData = rData.mRenderData;
    renderData.mVertexAttributes[tier][bufferIndex].activate(
        al::GameFrameworkNx::getAglDrawContext());
    nvnCommandBufferCallCommands(
        agl::driver::getNvnCommandBuffer(al::GameFrameworkNx::getAglDrawContext()), 1,
        renderData.mDisplayLists[tier][bufferIndex].getHandlePtr());
}

/**
 * Deletes the vertex buffers.
 * @param rData grid data
 */
void Free(Data& rData) {
    for (s32 tier = 0; tier < cTierNum; tier++) {
        for (s32 i = 0; i < cBufferNum; i++) {
            delete rData.mRenderData.mVertexBuffers[tier][i];
        }
    }
}

}  // namespace WaveGrid

#pragma once
#include <common/aglGPUMemBlock.h>
#include <basis/seadTypes.h>
#include <cstring>
#include <gfx/seadColor.h>
#include <math/seadVector.h>

/**
 * @brief A vertex of the ocean surface mesh, as written to the GPU vertex buffer.
 */
struct OceanVertex {
    sead::Vector3f mPos;       // 0x00
    sead::Vector3f mNormal;    // 0x0c
    sead::Vector2f mUV;        // 0x18
    sead::Color4f mColor;      // 0x20
    sead::Vector3f mTangent;   // 0x30
    sead::Vector3f mBinormal;  // 0x3c
};
static_assert(sizeof(OceanVertex) == 0x48);

/**
 * @brief A list of vertex indices grouped into runs of (start, length) pairs.
 * @tparam N Index capacity.
 */
template <u32 N>
struct TesselatorIndexStrip {
    /**
     * @brief Clear every index, run and the reuse counter.
     */
    void clear() { std::memset(this, 0, sizeof(*this)); }

    /**
     * @brief Replace this strip with a copy of another strip's indices and runs.
     * @param rOther Strip to copy.
     */
    void copy(const TesselatorIndexStrip& rOther) {
        clear();
        mNum = rOther.mNum;
        std::memcpy(mIndices, rOther.mIndices, mNum * sizeof(u32));
        mRunNum = rOther.mRunNum;
        std::memcpy(mRuns, rOther.mRuns, mRunNum * sizeof(u32));
    }

    /**
     * @brief Record a run of indices.
     * @param start First index of the run.
     * @param length Number of indices in the run.
     */
    void addRun(u32 start, u32 length) {
        mRuns[mRunNum++] = start;
        mRuns[mRunNum++] = length;
    }

    /**
     * @brief Append an index to the last run, opening a first run if there is none.
     * @param index Vertex index to append.
     */
    void push(u32 index) {
        mIndices[mNum++] = index;

        u32 last;

        if (mRunNum == 0) {
            mRunNum = 2;
            last = 1;
        } else {
            last = mRunNum - 1;
        }

        mRuns[last]++;
    }

    /**
     * @brief Append all indices of another strip as a new run.
     * @param rOther Strip whose indices are appended.
     */
    void append(const TesselatorIndexStrip& rOther) {
        mRuns[mRunNum++] = mNum;
        mRuns[mRunNum++] = rOther.mNum;
        std::memcpy(&mIndices[mNum], rOther.mIndices, rOther.mNum * sizeof(u32));
        mNum += rOther.mNum;
    }

    /**
     * @brief Read the start of a run.
     * @param run Run number.
     * @return The run's first index position, or 0 when the run does not exist.
     */
    u32 getRunStart(u32 run) const { return run * 2 < mRunNum ? mRuns[run * 2] : 0; }

    /**
     * @brief Read the length of a run.
     * @param run Run number.
     * @return The run's index count, or 0 when the run does not exist.
     */
    u32 getRunLength(u32 run) const { return run * 2 + 1 < mRunNum ? mRuns[run * 2 + 1] : 0; }

    /**
     * @brief Replace this strip with one run of another strip.
     * @param rOther Strip to read from.
     * @param run Run number to extract.
     */
    template <u32 M>
    void setRun(const TesselatorIndexStrip<M>& rOther, u32 run) {
        u32 start = rOther.getRunStart(run);
        u32 length = rOther.getRunLength(run);

        if (start + length > rOther.mNum) {
            return;
        }

        clear();
        mNum = length;
        std::memcpy(mIndices, &rOther.mIndices[start], length * sizeof(u32));
        addRun(0, length);
    }

    u32 mNum;
    u32 mIndices[N];
    u32 mRunNum;
    u32 mRuns[512];
    mutable u32 mReuseNum;
};

typedef TesselatorIndexStrip<20> TesselatorEdgeStrip;
typedef TesselatorIndexStrip<2048> TesselatorLineStrip;

/**
 * @brief Shared border state passed between neighbouring tessellation cells.
 */
class TesselatorData {
  public:
    TesselatorData();
    void reset();
    void incrementLine();
    void setX(float x);
    void setXBounds(float minX, float maxX, float step);
    float getX() const;
    float getMinX() const;
    float getMaxX() const;

    /**
     * @brief Copy the X bounds, step and line number of another cell.
     * @param rOther Cell to copy from.
     */
    void copyLineState(const TesselatorData& rOther) {
        mMinX = rOther.mMinX;
        mMaxX = rOther.mMaxX;
        mStep = rOther.mStep;
        mLine = rOther.mLine;
    }

    float getStep() const { return mStep; }
    s32 getLine() const { return mLine; }
    void setLine(s32 line) { mLine = line; }
    u32 getDepth() const { return mDepth; }
    void setDepth(u32 depth) { mDepth = depth; }

    TesselatorEdgeStrip mLeft;
    TesselatorEdgeStrip mRight;
    TesselatorLineStrip mTop;
    TesselatorLineStrip mBottom;

  private:
    s32 mLine;
    float mX;
    float mMinX;
    float mMaxX;
    float mStep;
    u8 _60e4[0x60f4 - 0x60e4];
    u32 mDepth;
};
static_assert(sizeof(TesselatorData) == 0x60f8);

/**
 * @brief Builds the ocean surface mesh by recursively subdividing quads.
 */
class Tesselator {
  public:
    /**
     * @brief Parameters of one subdivision request.
     */
    struct SubdivideData {
        sead::Vector3f mCorner0;
        sead::Vector3f mCorner1;
        sead::Vector3f mCorner2;
        sead::Vector3f mCorner3;
        u32 mLevel;
        float mX;
    };

    Tesselator();
    void prepare(agl::GPUMemBlock<OceanVertex>* pVertexBlock, agl::GPUMemBlock<u32>* pIndexBlock);
    void subdivide(const SubdivideData& rData);
    void subdivideImpl(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                       const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3, u32 depth,
                       TesselatorData& rOut, const TesselatorData& rIn, const SubdivideData& rData);
    void subdivide(const sead::Vector3f& rMin, const sead::Vector3f& rMax, u32 depth);
    void moveUp(float minX, float maxX);
    void subdivideImpl(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                       const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3, u32 depth,
                       u32 level, TesselatorData& rOut, const TesselatorData& rIn);
    void createQuad(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                    const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3,
                    TesselatorData& rOut, const TesselatorData& rIn);
    u32 createVertex(const sead::Vector3f& rPos);
    void createTriangle(u32 index0, u32 index1, u32 index2);
    void selectiveCopy(TesselatorData& rOut, const TesselatorData& rIn, float x, float minX,
                       float maxX, float step);

  private:
    void addIndex(u32 index);

    agl::GPUMemBlock<OceanVertex>* mVertexBlock;
    agl::GPUMemBlock<u32>* mIndexBlock;
    s32 mVertexNum;
    s32 mIndexNum;
    u8 _18[0x24 - 0x18];
    u32 _24;
    u32 _28;
    u32 _2c;
    u8 _30[0x34 - 0x30];
    TesselatorData mData;
};
static_assert(sizeof(Tesselator) == 0x6130);

bool isCollision(const sead::Vector3f& rPos, float radius, const sead::Vector3f& rCorner0,
                 const sead::Vector3f& rCorner1, const sead::Vector3f& rCorner2,
                 const sead::Vector3f& rCorner3);

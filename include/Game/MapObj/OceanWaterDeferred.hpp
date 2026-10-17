#pragma once

#include <basis/seadTypes.h>
#include <common/aglDisplayList.h>
#include <common/aglGPUMemBlock.h>
#include <common/aglIndexStream.h>
#include <common/aglTextureData.h>
#include <common/aglTextureDataImageAccessor.h>
#include <common/aglVertexAttribute.h>
#include <container/seadBuffer.h>
#include <container/seadOrderedSet.h>
#include <container/seadPriorityQueue.h>
#include <gfx/seadColor.h>
#include <math/seadVector.h>
#include <thread/seadMessageQueue.h>
#include <thread/seadMutex.h>

#include "MapObj/Ocean.hpp"
#include "MapObj/OceanData.hpp"
#include "MapObj/OceanWater.hpp"
#include "MapObj/OceanWaveGenerator.hpp"
#include "Project/OceanWave/OceanWaveDirector.hpp"
#include "System/Data/TesselatorData.hpp"

namespace agl {
class RenderBuffer;
class ShaderProgram;
class VertexAttribute;
class VertexBuffer;
}  // namespace agl

namespace al {
class ActorInitInfo;
class AreaObj;
class FullScreenQuadModel;
class HitSensor;
class IUseSceneObjHolder;
class SensorMsg;
class UniformBlock;
}  // namespace al

namespace sead {
class DelegateThread;
class Thread;
}  // namespace sead

/**
 * @brief A square patch of the ocean height field that follows the sea center.
 */
struct WavePatch {
    f32 mOriginX;                                 // 0x00
    f32 mOriginZ;                                 // 0x04
    f32 mCenterX;                                 // 0x08
    f32 mCenterZ;                                 // 0x0c
    s64 mX;                                       // 0x10
    s64 mZ;                                       // 0x18
    s64 mSize;                                    // 0x20
    s64 mPointNum;                                // 0x28
    s64 mMinX;                                    // 0x30
    s64 mMaxX;                                    // 0x38
    s64 mMinZ;                                    // 0x40
    s64 mMaxZ;                                    // 0x48
    agl::GPUMemBlock<OceanVertex>* mVertexBlock;  // 0x50
    s64 _58;                                      // 0x58
    s64 _60;                                      // 0x60
};

static_assert(sizeof(WavePatch) == 0x68);

/**
 * @brief Ocean surface whose height field is drawn into the deferred G-buffer, with ink areas
 * painted from a lookup texture.
 */
class OceanWaterDeferred : public OceanWater {
  public:
    /**
     * @brief One texel of the ink lookup: ink mask, wave blend and base height.
     */
    struct InkSample {
        u16 mInkBlend : 8;
        u16 mHeight : 8;

        /**
         * @brief Get the ink mask, 0xf where there is no ink.
         * @return The ink mask.
         */
        u32 getInk() const { return mInkBlend & 0xf; }

        /**
         * @brief Get the blend between the sea and the raised ground height.
         * @return The blend, from 0 to 15.
         */
        u32 getBlend() const { return (mInkBlend >> 4) & 0xf; }

        /**
         * @brief Set the ink mask.
         * @param ink ink mask, from 0 to 15
         */
        void setInk(u32 ink) { mInkBlend = (mInkBlend & 0xf0) | (ink & 0xf); }

        /**
         * @brief Set the blend between the sea and the raised ground height.
         * @param blend blend, from 0 to 15
         */
        void setBlend(u32 blend) { mInkBlend = (mInkBlend & 0xf) | (blend << 4); }
    };

    static_assert(sizeof(InkSample) == 2);

    OceanWaterDeferred(const char* pName);
    ~OceanWaterDeferred() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    bool loadInkArea(const char* pTextureName, s32 phase, const char* pDataName);
    void movement() override;
    bool isInInk(const sead::Vector3f& rPos) override;
    const InkSample* getInkSample(s32 x, s32 y) const;
    virtual InkSample sampleBaseHeight(const sead::Vector3f& rPos);
    InkSample sampleTexture(const sead::Vector3f& rPos) const;
    f32 getWaterHeight(const sead::Vector3f& rPos, bool isIgnoreWave) override;
    void draw() const override;
    void drawCopyBuffers() const;
    bool buildSampleHeightLookup();

    /**
     * @brief Get the render type of the ocean.
     * @return Always 1.
     */
    s32 getRenderType() const override { return 1; }

  private:
    void* _289c88;                                   // 0x289c88
    const agl::ShaderProgram* mCopyTexturesShader;   // 0x289c90
    u32 mInkHeight = 0;                              // 0x289c98
    u32 mInkWidth = 0;                               // 0x289c9c
    InkSample* mInkSamples = nullptr;                // 0x289ca0
    agl::TextureData mInkTexture;                    // 0x289ca8
    agl::TextureDataImageAccessor mInkAccessor;      // 0x289dd0
    bool mIsInkLookupBuilt = false;                  // 0x28a150
    bool mIsInkTextureLoaded = false;                // 0x28a151
    sead::Mutex mInkMutex;                           // 0x28a158
    al::AreaObj* mInkArea = nullptr;                 // 0x28a198
    bool mIsShaderInk = false;                       // 0x28a1a0
};

/**
 * @brief Older ocean surface that simulates its own wave patches and draws them with indirect
 * (screen space) reflections.
 */
class OceanWaterIndirect : public al::OceanWaveDirector {
  public:
    /**
     * @brief A cell of the visible grid, ordered by its distance to the camera.
     */
    struct GridPoint {
        s32 mX;
        s32 mZ;
        u8 mLevel;

        /**
         * @brief Copy the position and the level of another grid point.
         * @param rOther grid point to copy
         * @return This grid point.
         */
        GridPoint& operator=(const GridPoint& rOther) {
            mX = rOther.mX;
            mZ = rOther.mZ;
            mLevel = rOther.mLevel;
            return *this;
        }

        /**
         * @brief Get the key the visited set is ordered by.
         * @return The key.
         */
        s32 getKey() const { return mX + mZ * 1000; }

        /**
         * @brief Order grid points by position, for the visited set.
         * @param rOther grid point to compare with
         * @return Whether this grid point comes first.
         */
        bool operator<(const GridPoint& rOther) const { return getKey() < rOther.getKey(); }

        /**
         * @brief Order grid points by row, for the queue.
         * @param rOther grid point to compare with
         * @return Whether this grid point comes after the other one.
         */
        bool operator>(const GridPoint& rOther) const { return mZ > rOther.mZ; }
    };

    static_assert(sizeof(GridPoint) == 0xc);

    /**
     * @brief Queue of grid cells still to visit.
     */
    class GridPointQueue : public sead::FixedPriorityQueue<GridPoint, 10240> {};

    /**
     * @brief A run of grid columns covered by the round wave patch on one row.
     */
    struct RowSpan {
        s64 mStart;
        s64 mLength;
    };

    /**
     * @brief Indices of the grid points on the border of the round wave patch.
     */
    struct BorderList {
        s64 mIndices[960];
        s64 mNum;
    };

    static OceanWaterIndirect* getOceanWater(const al::IUseSceneObjHolder* pHolder);

    OceanWaterIndirect(const char* pName);
    void initWavePatch();
    ~OceanWaterIndirect() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void threadFunc_(sead::Thread* pThread, s64 message);
    void setSeaCenterPosition(long unused, f32* pX, f32* pZ, f32* pDirX, f32* pDirZ);
    void WavePatchSetPosition(long index, f32 x, f32 z, f32 dirX, f32 dirZ);
    void updateWavePatch();
    void updateWaterSimultion(f32 rate);
    void initAfterPlacement() override;
    void initFromYaml(const al::ByamlIter& rIter, const char* pName) override;
    void initModeFromYaml(const char* pName, OceanData& rData, const al::ByamlIter& rIter);
    void initColorFromYaml(const char* pName, sead::Color4f& rColor, const al::ByamlIter& rIter);
    void initMaterialFromYaml(const char* pName, sead::Vector2f& rUV, sead::Vector2f& rUVScroll,
                              const al::ByamlIter& rIter);
    void initMaterialFromYaml(const char* pName, sead::Vector4f& rUV, f32& rUVRot,
                              f32& rIntensity, const al::ByamlIter& rIter);
    void initRefractionFromYaml(const char* pName, sead::Color4f& rColor, f32& rFadeHeight,
                                const al::ByamlIter& rIter);
    void initReflectionFromYaml(const char* pName, OceanData& rData, const al::ByamlIter& rIter);
    bool createWave(const al::LiveActor* pActor, const al::OceanWaveInfo* pInfo) override;
    f32 getY(const sead::Vector3f& rPos) override;
    void kill() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void draw() const override;
    void drawNormals() const;
    void drawReflections() const;
    void drawCompose(const agl::RenderBuffer* pRenderBuffer) const;
    void drawDeffered() const;
    void bindUniformBlock(al::UniformBlock* pBlock) const;
    void bindTextures(const agl::ShaderProgram* pProgram) const;
    void movement() override;
    void setupVertices(f32 minX, f32 minZ, f32 maxX, f32 maxZ);
    void setupIndices(f32 sizeX, f32 sizeZ);
    void wavePatchCalcXZ(WavePatch* pPatch);
    void waveOceanUpdateWavePatch(WavePatch* pPatch);
    void waveGenUpdateWavePatch(WavePatch* pPatch);
    void wavePatchCalcNormal(WavePatch* pPatch);
    void wavePatchExtendBorder(WavePatch* pPatch);
    void setupStreams(agl::VertexBuffer* pBuffer, agl::VertexAttribute* pAttribute);
    static void setupStrip(agl::GPUMemBlock<u32>* pBlock, agl::IndexStream* pStream, u32 start,
                           u32 nextStart, s32 length);
    void calculateNormals(agl::GPUMemBlock<OceanVertex>& rVertices);
    void calculateUVs();
    void updateWaterPatchXZ();
    s32 markGridPoints(GridPointQueue& rQueue, const sead::Vector3f& rPos,
                        const sead::Vector3f& rDir, const f32& rNear, const f32& rFar,
                        const sead::Vector3f& rMin, const sead::Vector3f& rMax, u8 level);
    void addQuad(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                 const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3, bool isEdge,
                 s32* pEdgeIndices, u32* pIndices, u32& rIndexNum, u32* pIndices2,
                 u32& rIndexNum2, u32& rIndexNum3, s32* pEdgeIndices2, u32 level, s32 side,
                 u32* pIndices3);
    u32 addVertex(const sead::Vector3f& rPos, const sead::Color4f& rColor);
    void addTriangle(u32 index0, u32 index1, u32 index2);
    void subdivide(const sead::Vector3f& rCorner0, const sead::Vector3f& rCorner1,
                   const sead::Vector3f& rCorner2, const sead::Vector3f& rCorner3,
                   u32* pIndices, u8 maxDepth, u8 depth);
    void findAllFrustumCells();
    void waveWakeUpdateWavePatch(WavePatch* pPatch);
    void wavePatchCalcNormal_new(WavePatch* pPatch);
    f32 calculateLerp() const;

    /**
     * @brief Get the name of the scene object.
     * @return The scene object name.
     */
    const char* getSceneObjName() const override { return "OceanWaterIndirect"; }

    /**
     * @brief Get the render type of the ocean.
     * @return Always 0.
     */
    s32 getRenderType() const override { return 0; }

    /**
     * @brief Set the stage the ocean is in (unused).
     * @param pStageName stage name
     */
    void setStageName(const char* pStageName) override {}

  private:
    void addIndex(u32 index);

    RowSpan mRowSpans[120];                                      // 0x158
    BorderList mBorders[1];                                      // 0x8d8
    WavePatch mWavePatches[4];                                   // 0x26e0
    f32 mNormalScales[64][64];                                   // 0x2880
    u32 mWavePatchNum = 0;                                       // 0x6880
    agl::DisplayList mDisplayList;                               // 0x6888
    agl::GPUMemBlock<u8> mCommandBlock;                          // 0x6ae8
    sead::Buffer<agl::IndexStream> mStripStreams;                // 0x6b20
    sead::Buffer<agl::GPUMemBlock<u32>> mStripBlocks;            // 0x6b30
    agl::GPUMemBlock<OceanVertex> mGridVertexBlocks[3];          // 0x6b40
    agl::VertexBuffer* mGridVertexBuffers[3];                    // 0x6be8
    agl::VertexAttribute mGridVertexAttributes[3];               // 0x6c00
    agl::GPUMemBlock<u32> mIndexBlocks[3];                       // 0x71a0
    agl::IndexStream mIndexStreams[3];                           // 0x7248
    s32 mVertexNums[3];                                          // 0x7368
    s32 mIndexNums[3];                                           // 0x7374
    al::UniformBlock* mUniformBlock;                             // 0x7380
    f32 mReflectionRayMaxStep;                                   // 0x7388
    sead::Vector2f mReflectionRayScale;                          // 0x738c
    bool mIsGaussianBlur;                                        // 0x7394
    agl::GPUMemBlock<OceanVertex> mVertexBlocks[3];              // 0x7398
    agl::VertexBuffer* mVertexBuffers[3];                        // 0x7440
    agl::VertexAttribute mVertexAttributes[3];                   // 0x7458
    u32 mBufferIndex = 0;                                        // 0x79f8
    agl::GPUMemBlock<OceanVertex> mMeshVertexBlocks[3];          // 0x7a00
    agl::VertexBuffer* mMeshVertexBuffers[3];                    // 0x7aa8
    agl::VertexAttribute mMeshVertexAttributes[3];               // 0x7ac0
    s32 mFresnelType;                                            // 0x8060
    u32 _8064;                                                   // 0x8064
    OceanData mModes[2];                                         // 0x8068
    mutable bool mIsLerping;                                     // 0x8298
    mutable u32 mFromModeIndex;                                  // 0x829c
    mutable u32 mToModeIndex;                                    // 0x82a0
    sead::DelegateThread* mThread;                               // 0x82a8
    u32 mThreadBufferIndex = 0;                                  // 0x82b0
    sead::MessageQueue mRequestQueue;                            // 0x82b8
    sead::MessageQueue mResultQueue;                             // 0x8308
    const agl::VertexAttribute* mCurrentVertexAttributes;        // 0x8358
    u32 _8360 = 0;                                               // 0x8360
    Ocean mOcean;                                                // 0x8368
    OceanWaveGenerator mGenerator;                               // 0x54c50
    s32 mPolygonModeFront;                                       // 0x55698
    s32 mPolygonModeBack;                                        // 0x5569c
    s32 _556a0;                                                  // 0x556a0
    u32 mWaveMode;                                               // 0x556a4
    s32 mReflectionType;                                         // 0x556a8
    agl::ShaderProgram* mShader;                                 // 0x556b0
    agl::ShaderProgram* mNormalsShader;                          // 0x556b8
    agl::ShaderProgram* mReflectionsShader;                      // 0x556c0
    agl::ShaderProgram* mComposeShader;                          // 0x556c8
    agl::ShaderProgram* mDeferredShader;                         // 0x556d0
    al::FullScreenQuadModel* mQuadModel;                         // 0x556d8
    OceanWater::RenderMode mRenderMode;                          // 0x556e0
    agl::TextureData mTextures[4];                               // 0x556e8
    f32 mSeaY = 0.0f;                                            // 0x55b88
    s16 mSeaOffset;                                              // 0x55b8c
    u32 mFrame;                                                  // 0x55b90
    al::LiveActor* mPlayer;                                      // 0x55b98
    GridPointQueue mGridQueue;                                   // 0x55ba0
    sead::FixedOrderedSet<GridPoint, 10240> mVisitedGridPoints;  // 0x73bb0
    u8 _ffbd0[0xffbe8 - 0xffbd0];
    Tesselator mTesselator;                                      // 0xffbe8
};


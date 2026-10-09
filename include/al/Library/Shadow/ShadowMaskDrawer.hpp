#pragma once

#include <attributes.h>
#include <basis/seadTypes.h>
#include <container/seadBuffer.h>
#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>
#include <gfx/seadColor.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include <agl/common/aglRenderBuffer.h>
#include <agl/common/aglRenderTarget.h>
#include <agl/common/aglShaderEnum.h>
#include <agl/common/aglTextureSampler.h>
#include <agl/common/aglVertexAttribute.h>
#include <agl/utility/aglParameter.h>
#include <agl/utility/aglParameterObj.h>

#include "Library/Execute/IUseExecutor.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"
#include "Project/Draw/GraphicsParamKeeper.hpp"

namespace agl {
class ShaderProgram;
class TextureData;
class UniformBlock;
}  // namespace agl

namespace sead {
class FixedSizeJQ;
class WorkerMgr;
}  // namespace sead

namespace al {
class ExecuteDirector;
class GBufferArray;
class GraphicsSystemInfo;
class LiveActor;
class ShaderHolder;
class ShadowMaskBase;
class ShadowMaskKeeper;
class UniformBlock;

class ShadowMaskDrawer : public IUseExecutor {
public:
    NOINLINE ShadowMaskDrawer(ShadowMaskKeeper* pKeeper, s32 drawCategory,
                              ExecuteDirector* pDirector);

    void execute() override { mGBufferArray = nullptr; }

    void draw() const override;
    agl::ShaderMode drawToTextureData(const agl::TextureData* pTarget,
                                      const agl::TextureData* pLightBuffer,
                                      agl::ShaderMode mode) const;
    NOINLINE agl::ShaderMode drawLightScaleToAlbedoGBuffer(agl::ShaderMode mode) const;

    ShadowMaskKeeper* mKeeper;
    s32 mDrawCategory;
    s32 mViewIndex = 0;
    const sead::Matrix34f* mViewMtx = nullptr;
    const sead::Matrix44f* mProjMtx = nullptr;
    const agl::ShaderProgram* mShaderProgram = nullptr;
    GBufferArray* mGBufferArray = nullptr;
    agl::TextureData* mLinearDepthTexture = nullptr;
    agl::TextureData* mColorTexture = nullptr;
};

static_assert(sizeof(ShadowMaskDrawer) == 0x48);

class ShadowMaskParam : public agl::utl::IParameterObj {
public:
    void init();
    bool operator==(const ShadowMaskParam& rOther) const;
    ShadowMaskParam& operator=(const ShadowMaskParam& rOther);
    void interp(const ShadowMaskParam& rParamA, const ShadowMaskParam& rParamB, f32 rate);

    agl::utl::IParameterObj* getParamObj() { return this; }

    s32 getBlockIntensity() const { return *mBlockIntensity; }

    s32 getItemIntensity() const { return *mItemIntensity; }

    s32 getMapObjIntensity() const { return *mMapObjIntensity; }

    s32 getEnemyIntensity() const { return *mEnemyIntensity; }

    s32 getPlayerIntensity() const { return *mPlayerIntensity; }

private:
    agl::utl::Parameter<s32> mBlockIntensity;
    agl::utl::Parameter<s32> mItemIntensity;
    agl::utl::Parameter<s32> mMapObjIntensity;
    agl::utl::Parameter<s32> mEnemyIntensity;
    agl::utl::Parameter<s32> mPlayerIntensity;
};

static_assert(sizeof(ShadowMaskParam) == 0xd0);

/**
 * Collects the shadow mask primitives of a scene every frame and draws them into the G-buffers.
 */
class ShadowMaskKeeper : public GraphicsParamRequestInterpKeeper<ShadowMaskParam> {
public:
    static constexpr s32 cCategoryNum = 17;

    enum PrimType : s32 {
        cPrimType_Sphere = 0,
        cPrimType_Cylinder = 1,
        cPrimType_InterpolateCube = 2,
        cPrimType_Cube = 3,
        cPrimType_OvalCylinder = 4,
    };

    struct PrimInfo {
        PrimInfo() {}

        PrimType mType = cPrimType_Sphere;
        sead::Vector3f _4 = sead::Vector3f::zero;
        sead::Vector3f _10 = {100.0f, 100.0f, 100.0f};
        sead::Vector3f _1c = sead::Vector3f::zero;
        sead::Matrix34f mMtx;
        sead::Color4f mColor = sead::Color4f::cWhite;
        sead::Vector3f mExp = {2.0f, 2.0f, 2.0f};
        f32 _74;
        f32 mDistYBase;
        s32 _7c;
        s32 mCategory = 0;
        f32 mIntensity = 0.0f;
        agl::TextureSampler* mSampler = nullptr;
        sead::Vector2f mTexOffset = {0.0f, 0.0f};
        sead::Vector2f mTexScale = {0.0f, 0.0f};
        sead::Vector2f mTexRange = {1.0f, 1.0f};
        s32 mTexType = 1;
        s32 mIsTexFlip = false;
    };

    static_assert(sizeof(PrimInfo) == 0xb0);

    struct Context {
        UniformBlock* mUniformBlock;
        agl::TextureSampler mLinearDepthSampler;
        agl::TextureSampler mColorSampler;
        agl::RenderTargetColor mRenderTarget;
        agl::RenderBuffer mRenderBuffer;
        const agl::TextureData* mTexture;
    };

    static_assert(sizeof(Context) == 0x4d0);

    typedef sead::ObjArray<PrimInfo> PrimInfoArray;
    typedef sead::PtrArray<UniformBlock> UniformBlockArray;
    typedef sead::Buffer<UniformBlockArray> UniformBlockBuffer;

    ShadowMaskKeeper(u8 bufferNum, GraphicsSystemInfo* pInfo);
    ~ShadowMaskKeeper();

    void updateMultiCore(s32 coreIndex);
    void updateGpuBufferMultiCore(s32 coreIndex);
    u8 getShadowIntensity(s32 category) const;
    void endInit();
    void initialize(u32 primNum, u8 bufferNum);
    void clear();
    void updateShadowMask();
    void setUniformSphere(agl::UniformBlock* pBlock, const PrimInfo& rInfo,
                          const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);
    void setUniformCylinder(agl::UniformBlock* pBlock, const PrimInfo& rInfo,
                            const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);
    void setUniformCube(agl::UniformBlock* pBlock, const PrimInfo& rInfo,
                        const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);
    void registerShadowMask(ShadowMaskBase* pMask);
    void removeShadowMask(ShadowMaskBase* pMask);
    void init(ExecuteDirector* pDirector);
    void initShader(ShaderHolder* pShaderHolder);
    void addSphere(const sead::Matrix34f& rMtx, const sead::Color4f& rColor, f32 exp, f32 intensity,
                   s32 category);
    void addCylinder(const sead::Matrix34f& rMtx, const sead::Color4f& rColor, f32 expXZ, f32 expY,
                     f32 intensity, f32 distYBase, s32 category);
    void addCastOvalCylinder(const sead::Matrix34f& rMtx, const sead::Color4f& rColor, f32 expXZ,
                             f32 expY, f32 intensity, f32 distYBase, s32 category);
    void addCube(const sead::Matrix34f& rMtx, const sead::Color4f& rColor,
                 const sead::Vector3f& rExp, f32 intensity, f32 distYBase, s32 category,
                 agl::TextureSampler* pSampler, bool isCube, const sead::Vector2f& rTexOffset,
                 const sead::Vector2f& rTexScale, const sead::Vector2f& rTexRange, s32 texType,
                 bool isTexFlip);
    void addCastInterpolateCube(const sead::Matrix34f& rMtx, const sead::Color4f& rColor,
                                const sead::Vector3f& rExp, f32 intensity, f32 distYBase,
                                s32 category);
    void setUniformCommon(s32 bufferIndex, s32 width, s32 height);
    void updateGpuBuffer(s32 bufferIndex, const sead::Matrix34f* pViewMtx,
                         const sead::Matrix44f* pProjMtx);
    void flush(s32 bufferIndex, bool isForce);
    void swapGpuBuffer();
    agl::ShaderMode drawPrimSphere(s32 bufferIndex, Context& rContext,
                                   const agl::ShaderProgram* pProgram,
                                   const agl::TextureData& rDepth, const sead::Matrix34f& rViewMtx,
                                   const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                   s32 category);
    agl::ShaderMode drawPrimCylinder(s32 bufferIndex, Context& rContext,
                                     const agl::ShaderProgram* pProgram,
                                     const agl::TextureData& rDepth,
                                     const sead::Matrix34f& rViewMtx,
                                     const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                     s32 category);
    agl::ShaderMode drawPrimCube(s32 bufferIndex, Context& rContext,
                                 const agl::ShaderProgram* pProgram,
                                 const agl::TextureData& rDepth, const sead::Matrix34f& rViewMtx,
                                 const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                 s32 category);
    agl::ShaderMode drawPrimTexCube(s32 bufferIndex, Context& rContext,
                                    const agl::ShaderProgram* pProgram,
                                    const agl::TextureData& rDepth,
                                    const sead::Matrix34f& rViewMtx,
                                    const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                    s32 category, bool isTex2, s32 dumpType);
    agl::ShaderMode drawPrimTex2Cube(s32 bufferIndex, Context& rContext,
                                     const agl::ShaderProgram* pProgram,
                                     const agl::TextureData& rDepth,
                                     const sead::Matrix34f& rViewMtx,
                                     const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                     s32 category);
    agl::ShaderMode drawPrimTex2CubeDump(s32 bufferIndex, Context& rContext,
                                         const agl::ShaderProgram* pProgram,
                                         const agl::TextureData& rDepth,
                                         const sead::Matrix34f& rViewMtx,
                                         const sead::Matrix44f& rProjMtx, agl::ShaderMode mode,
                                         s32 category);
    void setViewInfo(s32 viewIndex, GBufferArray* pGBufferArray, const sead::Matrix34f* pViewMtx,
                     const sead::Matrix44f* pProjMtx);
    void overrideLinearDepthTexture(agl::TextureData* pTexture);
    void overrideColorTexture(agl::TextureData* pTexture);
    bool isExistDrawShadowMask() const;
    NOINLINE bool isExistDrawShadowMaskCategory(s32 category) const;
    agl::ShaderMode draw(s32 bufferIndex, const agl::ShaderProgram* pProgram,
                         const agl::TextureData& rDepth, const sead::Matrix34f& rViewMtx,
                         const sead::Matrix44f& rProjMtx, agl::ShaderMode mode, s32 category);
    agl::ShaderMode drawLightScaleToAlbedo(agl::ShaderMode mode) const;

    void declare(s32 type, ShadowMaskDrawCategory category) {
        mDeclareCount[type - 1][category]++;
    }

    s32 getRenderType() const { return mRenderType; }

    bool isExistPrim(s32 category) const {
        return mSpherePrimInfos[category].size() > 0 || mCylinderPrimInfos[category].size() > 0 ||
               mCubePrimInfos[category].size() > 0;
    }

    GraphicsSystemInfo* mSystemInfo;
    sead::PtrArray<ShadowMaskDrawer> mDrawers;
    ShadowMaskArray mEvenTargetMasks;
    ShadowMaskArray mMasks;
    PrimInfoArray mSpherePrimInfos[cCategoryNum];
    PrimInfoArray mCylinderPrimInfos[cCategoryNum];
    PrimInfoArray mCubePrimInfos[cCategoryNum];
    UniformBlockBuffer mSphereBlocks[cCategoryNum];
    UniformBlockBuffer mCylinderBlocks[cCategoryNum];
    UniformBlockBuffer mCubeBlocks[cCategoryNum];
    s32 mPrimNum = -1;
    sead::Buffer<Context> mContexts;
    agl::VertexAttribute mSphereAttribute;
    agl::VertexAttribute mCylinderAttribute;
    agl::VertexAttribute mCubeAttribute;
    f32 mShadowRate = 0.5f;
    s32 mDeclareCount[5][cCategoryNum];
    u8 mBufferNum;
    s32 mGpuBufferIndex = 0;
    bool mIsDrawIntensity = true;
    s32 mRenderType = 0;
    s32 mEvenTargetMaskLastIndex = -1;
    s32 mMaskLastIndex = -1;
    sead::WorkerMgr* mWorkerMgr = nullptr;
    sead::FixedSizeJQ* mUpdateJobQueue = nullptr;
    sead::FixedSizeJQ* mGpuBufferJobQueue = nullptr;
    s32 mUpdateBufferIndex = 0;
    const sead::Matrix34f* mViewMtx = nullptr;
    const sead::Matrix44f* mProjMtx = nullptr;
};

static_assert(sizeof(ShadowMaskKeeper) == 0x1790);

}  // namespace al

namespace ShadowMaskFunction {
al::ShadowMaskKeeper* getShadowMaskKeeper(const al::LiveActor* pActor);
}

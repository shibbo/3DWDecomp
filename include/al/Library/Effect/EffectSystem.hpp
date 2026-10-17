#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <nn/vfx/Heap.h>

#include "Library/Effect/EffectSystemInfo.hpp"

namespace agl {
class DrawContext;
class TextureData;
}  // namespace agl

namespace agl::sdw {
class DepthShadow;
}  // namespace agl::sdw

namespace sead {
class Heap;
class Viewport;
}  // namespace sead

namespace al {
class CameraDirector;
class EffectCameraHolder;
struct EffectDrawCategoryInfo;
class EffectEnvParam;
class EffectGroupDrawer;
class EffectHeap;
class EffectLayoutDrawer;
class EffectShaderHolder;
class ExecuteDirector;
class GraphicsSystemInfo;

class EffectSystem {
public:
    static EffectSystem* createSystem(agl::DrawContext* pDrawContext, sead::Heap* pHeap);
    static EffectSystem* createSystemWithPatchResouce(agl::DrawContext* pDrawContext,
                                                      sead::Heap* pHeap);
    static void loadEffectResource(EffectSystem* pEffectSystem);
    static EffectSystem* initializeSystem(agl::DrawContext* pDrawContext, sead::Heap* pHeap,
                                          bool isDelayLoadResource);
    static EffectSystem* initializeSystemWithPatchResource(agl::DrawContext* pDrawContext,
                                                           sead::Heap* pHeap,
                                                           bool isDelayLoadResource);

    EffectSystem();

    void loadPtclResource(sead::Heap* pHeap);
    static bool isEnableBatchCompute();
    static s32 getPauseForceCalcFrame();
    void setDrawContext(agl::DrawContext* pDrawContext);
    void addResourcePath(const char* pPath);
    void init();
    void loadDbResource(sead::Heap* pHeap);
    void initScene();
    void endInit();
    void startScene(ExecuteDirector* pExecuteDirector);
    void preprocess();
    void postprocess();
    void endScene();
    void setCameraDirector(CameraDirector* pCameraDirector);
    void calcParticle(u64 userData);
    void setGraphicsSystemInfo(const GraphicsSystemInfo* pGraphicsSystemInfo);
    void updateEffect(const char* pGroupName) const;
    EffectGroupDrawer* findGroupDrawer(const char* pGroupName) const;
    void calcEffectCompute() const;
    void drawEffectWithRenderPathAndCamPos(const sead::Matrix44f& rProjMtx,
                                           const sead::Matrix34f& rViewMtx,
                                           const sead::Vector3f& rCamPos, f32 near, f32 far,
                                           f32 fovy, const char* pGroupName, u32 renderPath,
                                           bool isCalcCompute) const;
    void drawEffectWithRenderPath(const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                                  f32 near, f32 far, f32 fovy, const char* pGroupName,
                                  u32 renderPath, bool isCalcCompute) const;
    void calcShadowClipVolume(agl::sdw::DepthShadow* pDepthShadow, const char* pGroupName,
                              u32 renderPath) const;
    void addCalcEffect(u64 userData);
    bool isHasRenderingEmitter(u32 flag) const;
    void checkCalculateFlag(s32 groupId);
    void calcParticle(s32 groupId);
    void calcChildParticle(s32 groupId);

    EffectSystemInfo* getEffectSystemInfo() { return &mEffectSystemInfo; }
    const EffectSystemInfo* getEffectSystemInfo() const { return &mEffectSystemInfo; }
    PtclSystem* getPtclSystem() const { return mEffectSystemInfo.mPtclSystem; }
    EffectShaderHolder* getShaderHolder() const { return mShaderHolder; }
    EffectCameraHolder* getEffectCameraHolder() const { return mEffectCameraHolder; }
    agl::DrawContext* getDrawContext() const { return mDrawContext; }
    EffectEnvParam* getEffectEnvParam() const { return mEffectEnvParam; }
    s32 getGroupDrawerNum() const { return mGroupDrawerNum; }
    EffectGroupDrawer* getGroupDrawer(s32 index) const { return mGroupDrawers[index]; }
    void setStopCalc(bool stopped) { mIsStopCalc = stopped; }
    bool isStopCalc() const { return mIsStopCalc || mIsStopCalcByDemo; }
    bool isSwapBuffer() const { return mIsSwapBuffer; }

private:
    void calcParticleImpl(u64 userData);
    inline const EffectDrawCategoryInfo& getDrawCategory(s32 index) const;

    sead::Heap* mHeap;
    EffectSystemInfo mEffectSystemInfo;
    EffectCameraHolder* mEffectCameraHolder;
    s32 mGroupDrawerNum;
    EffectGroupDrawer** mGroupDrawers;
    s32 mResourcePathNum;
    s32 mResourceIndex;
    const char* mResourcePaths[6];
    bool mIsLoadedPtclResource;
    bool mIsStopCalc;
    bool mIsStopCalcByDemo;
    bool mIsSwapBuffer;
    s32 mCalcEffectNum;
    u64 mCalcEffects[0x60];
    s32 mLayoutDrawerNum;
    EffectLayoutDrawer** mLayoutDrawers;
    EffectShaderHolder* mShaderHolder;
    void* _398;
    void* _3a0;
    void* _3a8;
    EffectHeap* mEffectHeap;
    agl::DrawContext* mDrawContext;
    EffectEnvParam* mEffectEnvParam;
    u32 mCalculateFlag;
};

static_assert(sizeof(EffectSystem) == 0x3d0);
}  // namespace al

class EffectHeapForNw : public nn::vfx::Heap {
public:
    explicit EffectHeapForNw(sead::Heap* pHeap) : mHeap(pHeap) {}

    ~EffectHeapForNw() override;
    void* Alloc(size_t size, size_t alignment) override;
    void Free(void* ptr) override;

private:
    sead::Heap* mHeap;
};

namespace alEffectSystemFunction {
void setDrawPathRenderStateSetCallbackSRT(const al::EffectSystem* pEffectSystem, bool isEnable);
void calcEffectCompute(const al::EffectSystem* pEffectSystem);
void setDrawPathRenderStateSetCallbackMRT(const al::EffectSystem* pEffectSystem);
void drawEffectDeferredShadowMask(const al::EffectSystem* pEffectSystem,
                                  const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                                  f32 near, f32 far, f32 fovy);
void drawEffectDeferredShadowMaskWithPos(const al::EffectSystem* pEffectSystem,
                                         const sead::Matrix44f& rProjMtx,
                                         const sead::Matrix34f& rViewMtx,
                                         const sead::Vector3f& rCamPos, f32 near, f32 far,
                                         f32 fovy);
void drawEffectDeferredShadowMaskPlayer(const al::EffectSystem* pEffectSystem,
                                        const sead::Matrix44f& rProjMtx,
                                        const sead::Matrix34f& rViewMtx, f32 near, f32 far,
                                        f32 fovy);
void drawEffectDeferred(const al::EffectSystem* pEffectSystem, const sead::Matrix44f& rProjMtx,
                        const sead::Matrix34f& rViewMtx, f32 near, f32 far, f32 fovy);
void drawEffectDeferredWithPos(const al::EffectSystem* pEffectSystem,
                               const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                               const sead::Vector3f& rCamPos, f32 near, f32 far, f32 fovy);
void drawEffectDeferredPlayer(const al::EffectSystem* pEffectSystem,
                              const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                              f32 near, f32 far, f32 fovy);
void drawEffectForward(const al::EffectSystem* pEffectSystem, const sead::Matrix44f& rProjMtx,
                       const sead::Matrix34f& rViewMtx, f32 near, f32 far, f32 fovy);
void drawEffectForwardPlayer(const al::EffectSystem* pEffectSystem,
                             const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                             f32 near, f32 far, f32 fovy);
void drawEffectForwardAfterFog(const al::EffectSystem* pEffectSystem,
                               const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                               f32 near, f32 far, f32 fovy);
void drawEffectForwardReduced(const al::EffectSystem* pEffectSystem,
                              const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                              f32 near, f32 far, f32 fovy);
void drawEffectForwardReducedHDR(const al::EffectSystem* pEffectSystem,
                                 const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                                 f32 near, f32 far, f32 fovy);
void drawEffectIndirect(const al::EffectSystem* pEffectSystem, const sead::Matrix44f& rProjMtx,
                        const sead::Matrix34f& rViewMtx, f32 near, f32 far, f32 fovy,
                        const sead::Viewport* pViewport);
void drawEffectIndirectPlayer(const al::EffectSystem* pEffectSystem,
                              const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                              f32 near, f32 far, f32 fovy, const sead::Viewport* pViewport);
void drawEffectPostEffectBackground(const al::EffectSystem* pEffectSystem,
                                    const sead::Matrix44f& rProjMtx,
                                    const sead::Matrix34f& rViewMtx, f32 near, f32 far, f32 fovy);
void drawEffectPostEffect(const al::EffectSystem* pEffectSystem, const sead::Matrix44f& rProjMtx,
                          const sead::Matrix34f& rViewMtx, f32 near, f32 far, f32 fovy);
void drawEffectAfterFog(const al::EffectSystem* pEffectSystem, const sead::Matrix44f& rProjMtx,
                        const sead::Matrix34f& rViewMtx, f32 near, f32 far, f32 fovy);
void drawEffectShadowCaster(const al::EffectSystem* pEffectSystem,
                            const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                            f32 near, f32 far, f32 fovy);
void drawEffectShadowReceiver(const al::EffectSystem* pEffectSystem,
                              const sead::Matrix44f& rProjMtx, const sead::Matrix34f& rViewMtx,
                              f32 near, f32 far, f32 fovy);
void drawEffect2D(const al::EffectSystem* pEffectSystem, const sead::Viewport* pViewport);
void updateEffect2D(al::EffectSystem* pEffectSystem);
void calcShadowClipVolume(const al::EffectSystem* pEffectSystem,
                          agl::sdw::DepthShadow* pDepthShadow);
void tryDeleteEmitterAndParticleOneTime(const al::EffectSystemInfo* pSystemInfo);
void setDepthTexture(const al::EffectSystem* pEffectSystem, const agl::TextureData* pTexture);
bool isHasRenderingEmitterInReduceBuffer(const al::EffectSystem* pEffectSystem);
bool isHasRenderingEmitterInReduceBufferHdr(const al::EffectSystem* pEffectSystem);
}  // namespace alEffectSystemFunction

#pragma once

#include <gfx/seadColor.h>
#include <math/seadBoundBox.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
    class GraphicsSystemInfo;
    class ModelKeeper;
    class LiveActor;
    class FunctorBase;
    class SubActorKeeper;
    class ClippingDirectorBase;
    class UniformBlock;

    void tryInitFixedModelGpuBuffer(LiveActor*);

    UniformBlock* getModelUniformBlock(const LiveActor* pActor, const char* pName);

    bool isHideModel(const LiveActor*);

    void hideModel(LiveActor*);
    void showModel(LiveActor*);

    void showModelIfHide(LiveActor*);
    void setLODForceLevel0(LiveActor*);
    void forceApplyCubeMap(LiveActor* pActor, const char* pCubeMapName);
    void forceApplyCubeMap(ModelKeeper* pModelKeeper, const GraphicsSystemInfo* pInfo,
                           const char* pCubeMapName);
    void hideModelIfShow(LiveActor*);

    bool isExistModel(const LiveActor*);

    void setNeedSetBaseMtxAndCalcAnimFlag(LiveActor*, bool);

    sead::Matrix34f* getJointMtxPtr(const LiveActor*, const char*);

    void updateMaterialCodeWater(LiveActor*);

    void setRenderStateBlendColor(LiveActor*, const sead::Color4f&);

    void setCustomRenderEnable(LiveActor*, bool);
    void setEnableDepthTest(LiveActor*, bool);
    void setEnableDepthWrite(LiveActor*, bool);

    void setFixedModelFlag(LiveActor* pActor);
    void setIgnoreUpdateDrawClipping(LiveActor* pActor, bool isIgnore);
    void changeEnvTextureWetObj(LiveActor* pActor);
    void changeEnvTextureWetObjStrong(LiveActor* pActor);
    void setModelLodParams(const LiveActor* pActor, const f32* pSwitchDistances, s32 lodNum,
                           bool isDisableDemoLod);
    f32 calcModelBoundingSphereRadius(const LiveActor* pActor);
    void setLodDisabled(LiveActor* pActor, bool isDisabled);

    void updateMaterialCodeArea(LiveActor* pActor);
    void updateMaterialCodeWater(LiveActor* pActor, bool isInWater, bool isAshore);
    void resetMaterialCode(LiveActor* pActor);
    void setModelAlphaPtr(LiveActor* pActor, f32* pAlpha);
    void switchShowHideModelIfNearCamera(LiveActor* pActor, f32 distance);
    void showSilhouetteModel(LiveActor* pActor);
    void hideSilhouetteModel(LiveActor* pActor);
    bool isSilhouetteModelHidden(LiveActor* pActor);
    void showSilhouetteModelIfHide(LiveActor* pActor);
    void hideSilhouetteModelIfShow(LiveActor* pActor);
    LiveActor* getSilhouetteModel(LiveActor* pActor);
    void setDisableDraw(LiveActor* pActor, bool isDisable);
    void removeFromDepthShadowDrawer(LiveActor* pActor);
    void addToDepthShadowDrawer(LiveActor* pActor);
    void setDisableDepthShadow(LiveActor* pActor, bool isDisable, bool isUpdateDrawer);
    void showInvincibleModel(LiveActor* pActor);
    void hideInvincibleModel(LiveActor* pActor);
    bool isInvincibleModelHidden(LiveActor* pActor);
    bool isExistJoint(const LiveActor* pActor, const char* pName);
    sead::Matrix34f* getJointMtxPtrByIndex(const LiveActor* pActor, s32 index);
    const sead::Matrix34f* getJointLocalMtxPtr(const LiveActor* pActor, const char* pName);
    const void* getJointLocalMtxPtrByIndex(const LiveActor* pActor, s32 index);
    void calcJointPos(sead::Vector3f* pPos, const LiveActor* pActor, const char* pName);
    void calcJointSideDir(sead::Vector3f* pDir, const LiveActor* pActor, const char* pName);
    void calcJointUpDir(sead::Vector3f* pDir, const LiveActor* pActor, const char* pName);
    void calcJointFrontDir(sead::Vector3f* pDir, const LiveActor* pActor, const char* pName);
    void multVecJointMtx(sead::Vector3f* pOut, const sead::Vector3f& rVec, LiveActor* pActor, const char* pName);
    void multVecJointInvMtx(sead::Vector3f* pOut, const sead::Vector3f& rVec, LiveActor* pActor, const char* pName);
    void setJointVisibility(const LiveActor* pActor, const char* pName, bool isVisible);
    bool getJointVisibility(const LiveActor* pActor, const char* pName);
    const char* getModelName(const LiveActor* pActor);
    void calcModelBoundingBox(sead::BoundBox3f* pBox, const LiveActor* pActor);
    void submitViewModel(const LiveActor* pActor, const sead::Matrix34f& rViewMtx);
    bool isJudgedToClipFrustum(const ClippingDirectorBase* pDirector, const sead::Vector3f& rPos, f32 radius, f32 nearClip);
    bool isJudgedToClipFrustum(const LiveActor* pActor, const sead::Vector3f& rPos, f32 radius, f32 nearClip);
    bool isJudgedToClipFrustum(const LiveActor* pActor, f32 radius, f32 nearClip);
    bool isJudgedToClipFrustumWithoutFar(const ClippingDirectorBase* pDirector, const sead::Vector3f& rPos, f32 radius, f32 nearClip);
    bool isJudgedToClipFrustumWithoutFar(const LiveActor* pActor, const sead::Vector3f& rPos, f32 radius, f32 nearClip);
    bool isJudgedToClipFrustumWithoutFar(const LiveActor* pActor, f32 radius, f32 nearClip);
    void resetPosition(LiveActor* pActor, bool isSkipCalcAnim);
    void resetPosition(LiveActor* pActor, const sead::Vector3f& rTrans, bool isSkipCalcAnim);
    void resetPosition(LiveActor* pActor, const sead::Vector3f& rTrans, const sead::Vector3f& rRotate);
    void resetEnvTexture(LiveActor* pActor);
    void changeEnvTextureStamp(LiveActor* pActor);
    void setEnvTextureMirror(LiveActor* pActor, s32 textureId);
    void enableUpdateModelBounding(LiveActor* pActor);
    void disableUpdateModelBounding(LiveActor* pActor);
    void createRenderState(LiveActor* pActor);
    void resetRenderState(LiveActor* pActor);
    void hideMaterial(LiveActor* pActor, s32 index);
    void showMaterial(LiveActor* pActor, s32 index);
    void hideMaterialAll(LiveActor* pActor);
    void showMaterialAll(LiveActor* pActor);
    void setMaterialProgrammable(LiveActor* pActor);
    void setPostUpdateWorldMatrixCallback(LiveActor* pActor, const FunctorBase& rFunctor);
    void getBoundingShpereCenterAndRadius(sead::Vector3f* pCenter, f32* pRadius, const LiveActor* pActor);
}  // namespace al

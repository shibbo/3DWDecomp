#pragma once

#include <gfx/seadCamera.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/HostIO/IUseHioNode.hpp"
#include "Library/HostIO/IUseName.hpp"
#include "Library/Nerve/IUseNerve.hpp"
#include "Library/Projection/OrthoProjectionInfo.hpp"
#include "Library/Rail/IUseRail.hpp"
#include "Project/AreaObj/IUseAreaObj.hpp"
#include "Project/Audio/IUseAudioKeeper.hpp"
#include "Project/Collision/IUseCollision.hpp"

namespace al {
class AudioKeeper;
class ByamlIter;
class CameraAngleCtrlInfo;
class CameraAngleSwingInfo;
class CameraArrowCollider;
class CameraInputHolder;
class CameraOffsetCtrlPreset;
class CameraParamMoveLimit;
class CameraTargetAreaLimitter;
class CameraTargetHolder;
class CameraTurnInfo;
class CameraVerticalAbsorber;
class CameraViewInfo;
class GyroCameraCtrl;
class Nerve;
class NerveKeeper;
struct PlacementInfo;
class RailKeeper;
class SnapShotCameraCtrl;
struct CameraFlagCtrl;
struct CameraObjectRequestInfo;
struct CameraPoserFlag;
struct CameraPoserSceneInfo_RS;
struct CameraStartInfo;

class CameraPoser_RS : public HioNode,
                       public IUseAreaObj,
                       public IUseAudioKeeper,
                       public IUseCollision,
                       public IUseName,
                       public IUseNerve,
                       public IUseRail {
public:
    enum class ActiveState : s32 {
        Start = 0,
        Active = 1,
        End = 2,
    };

    enum class CameraInterpoleStepType : s32 {
        Undefined = -1,
        ByStep = 0,
        ByCameraDistance = 1,
    };

    struct LocalInterpole {
        s32 step = -1;
        s32 end = 0;
        sead::Vector3f prevCameraPos = {0.0f, 0.0f, 0.0f};
        sead::Vector3f prevLookAtPos = {0.0f, 0.0f, 0.0f};
    };

    static_assert(sizeof(LocalInterpole) == 0x20);

    struct LookAtInterpole {
        LookAtInterpole(f32 rate) : lerp(rate) {}

        sead::Vector3f target = {0.0f, 0.0f, 0.0f};
        f32 lerp;
    };

    static_assert(sizeof(LookAtInterpole) == 0x10);

    struct CameraInterpoleStep {
        CameraInterpoleStepType stepType = CameraInterpoleStepType::Undefined;
        s32 stepNum = -1;
    };

    static_assert(sizeof(CameraInterpoleStep) == 0x8);

    struct CameraInterpoleParam : public CameraInterpoleStep {
        CameraInterpoleParam() : CameraInterpoleStep({CameraInterpoleStepType::ByCameraDistance}) {}

        void set(CameraInterpoleStepType type, s32 step, bool isByStep) {
            stepType = type;
            stepNum = step;
            isInterpolateByStep = isByStep;
        }

        s8 isEaseOut = false;
        bool isInterpolateByStep = false;
    };

    static_assert(sizeof(CameraInterpoleParam) == 0xC);

    struct OrthoProjectionParam {
        OrthoProjectionParam() {}

        bool isSetInfo = false;
        OrthoProjectionInfo info;
    };

    static_assert(sizeof(OrthoProjectionParam) == 0xC);

    CameraPoser_RS(const char* pName);

    AreaObjDirector* getAreaObjDirector() const override;

    virtual void init() {}

    virtual void initByPlacementObj(const PlacementInfo& rInfo) {}

    virtual void endInit() {}

    virtual void start(const CameraStartInfo& rInfo) {}

    virtual void update() {}

    virtual void end() { mActiveState = ActiveState::End; }

    virtual void loadParam(const ByamlIter& rIter) {}

    virtual void makeLookAtCamera(sead::LookAtCamera* pCamera) const {}

    virtual bool receiveRequestFromObject(const CameraObjectRequestInfo& rInfo) { return false; }

    virtual bool isZooming() const { return false; }

    virtual bool isEnableRotateByPad() const;

    virtual void reset() {}

    virtual void startSnapShotMode() {}

    virtual void endSnapShotMode() {}

    const char* getName() const override { return mPoserName; }

    CollisionDirector* getCollisionDirector() const override;

    NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }

    AudioKeeper* getAudioKeeper() const override { return mAudioKeeper; }

    RailRider* getRailRider() const override;

    virtual void load(const ByamlIter& rIter);
    virtual void movement();
    virtual void calcCameraPose(sead::LookAtCamera* pCamera) const;

    virtual void startCameraReset(bool isReset) {}

    virtual f32 getVerticalAngle() { return 0.0f; }

    virtual bool requestTurnToDirection(const CameraTurnInfo* pInfo) { return false; }

    bool tryCalcOrthoProjectionInfo(OrthoProjectionInfo* pInfo) const;
    f32 getFovyDegree() const;
    f32 getSceneFovyDegree() const;
    CameraInputHolder* getInputHolder() const;
    CameraTargetHolder* getTargetHolder() const;
    CameraFlagCtrl* getFlagCtrl() const;
    f32 getAbsorbHeight() const;
    bool isInterpoleByCameraDistance() const;
    s32 getInterpoleStep() const;
    void setInterpoleStep(s32 step);
    void setEndInterpoleStep(s32 step);
    void resetInterpoleStep();
    bool isInterpoleEaseOut() const;
    void setInterpoleEaseOut();
    bool isEndInterpoleByStep() const;
    s32 getEndInterpoleStep() const;
    void initNerve(const Nerve* pNerve, s32 stateNum);
    void initArrowCollider(CameraArrowCollider* pCollider);
    void initAudioKeeper(const char* pName);
    void initRail(const PlacementInfo& rInfo);
    void initLocalInterpole();
    void initLookAtInterpole(f32 rate);
    void initOrthoProjectionParam();
    void tryInitAreaLimitter(const PlacementInfo& rInfo);
    bool isFirstCalc() const;
    void appear(const CameraStartInfo& rInfo);
    void makeLookAtCameraPrev(sead::LookAtCamera* pCamera) const;
    void makeLookAtCameraPost(sead::LookAtCamera* pCamera) const;
    void makeLookAtCameraLast(sead::LookAtCamera* pCamera) const;
    void movementAndCalcCameraPoseForEndAfterInterpole(sead::LookAtCamera* pCamera);
    void setPauseMoveLimit(bool isPause);
    void setWaterHeight(f32 height);
    void makeLookAtCameraCollide(sead::LookAtCamera* pCamera) const;
    bool receiveRequestFromObjectCore(const CameraObjectRequestInfo& rInfo);
    void startSnapShotModeCore();
    void enableSnapShotRoll(bool isEnable);
    void endSnapShotModeCore();

    const sead::Vector3f& getEye() const { return mEye; }

    const sead::Vector3f& getAt() const { return mAt; }

    sead::Vector3f* getAtPtr() { return &mAt; }

    sead::Vector3f* getUpPtr() { return &mUp; }

    const sead::Vector3f& getUp() const { return mUp; }

    const sead::Matrix34f& getViewMtx() const { return mViewMtx; }

    f32 getNearClipDistance() const { return mNearClipDistance; }

    f32 getFarClipDistance() const { return mFarClipDistance; }

    bool is140() const { return _140; }

    bool is141() const { return _141; }

    bool isCalcEndAfterInterpole() const { return _9c; }

    CameraViewInfo* getViewInfo() const { return mViewInfo; }

    CameraPoserSceneInfo_RS* getSceneInfo() const { return mSceneInfo; }

    CameraVerticalAbsorber* getCameraVerticalAbsorber() const { return mVerticalAbsorber; }

    CameraAngleCtrlInfo* getAngleCtrlInfo() const { return mAngleCtrlInfo; }

    CameraAngleSwingInfo* getAngleSwingInfo() const { return mAngleSwingInfo; }

    CameraOffsetCtrlPreset* getOffsetCtrlPreset() const { return mOffsetCtrlPreset; }

    CameraParamMoveLimit* getParamMoveLimit() const { return mParamMoveLimit; }

    GyroCameraCtrl* getGyroCtrl() const { return mGyroCtrl; }

    CameraPoserFlag* getPoserFlag() const { return mPoserFlag; }

    SnapShotCameraCtrl* getSnapShotCtrl() const { return mSnapShotCtrl; }

    void setEye(const sead::Vector3f& rPos) { mEye = rPos; }

    void addEyeOffset(const sead::Vector3f& rOffset) { mEye += rOffset; }

    void setAt(const sead::Vector3f& rPos) { mAt = rPos; }

    void setCameraUp(const sead::Vector3f& rDir) { mUp = rDir; }

    void setViewMtx(const sead::Matrix34f& rMtx) { mViewMtx = rMtx; }

    void setFovyDegree(f32 fovy) { mFovyDegree = fovy; }

    void setViewInfo(CameraViewInfo* pInfo) { mViewInfo = pInfo; }

    void setSceneInfo(CameraPoserSceneInfo_RS* pInfo) { mSceneInfo = pInfo; }

    void setNearClipDistance(f32 distance) { mNearClipDistance = distance; }

    void setVerticalAbsorber(CameraVerticalAbsorber* pAbsorber) { mVerticalAbsorber = pAbsorber; }

    void setParamMoveLimit(CameraParamMoveLimit* pLimit) { mParamMoveLimit = pLimit; }

    void setAngleCtrlInfo(CameraAngleCtrlInfo* pInfo) { mAngleCtrlInfo = pInfo; }

    void setAngleSwingInfo(CameraAngleSwingInfo* pInfo) { mAngleSwingInfo = pInfo; }

    void setOffsetCtrlPreset(CameraOffsetCtrlPreset* pPreset) { mOffsetCtrlPreset = pPreset; }

    void setGyroCtrl(GyroCameraCtrl* pCtrl) { mGyroCtrl = pCtrl; }

    void setSnapShotCtrl(SnapShotCameraCtrl* pCtrl) { mSnapShotCtrl = pCtrl; }

protected:
    const char* mPoserName;
    ActiveState mActiveState = ActiveState::Start;
    sead::Vector3f mEye = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mAt = {0.0f, 0.0f, 500.0f};
    sead::Vector3f mUp = sead::Vector3f::ey;
    f32 mFovyDegree = 45.0f;
    f32 mNearClipDistance = -1.0f;
    f32 mFarClipDistance = 150000.0f;
    sead::Matrix34f mViewMtx = sead::Matrix34f::ident;
    bool _9c = false;
    CameraPoserSceneInfo_RS* mSceneInfo = nullptr;
    CameraViewInfo* mViewInfo = nullptr;
    CameraPoserFlag* mPoserFlag;
    CameraVerticalAbsorber* mVerticalAbsorber = nullptr;
    CameraAngleCtrlInfo* mAngleCtrlInfo = nullptr;
    CameraAngleSwingInfo* mAngleSwingInfo = nullptr;
    CameraArrowCollider* mArrowCollider = nullptr;
    CameraOffsetCtrlPreset* mOffsetCtrlPreset = nullptr;
    LocalInterpole* mLocalInterpole = nullptr;
    LookAtInterpole* mLookAtInterpole = nullptr;
    CameraParamMoveLimit* mParamMoveLimit = nullptr;
    CameraTargetAreaLimitter* mTargetAreaLimitter = nullptr;
    GyroCameraCtrl* mGyroCtrl = nullptr;
    SnapShotCameraCtrl* mSnapShotCtrl = nullptr;
    AudioKeeper* mAudioKeeper = nullptr;
    NerveKeeper* mNerveKeeper = nullptr;
    RailKeeper* mRailKeeper = nullptr;
    CameraInterpoleParam* mActiveInterpoleParam = nullptr;
    CameraInterpoleStep* mEndInterpoleParam = nullptr;
    OrthoProjectionParam* mOrthoProjectionParam = nullptr;
    bool _140 = false;
    bool _141 = false;
};

static_assert(sizeof(CameraPoser_RS) == 0x148);

}  // namespace al

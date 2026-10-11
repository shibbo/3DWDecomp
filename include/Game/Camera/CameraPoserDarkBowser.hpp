#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadCamera.h>
#include <math/seadVector.h>

#include "Library/Camera/CameraPoser_RS.hpp"

namespace al {
class ByamlIter;
class CameraOffsetPreset;
struct CameraStartInfo;
struct PlacementInfo;
}  // namespace al

/**
 * @brief Camera of the Fury Bowser fights in Bowser's Fury: circles around Fury Bowser (the axis)
 * and keeps the player in view, with an aerial camera for when Fury Bowser rises up.
 */
class CameraPoserDarkBowser : public al::CameraPoser_RS {
public:
    CameraPoserDarkBowser(const char* pName, const sead::Vector3f* pAxisPos);

    void init() override;
    void initByPlacementObj(const al::PlacementInfo& rInfo) override;
    void loadParam(const al::ByamlIter& rIter) override;
    void start(const al::CameraStartInfo& rInfo) override;
    f32 calcMarginH() const;
    sead::Vector3f calcOffset();
    void calcAxisPos(sead::Vector3f* pAxisPos) const;
    void calcDirHV(sead::Vector3f* pDir, f32 angleH, f32 angleV) const;
    f32 calcDistance();
    void startSnapShotMode() override;
    void endSnapShotMode() override;
    void setParams(f32 marginAngleH, f32 angleV);
    void requestAutoCamera();
    void requestCameraIn();
    void requestCameraOut();
    void requestLookAtStaticTarget(const sead::Vector3f& rTarget);
    void requestAerialCamera();
    void endAerialCamera();
    void toggleFocus(const sead::Vector3f& rLookAt, const sead::Vector3f& rCameraPos);
    f32 calcMaxMarginRate() const;
    void movement() override;
    f32 calcStickRotateSpeedWithFrictionH() const;
    void update() override;
    void resetInputRotate(f32 speed, s32 minStep);
    void makeLookAtCamera(sead::LookAtCamera* pCamera) const override;

    void exeAuto();
    void exeAutoInput();
    void exeFreeLook();
    bool checkStickyCamera() const;
    void exeAerial();
    void calcAerialMidpoint(sead::Vector3f* pMidpoint) const;
    void exeAerialEnd();
    void exeFocus();

    void setZoomIn(f32 distance, f32 distanceNear, f32 offsetY, s32 frame, bool isUnused);
    void startRotateInterp(f32 startAngleH, f32 endAngleH, f32 speed, s32 minStep);
    f32 getDist() const;
    f32 getDistNear() const;
    f32 calcDistanceBetweenAxisPosAndTargetPos() const;

private:
    void updateAngleVAboveWater();

    bool isResetting() const { return mResetStep >= 0 && mResetStep < mResetStepNum; }

    const sead::Vector3f* mAxisPosPtr;
    sead::Vector3f mAxisPos = {0.0f, 0.0f, 0.0f};
    bool mIsSetAxisPos = false;
    sead::Vector3f mLocalAxisPos = {0.0f, 0.0f, 0.0f};
    s32 _16c;
    s32 mResetStep = -1;
    s32 mResetStepNum = -1;
    f32 mResetStartAngleH = 0.0f;
    f32 mResetEndAngleH = 0.0f;
    f32 mResetSpeed = 1.1f;
    al::CameraOffsetPreset* mOffsetPreset = nullptr;
    f32 mDistance = 1800.0f;
    f32 mDistanceNear = 1800.0f;
    f32 mDistanceOut = 1800.0f;
    f32 mDistanceNearOut = 1800.0f;
    f32 mZoomDistance = 1800.0f;
    f32 mZoomDistanceNear = 1800.0f;
    f32 mZoomOffsetY = 0.0f;
    s32 mZoomFrame = 0;
    s32 mZoomStep = 0;
    f32 mSwitchToFollowDistance = 1000.0f;
    f32 mAngleH = 0.0f;
    f32 mTargetAngleH = 0.0f;
    f32 mMarginAngleH = 60.0f;
    f32 mUserMarginAngleH = -1.0f;
    f32 mCurrentMarginAngleH = 60.0f;
    f32 mFollowSpeedH = 0.0f;
    f32 mInputSpeedH = 0.0f;
    f32 mInterpAngleH = 0.0f;
    sead::Vector3f mTowerEye = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mPrevTargetTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mPrevAxisPos = {0.0f, 0.0f, 0.0f};
    f32 mInputOffsetAngleH = 0.0f;
    f32 mInterpRotateSpeedByFrame = 1.1f;
    bool mIsClampInMarginInputOffsetAngleH = false;
    bool mIsValidVelocityOffset = false;
    bool mIsZoomIn = false;
    bool mIsAuto = true;
    bool mIsFocus = false;
    bool mIsCameraIn = true;
    bool mIsAerialAngleReached = false;
    s32 mCameraInStep = 0;
    s32 mMarginStep = 0;
    sead::Vector3f mFocusLookAt = sead::Vector3f::zero;
    sead::Vector3f mFocusCameraPos = sead::Vector3f::zero;
    f32 mVelocityOffset = 0.0f;
    f32 mVelocityOffsetTarget = 0.0f;
    f32 mVelocityOffsetMax = 500.0f;
};

static_assert(sizeof(CameraPoserDarkBowser) == 0x238);

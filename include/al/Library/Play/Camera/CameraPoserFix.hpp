#pragma once

#include <math/seadVector.h>

#include "Library/Camera/CameraPoser_RS.hpp"

namespace al {

class CameraPoserFix : public CameraPoser_RS {
public:
    CameraPoserFix(const char* pName);

    void init() override;
    void initCameraPosAndLookAtPos(const sead::Vector3f& rCameraPos,
                                   const sead::Vector3f& rLookAtPos);
    void loadParam(const ByamlIter& rIter) override;
    void start(const CameraStartInfo& rInfo) override;
    sead::Vector3f& getFixedLookAt();
    void setFixedLookAt(sead::Vector3f& rLookAtPos);
    void setPhaseOffsets(f32 distanceOffset, f32 heightOffset);
    void setDistanceInitOffset(f32 offset, f32 decayRate);
    void setRedirectAngles(f32 angleH, f32 angleV, f32 rate);
    bool isSecondCameraDone();
    void update() override;
    void resetReturn();
    void storeCamera(const sead::Vector3f& rCameraPos, const sead::Vector3f& rLookAtPos);
    void setReturnWithAngles(s32 step, f32 angleH, f32 angleV, f32 distance);
    void setReturn(s32 step);

    static const char* getFixAbsoluteCameraName();
    static const char* getFixDoorwayCameraName();

    void setIsCalcNearestAtFromPreAt(bool isCalcNearestAtFromPreAt) {
        mIsCalcNearestAtFromPreAt = isCalcNearestAtFromPreAt;
    }

    void setOwnerObject(void* pOwner) { _1c8 = pOwner; }

    bool isReturnDone() const { return mIsReturnDone; }

private:
    sead::Vector3f mLookAtPos = sead::Vector3f::zero;
    sead::Vector3f mReturnStartCameraPos = sead::Vector3f::zero;
    f32 mDistance = 1800.0f;
    f32 mDistanceOffset = 0.0f;
    f32 mDistanceInitOffset = 0.0f;
    f32 mDistanceInitOffsetRate = 0.0f;
    f32 mHeightOffset = 0.0f;
    f32 mAngleV = 30.0f;
    f32 mAngleH = 0.0f;
    bool mIsCalcNearestAtFromPreAt = false;
    sead::Vector3f mPreLookAtPos = sead::Vector3f::zero;
    sead::Vector3f mReturnCameraPos = sead::Vector3f::zero;
    sead::Vector3f mReturnLookAtPos = sead::Vector3f::zero;
    f32 mReturnDistance = 0.0f;
    s32 mReturnStepMax = 0;
    s32 mReturnStep = 0;
    f32 mRedirectAngleH = 0.0f;
    f32 mRedirectAngleV = 0.0f;
    f32 mCurrentAngleH = 0.0f;
    f32 mCurrentAngleV = 0.0f;
    f32 mRedirectRate = 0.0f;
    bool mIsReturn = false;
    bool mIsReturnDone = false;
    bool mIsRedirect = false;
    bool mIsSecondCameraDone = false;
    void* _1c8 = nullptr;
};

static_assert(sizeof(CameraPoserFix) == 0x1d0);

}  // namespace al

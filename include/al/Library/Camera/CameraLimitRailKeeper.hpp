#pragma once

#include <math/seadVector.h>

namespace al {
class CameraPoser_RS;
struct PlacementInfo;
class Rail;
class RailRider;

class CameraLimitRailKeeper {
public:
    CameraLimitRailKeeper();

    void init(const PlacementInfo& rInfo, s32 viewNum);
    void updateRider(const CameraPoser_RS* pPoser);
    RailRider* getRider(const CameraPoser_RS* pPoser) const;
    void calcCameraDirH(sead::Vector3f* pDir, const CameraPoser_RS* pPoser) const;
    bool isNearInsideRailPoint(const CameraPoser_RS* pPoser, f32 margin) const;
    const sead::Vector3f& getRailPos(const CameraPoser_RS* pPoser) const;
    f32 calcDistanceFromNearestRailPos(const sead::Vector3f& rPos) const;
    void calcNearestRailPos(sead::Vector3f* pOut, const sead::Vector3f& rPos) const;

    f32 getActivateDistance() const { return mActivateDistance; }

    f32 getDegreeMargin() const { return mDegreeMargin; }

    f32 getAngleElevation() const { return mAngleElevation; }

    f32 getAngleElevation2() const { return mAngleElevation2; }

    s32 getAngleElevationIterpStep() const { return mAngleElevationIterpStep; }

    s32 getAngleElevationResetStep() const { return mAngleElevationResetStep; }

    bool isApplyAngleElevation() const { return mIsApplyAngleElevation; }

    bool isResetAngleElevation() const { return mIsResetAngleElevation; }

    bool isFixedAngle() const { return mIsFixedAngle; }

    bool isInvalidCheckCollision() const { return mIsInvalidCheckCollision; }

private:
    Rail* mRail = nullptr;
    RailRider** mRiders = nullptr;
    s32 mRiderCount = 0;
    s32 mCameraLookAtDir = 0;
    f32 mActivateDistance = 1000.0f;
    f32 mDegreeMargin = 40.0f;
    f32 mAngleElevation = 20.0f;
    f32 mAngleElevation2 = 20.0f;
    s32 mAngleElevationIterpStep = 60;
    s32 mAngleElevationResetStep = 60;
    f32 mCameraDistance = 2000.0f;
    f32 mCameraDistanceMin = 250.0f;
    f32 mCameraDistanceMax = 4000.0f;
    f32 mOffsetY = 120.0f;
    bool mIsApplyAngleElevation = false;
    bool mIsResetAngleElevation = false;
    bool mIsFixedAngle = false;
    bool mIsApplyCameraDistance = false;
    bool mIsApplyCameraDistanceRange = false;
    bool mIsApplyOffsetY = false;
    bool mIsInvalidCheckCollision = false;
};

}  // namespace al

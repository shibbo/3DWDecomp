#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

class CameraAngleVerticalCtrl;

/**
 * @brief The state of the target and the input that a vertical camera angle update reads.
 */
class CameraAngleUpdateInfo {
public:
    /**
     * Constructs an update info for a target standing still without input.
     */
    CameraAngleUpdateInfo()
        : mIsParallel2D(false), mIsSnapShotMode(false), mDefaultAngle(23.0f),
          mCameraPos(0.0f, 0.0f, 0.0f), mLookAtPos(0.0f, 0.0f, 0.0f),
          mTargetTrans(0.0f, 0.0f, 0.0f), mPrevTargetTrans(0.0f, 0.0f, 0.0f), mStick(0.0f, 0.0f),
          mStickSensitivityLevel(0), mStickSensitivityScale(1.0f), mIsPlayerTypeFlyer(false),
          mIsOnRideObj(false), mIsOnGround(false), mGroundNormal(0.0f, 0.0f, 0.0f),
          mIsChaseSubTarget(false), mSubTargetAngle(0.0f), mSubTargetRate(0.0f),
          mRotationScaler(1.0f), mIsInInk(false) {}

    CameraAngleUpdateInfo(bool isParallel2D, bool isSnapShotMode, f32 defaultAngle,
                          const sead::Vector3f& rCameraPos, const sead::Vector3f& rLookAtPos,
                          const sead::Vector3f& rTargetTrans, const sead::Vector3f& rPrevTargetTrans,
                          const sead::Vector2f& rStick, s32 stickSensitivityLevel,
                          f32 stickSensitivityScale, bool isPlayerTypeFlyer, bool isOnRideObj,
                          bool isOnGround, const sead::Vector3f& rGroundNormal);

    void chaseToSubTarget(f32 angle, f32 rate);

    void setRotationScaler(f32 scaler) { mRotationScaler = scaler; }

    void setIsFreezeAngle(bool isFreeze) { mIsFreezeAngle = isFreeze; }

    void setIsInInk(bool isInInk) { mIsInInk = isInInk; }

private:
    friend class CameraAngleVerticalCtrl;

    bool mIsParallel2D;
    bool mIsSnapShotMode;
    f32 mDefaultAngle;
    sead::Vector3f mCameraPos;
    sead::Vector3f mLookAtPos;
    sead::Vector3f mTargetTrans;
    sead::Vector3f mPrevTargetTrans;
    sead::Vector2f mStick;
    s32 mStickSensitivityLevel;
    f32 mStickSensitivityScale;
    bool mIsPlayerTypeFlyer;
    bool mIsOnRideObj;
    bool mIsOnGround;
    sead::Vector3f mGroundNormal;
    bool mIsChaseSubTarget;
    f32 mSubTargetAngle;
    f32 mSubTargetRate;
    f32 mRotationScaler;
    bool mIsFreezeAngle;
    bool mIsInInk;
};

static_assert(sizeof(CameraAngleUpdateInfo) == 0x6c);

#include "Camera/CameraAngleVerticalCtrl.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>

#include "Camera/CameraPoserFollowLimit.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Yaml/ByamlUtil.hpp"

namespace {

NERVE_DECL(CameraAngleVerticalCtrl, UserCtrl)
NERVE_DECL(CameraAngleVerticalCtrl, WaterCtrl)
NERVE_DECL(CameraAngleVerticalCtrl, HackFlyerCtrl)
NERVE_DECL(CameraAngleVerticalCtrl, Interp)
NERVE_DECL(CameraAngleVerticalCtrl, SnapStart)
NERVE_DECL(CameraAngleVerticalCtrl, Snap)
NERVE_DECL(CameraAngleVerticalCtrl, SnapEnd)

// The interpolation back to the default angle that starts by itself on low angles.
class CameraAngleVerticalCtrlNrvInterpAutoReset : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<CameraAngleVerticalCtrl>()->exeInterp();
    }
};

NERVES_MAKE_NOSTRUCT(CameraAngleVerticalCtrl, UserCtrl, WaterCtrl, HackFlyerCtrl, Interp,
                     SnapStart, Snap, SnapEnd, InterpAutoReset)

/// The lowest and highest vertical angles the camera can ever take.
constexpr f32 cAngleLimitMin = -87.5f;
constexpr f32 cAngleLimitMax = 87.5f;

/// The vertical angle range when no range is set.
constexpr f32 cDefaultRangeMin = -35.0f;
constexpr f32 cDefaultRangeMax = 85.0f;

/// The lowest vertical angle while the camera is under water.
constexpr f32 cUnderWaterRangeMin = -55.0f;

/// The rate the angle follows the target angle with.
constexpr f32 cAngleChaseRate = 0.1f;

/// The rate the target angle follows a requested angle with.
constexpr f32 cTargetChaseRate = 0.7f;

/// The number of steps of a snap.
constexpr s32 cSnapStep = 60;

/// The minimum number of steps of an interpolation.
constexpr s32 cInterpStepMin = 25;

}  // namespace

/**
 * Constructs the vertical angle control of a follow camera.
 * @param pPoser The follow camera.
 */
CameraAngleVerticalCtrl::CameraAngleVerticalCtrl(al::CameraPoser_RS* pPoser)
    : al::NerveExecutor("カメラ垂直角度操作"), mAngleDegree(getInitDefaultAngleDegree()),
      mTargetAngleDegree(getInitDefaultAngleDegree()),
      mPrevAngleDegree(getInitDefaultAngleDegree()),
      mPrevTargetAngleDegree(getInitDefaultAngleDegree()),
      mPoser(static_cast<CameraPoserFollowLimit*>(pPoser)) {
    mCtrlState = new CtrlState;
    mHackFlyerCtrl = new HackFlyerCtrl;
    mParam = new Param;
    mRailRange = new RailRange;
    initNerve(&NrvCameraAngleVerticalCtrlUserCtrl, 0);
}

/**
 * Gets the vertical angle a camera starts with.
 * @return The angle in degrees.
 */
f32 CameraAngleVerticalCtrl::getInitDefaultAngleDegree() {
    return 23.0f;
}

/**
 * Loads the vertical angle parameters.
 * @param rIter The camera parameters.
 */
void CameraAngleVerticalCtrl::loadParam(const al::ByamlIter& rIter) {
    Param* param = mParam;
    al::tryGetByamlF32(&param->defaultAngle, rIter, "DefaultAngle");

    if (al::tryGetByamlBool(&param->isSetAngleRange, rIter, "IsSetAngleRangeV") &&
        param->isSetAngleRange) {
        al::tryGetByamlF32(&param->minAngle, rIter, "MinAngle");
        al::tryGetByamlF32(&param->maxAngle, rIter, "MaxAngle");
    }

    al::tryGetByamlF32(&param->waterNeutralAngle, rIter, "WaterNeutralAngle");
    al::tryGetByamlBool(&param->isValidAutoLook, rIter, "IsValidAutoLook");
    al::tryGetByamlBool(&param->isInvalidHackFlyerCtrl, rIter, "IsInvalidHackFlyerCtrl");
    al::tryGetByamlBool(&param->isInvalidAutoLowAngleReset, rIter, "IsInvalidAutoLowAngleReset");
    param->isInvalidAutoLowAngleReset = true;
}

/**
 * Gets the nerve of the controller that is currently active.
 * @return The nerve.
 */
inline const al::Nerve* CameraAngleVerticalCtrl::getCtrlNerve() const {
    if (mCtrlState->hackFlyerState == 0) {
        return &NrvCameraAngleVerticalCtrlHackFlyerCtrl;
    }

    if (mCtrlState->type == CtrlType::Water) {
        return &NrvCameraAngleVerticalCtrlWaterCtrl;
    }

    if (mCtrlState->type == CtrlType::User) {
        return &NrvCameraAngleVerticalCtrlUserCtrl;
    }

    return nullptr;
}

/**
 * Hands the angle back to the controller that is currently active.
 */
inline void CameraAngleVerticalCtrl::setCtrlNerve() {
    al::setNerve(this, getCtrlNerve());
}

/**
 * Starts controlling the angle.
 * @param rTargetTrans The position of the target.
 */
void CameraAngleVerticalCtrl::start(const sead::Vector3f& rTargetTrans) {
    mAngleSpeed = 0.0f;
    mIsCancelAutoReset = false;
    mPrevTargetTrans.set(rTargetTrans);
    mCtrlState->type = CtrlType::User;
    setCtrlNerve();
}

/**
 * Lets the user control the angle.
 */
void CameraAngleVerticalCtrl::startUserCtrl() {
    mCtrlState->type = CtrlType::User;
}

/**
 * Sets the highest angle the user can turn the camera to.
 * @param angle The angle in degrees.
 */
void CameraAngleVerticalCtrl::setHiDegreeLimit(f32 angle) {
    mHiDegreeLimit = angle;
    mIsValidHiDegreeLimit = true;
}

/**
 * Removes the highest angle the user can turn the camera to.
 */
void CameraAngleVerticalCtrl::clearHiDegreeLimit() {
    mIsValidHiDegreeLimit = false;
}

/**
 * Sets the lowest angle the camera can take.
 * @param angle The angle in degrees.
 */
void CameraAngleVerticalCtrl::setLowDegreeLimit(f32 angle) {
    mLowDegreeLimit = angle;
    mIsValidLowDegreeLimit = true;
}

/**
 * Removes the lowest angle the camera can take.
 */
void CameraAngleVerticalCtrl::clearLowDegreeLimit() {
    mIsValidLowDegreeLimit = false;
}

/**
 * Updates the angle.
 * @param rInfo The state of the target and the input.
 */
void CameraAngleVerticalCtrl::update(const CameraAngleUpdateInfo& rInfo) {
    mInfo = rInfo;

    if (rInfo.mIsFreezeAngle && mAngleDegree < 0.0f) {
        mIsSnapAfterClimb = true;
        startSnap(10.0f);
    }

    if (mInfo.mIsPlayerTypeFlyer && !mParam->isInvalidHackFlyerCtrl) {
        mCtrlState->hackFlyerState = 0;
    } else {
        mCtrlState->hackFlyerState = -1;
    }

    if (al::isNerve(this, &NrvCameraAngleVerticalCtrlUserCtrl) ||
        al::isNerve(this, &NrvCameraAngleVerticalCtrlWaterCtrl) ||
        al::isNerve(this, &NrvCameraAngleVerticalCtrlHackFlyerCtrl)) {
        if (!al::isNerve(this, getCtrlNerve())) {
            setCtrlNerve();
        }
    }

    f32 prevAngle = mAngleDegree;
    updateNerve();
    mAngleSpeed = mAngleDegree - prevAngle;
    mPrevTargetTrans.set(rInfo.mTargetTrans);
}

/**
 * Starts snapping the angle.
 * @param angle The angle to snap to in degrees.
 */
void CameraAngleVerticalCtrl::startSnap(f32 angle) {
    mSnapAngleDegree = angle;
    al::setNerve(this, &NrvCameraAngleVerticalCtrlSnapStart);
}

/**
 * Gets the angle the camera returns to.
 * @return The angle in degrees.
 */
f32 CameraAngleVerticalCtrl::getDefaultAngleDegree() const {
    if (mInfo.mIsOnRideObj) {
        return 15.0f;
    }

    return mParam->defaultAngle;
}

/**
 * Gets the lowest angle of the current angle range.
 * @return The angle in degrees.
 */
inline f32 CameraAngleVerticalCtrl::getRangeMinDegree() const {
    const Param* param = mParam;
    const RailRange* railRange = mRailRange;
    f32 min = param->isSetAngleRange ? sead::Mathf::max(param->minAngle, cDefaultRangeMin) :
                                       cDefaultRangeMin;
    if (railRange->isValid) {
        min = sead::Mathf::clampMin(min, railRange->minAngle);
    }

    return min;
}

/**
 * Gets the highest angle of the current angle range.
 * @return The angle in degrees.
 */
inline f32 CameraAngleVerticalCtrl::getRangeMaxDegree() const {
    const Param* param = mParam;
    const RailRange* railRange = mRailRange;
    f32 max = param->isSetAngleRange ? sead::Mathf::min(param->maxAngle, cDefaultRangeMax) :
                                       cDefaultRangeMax;
    if (railRange->isValid) {
        max = sead::Mathf::clampMax(max, railRange->maxAngle);
    }

    return max;
}

/**
 * Clamps an angle to the current angle range.
 * @param angle The angle in degrees.
 * @return The clamped angle in degrees.
 */
inline f32 CameraAngleVerticalCtrl::clampAngleDegree(f32 angle) const {
    f32 min = sead::Mathf::clampMin(getRangeMinDegree(), cAngleLimitMin);
    f32 max = sead::Mathf::clampMax(getRangeMaxDegree(), cAngleLimitMax);
    return sead::Mathf::clamp(angle, min, max);
}

/**
 * Sets the angle at once.
 * @param angle The angle in degrees.
 */
void CameraAngleVerticalCtrl::setAngleDegree(f32 angle) {
    f32 clamped = clampAngleDegree(angle);
    mAngleDegree = clamped;
    mTargetAngleDegree = clamped;
    mPrevAngleDegree = clamped;
    mPrevTargetAngleDegree = clamped;
}

/**
 * Sets the angle range.
 * @param min The lowest angle in degrees.
 * @param max The highest angle in degrees.
 */
void CameraAngleVerticalCtrl::setVerticalAngleRange(f32 min, f32 max) {
    Param* param = mParam;
    param->minAngle = min;
    param->maxAngle = max;
    param->isSetAngleRange = true;
}

/**
 * Stores the angle range to restore it later.
 */
void CameraAngleVerticalCtrl::storeVerticalAngleRange() {
    Param* param = mParam;
    param->storedMinAngle = param->minAngle;
    param->storedMaxAngle = param->maxAngle;
}

/**
 * Restores the stored angle range.
 */
void CameraAngleVerticalCtrl::restoreVerticalAngleRange() {
    Param* param = mParam;
    param->minAngle = param->storedMinAngle;
    param->maxAngle = param->storedMaxAngle;
}

/**
 * Starts interpolating to an angle with a speed that depends on the distance.
 * @param angle The angle in degrees.
 */
void CameraAngleVerticalCtrl::startTargetInterpole(f32 angle) {
    f32 diff = sead::Mathf::abs(angle - mAngleDegree);
    f32 speed = mInfo.mStickSensitivityScale * 0.5f;
    if (speed <= 0.0f || diff < speed) {
        return;
    }

    startTargetInterpole(angle, static_cast<s32>(diff / speed));
}

/**
 * Starts interpolating to an angle.
 * @param angle The angle in degrees.
 * @param step The number of steps of the interpolation, or 0 or less to set the angle at once.
 */
void CameraAngleVerticalCtrl::startTargetInterpoleByStep(f32 angle, s32 step) {
    if (step <= 0) {
        setAngleDegree(angle);
        if (al::isNerve(this, &NrvCameraAngleVerticalCtrlInterp)) {
            setCtrlNerve();
        }

        return;
    }

    mStartAngleDegree = mAngleDegree;
    mTargetAngleDegree = clampAngleDegree(angle);
    mInterpStep = step;
    mIsSnapAfterClimb = false;

    if (!al::isNerve(this, &NrvCameraAngleVerticalCtrlInterp)) {
        al::setNerve(this, &NrvCameraAngleVerticalCtrlInterp);
    }
}

/**
 * Starts interpolating to an angle with a minimum number of steps.
 * @param angle The angle in degrees.
 * @param step The number of steps of the interpolation.
 */
void CameraAngleVerticalCtrl::startTargetInterpole(f32 angle, s32 step) {
    startTargetInterpoleByStep(angle, step > cInterpStepMin ? step : cInterpStepMin);
    mIsSnapAfterClimb = false;
}

/**
 * Starts interpolating back to the default angle.
 */
void CameraAngleVerticalCtrl::startResetInterpole() {
    mIsCancelAutoReset = false;
    startTargetInterpole(mInfo.mDefaultAngle);
}

/**
 * Starts interpolating back to the default angle.
 * @param step The number of steps of the interpolation.
 */
void CameraAngleVerticalCtrl::startResetInterpoleByStep(s32 step) {
    mIsCancelAutoReset = false;
    startTargetInterpoleByStep(mInfo.mDefaultAngle, step);
}

/**
 * Moves the target angle towards an angle.
 * @param angle The angle in degrees.
 */
void CameraAngleVerticalCtrl::chaseToTargetDegree(f32 angle) {
    f32 clamped = clampAngleDegree(angle);
    f32 target = al::lerpValue(cTargetChaseRate, mTargetAngleDegree, clamped);
    mTargetAngleDegree = al::lerpValue(cAngleChaseRate, mTargetAngleDegree, target);
}

/**
 * Moves the target angle towards an angle with a speed while the user controls the angle.
 * @param angle The angle in degrees.
 * @param speed The speed in degrees per step.
 */
void CameraAngleVerticalCtrl::chaseToTargetDegreeBySpeed(f32 angle, f32 speed) {
    if (al::isNerve(this, &NrvCameraAngleVerticalCtrlUserCtrl)) {
        chaseToTargetDegree(al::converge(mTargetAngleDegree, angle, speed));
    }
}

/**
 * Checks whether the angle range is too small to move the angle.
 * @return True if the lowest and the highest angle are the same.
 */
bool CameraAngleVerticalCtrl::isFixInRange() const {
    bool isWater = al::isNerve(this, &NrvCameraAngleVerticalCtrlWaterCtrl);

    f32 min = getRangeMinDegree();
    if (isWater) {
        min = sead::Mathf::clampMin(min, cUnderWaterRangeMin);
    }

    min = sead::Mathf::clampMin(min, cAngleLimitMin);

    f32 max = getRangeMaxDegree();
    if (isWater) {
        max = sead::Mathf::clampMax(max, cDefaultRangeMax);
    }

    max = sead::Mathf::clampMax(max, cAngleLimitMax);

    return !al::isNear(min, max, 0.001f);
}

/**
 * Sets the angle range of a camera limit rail and interpolates into it.
 * @param min The lowest angle in degrees.
 * @param max The highest angle in degrees.
 * @param step The number of steps of the interpolation.
 */
void CameraAngleVerticalCtrl::setRailAngleDegreeRangeAndInterp(f32 min, f32 max, s32 step) {
    mRailRange->isValid = true;
    mRailRange->minAngle = min;
    mRailRange->maxAngle = max;

    f32 prevAngle = mAngleDegree;
    mAngleDegree = clampAngleDegree(mAngleDegree);
    if (!al::isNear(prevAngle, mAngleDegree, 0.1f)) {
        startTargetInterpoleByStep(mAngleDegree, step);
    }
}

/**
 * Removes the angle range of a camera limit rail.
 */
void CameraAngleVerticalCtrl::resetRailAngleDegreeRange() {
    mRailRange->isValid = false;
}

/**
 * Starts controlling the angle in water.
 * @param step The number of steps to wait for input.
 */
void CameraAngleVerticalCtrl::startWaterCtrl(s32 step) {
    mCtrlState->type = CtrlType::Water;
    mWaterCtrlStep = step;
}

/**
 * Disables the automatic reset of low angles.
 */
void CameraAngleVerticalCtrl::invalidateAutoResetLowAngleV() {
    mParam->isInvalidAutoLowAngleReset = true;
}

/**
 * Ends snapping the angle.
 */
void CameraAngleVerticalCtrl::endSnap() {
    if (al::isNerve(this, &NrvCameraAngleVerticalCtrlSnapStart)) {
        al::setNerve(this, &NrvCameraAngleVerticalCtrlUserCtrl);
        return;
    }

    if (al::isNerve(this, &NrvCameraAngleVerticalCtrlSnap)) {
        al::setNerve(this, &NrvCameraAngleVerticalCtrlSnapEnd);
    }
}

/**
 * Calculates how much the stick turns the angle this step.
 * @return The angle in degrees.
 */
inline f32 CameraAngleVerticalCtrl::calcStickRotateDegree() const {
    if (al::isNearZero(mInfo.mStick.y, CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
        return 0.0f;
    }

    f32 speed = CameraPoserFollowLimitFunction::calcRotateSpeedDegree(mInfo.mStickSensitivityLevel);
    f32 rate = al::normalizeAbs(mInfo.mStick.y,
                                CameraPoserFollowLimitFunction::getRotateStickThreshold(), 1.0f);
    return -(speed * rate * mInfo.mRotationScaler);
}

/**
 * Moves the angle towards the target angle, which is kept above the lowest angle limit.
 */
inline void CameraAngleVerticalCtrl::updateAngleByTarget() {
    f32 target = mTargetAngleDegree;
    if (mIsValidLowDegreeLimit && target < mLowDegreeLimit) {
        target = mLowDegreeLimit;
        mTargetAngleDegree = target;
    }

    mAngleDegree = al::lerpValue(cAngleChaseRate, mAngleDegree, target);
}

/**
 * Checks whether the stick is tilted up or down.
 * @return True if the stick is tilted.
 */
inline bool CameraAngleVerticalCtrl::isStickInputV() const {
    return mInfo.mStick.y < -0.15f || mInfo.mStick.y > 0.15f;
}

/**
 * Lets the user turn the angle.
 */
void CameraAngleVerticalCtrl::exeUserCtrl() {
    if (al::isFirstStep(this)) {
        mAutoResetStep = -1;
        mIsMovingAtAutoReset = false;
        mAutoLookAngleDegree = mAngleDegree;
    }

    f32 moveDistance = (mInfo.mTargetTrans - mPrevTargetTrans).length();
    bool isMoving = moveDistance > 5.0f;
    bool isStickInput =
        !al::isNearZero(mInfo.mStick, CameraPoserFollowLimitFunction::getRotateStickThreshold());

    if (!mIsCancelAutoReset && !mInfo.mIsParallel2D && !mParam->isValidAutoLook &&
        !mParam->isInvalidAutoLowAngleReset && !mIsWaterCtrl) {
        if (mInfo.mIsOnGround &&
            5.0f - al::calcAngleDegree(sead::Vector3f::ey, mInfo.mGroundNormal) > mAngleDegree &&
            !isStickInput && moveDistance > 5.0f) {
            if ((mIsMovingAtAutoReset ? 120 : 20) <= mAutoResetStep++) {
                startTargetInterpole(getDefaultAngleDegree());
                if (al::isNerve(this, &NrvCameraAngleVerticalCtrlInterp)) {
                    al::setNerve(this, &NrvCameraAngleVerticalCtrlInterpAutoReset);
                }

                return;
            }
        } else {
            mAutoResetStep = 0;
            mIsMovingAtAutoReset = isMoving;
        }
    }

    if (!mInfo.mIsInInk) {
        f32 rotate = calcStickRotateDegree();
        if (rotate <= 0.0f) {
            sead::Vector3f dir;
            f32 radH = sead::Mathf::deg2rad(mPoser->getAngleH());
            dir.x = sinf(radH) * cosf(sead::Mathf::deg2rad(mAngleDegree + -5.0f));
            dir.y = sinf(sead::Mathf::deg2rad(mAngleDegree + -5.0f));
            dir.z = cosf(radH) * cosf(sead::Mathf::deg2rad(mAngleDegree + -5.0f));

            sead::Vector3f cameraPos = dir * mPoser->calcDistance() + mPoser->getAt();
            cameraPos.y -= mPoser->getAbsorbHeight();

            if (al::isInInk(mPoser, cameraPos, 300.0f) ||
                (mInfo.mIsSnapShotMode && rc::isInWaterAreaNoSink(mPoser, cameraPos))) {
                mAngleDegree = mPrevAngleDegree;
                mTargetAngleDegree = mPrevTargetAngleDegree;
                rotate = 0.0f;
            }
        }

        mPrevAngleDegree = mAngleDegree;
        mPrevTargetAngleDegree = mTargetAngleDegree;

        if (al::isNearZero(rotate, CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
            if (mParam->isValidAutoLook) {
                sead::Vector3f dir = mPrevTargetTrans - mInfo.mTargetTrans;
                if (al::tryNormalizeOrZero(&dir)) {
                    f32 autoLookAngle = mAutoLookAngleDegree;
                    f32 angle = sead::Mathf::rad2deg(asinf(sead::Mathf::clamp(dir.y, -1.0f, 1.0f)));
                    f32 target = clampAngleDegree(
                        al::lerpValue(cTargetChaseRate, autoLookAngle, angle));
                    mAutoLookAngleDegree =
                        al::lerpValue(cAngleChaseRate, mAutoLookAngleDegree, target);
                    mTargetAngleDegree = al::lerpValue(cTargetChaseRate, mTargetAngleDegree,
                                                       mAutoLookAngleDegree);
                }
            }
        } else {
            mTargetAngleDegree = clampAngleDegree(
                al::lerpValue(cTargetChaseRate, mTargetAngleDegree, rotate + mTargetAngleDegree));
            if (mIsValidHiDegreeLimit && mTargetAngleDegree > mHiDegreeLimit) {
                mTargetAngleDegree = mHiDegreeLimit;
            }
        }
    }

    if (mInfo.mIsChaseSubTarget) {
        f32 subTargetAngle = sead::Mathf::clamp(mInfo.mSubTargetAngle, mTargetAngleDegree - 7.5f,
                                                mTargetAngleDegree + 7.5f);
        f32 target = al::lerpValue(mInfo.mSubTargetRate, mTargetAngleDegree, subTargetAngle);
        mTargetAngleDegree = al::lerpValue(cAngleChaseRate, mTargetAngleDegree, target);
    }

    updateAngleByTarget();
}

/**
 * Sets whether the camera is under water.
 * @param isUnderWater Whether the camera is under water.
 */
void CameraAngleVerticalCtrl::setIsCameraUnderWater(bool isUnderWater) {
    mIsCameraUnderWater = isUnderWater;
}

/**
 * Sets whether the angle is kept above the horizon in water.
 * @param isLimit Whether the angle is kept above the horizon.
 */
void CameraAngleVerticalCtrl::limitNegativeVerticalAngle(bool isLimit) {
    mIsLimitNegativeAngle = isLimit;
}

/**
 * Lets the user turn the angle while the target is in water.
 */
void CameraAngleVerticalCtrl::exeWaterCtrl() {
    if (al::isFirstStep(this)) {
        mTargetAngleDegree = mAngleDegree;
    }

    sead::Vector3f moveDir = mInfo.mTargetTrans - mInfo.mPrevTargetTrans;
    if (al::tryNormalizeOrZero(&moveDir) && al::isNearZero(moveDir.y, 0.3f) &&
        !al::isNearZero(sead::Vector2f(moveDir.x, moveDir.z).length(), 0.001f)) {
        mWaterMoveStep++;
    } else {
        mWaterMoveStep = 0;
    }

    f32 rotate = calcStickRotateDegree();
    bool isNoStickInput =
        al::isNearZero(rotate, CameraPoserFollowLimitFunction::getRotateStickThreshold());

    if (al::isFirstStep(this) || !isNoStickInput || mWaterCtrlStep > 0) {
        if (!isNoStickInput) {
            mWaterCtrlStep = 120;
        } else if (mWaterCtrlStep > 0) {
            mWaterCtrlStep--;
        }
    }

    f32 target = rotate + mTargetAngleDegree;
    f32 min;
    if (mIsCameraUnderWater && !mIsLimitNegativeAngle) {
        min = sead::Mathf::clampMin(getRangeMinDegree(), cUnderWaterRangeMin);
    } else if (!mIsCameraUnderWater && mIsLimitNegativeAngle) {
        min = sead::Mathf::clampMin(sead::Mathf::clampMin(getRangeMinDegree(), -15.0f), 5.0f);
    } else {
        min = sead::Mathf::clampMin(getRangeMinDegree(), -15.0f);
    }

    min = sead::Mathf::clampMin(min, cAngleLimitMin);
    f32 max = sead::Mathf::clampMax(sead::Mathf::clampMax(getRangeMaxDegree(), cDefaultRangeMax),
                                    cAngleLimitMax);

    mTargetAngleDegree = al::lerpValue(cTargetChaseRate, mTargetAngleDegree,
                                       sead::Mathf::clamp(target, min, max));
    updateAngleByTarget();
}

/**
 * Turns the angle while the target is a flying hacked enemy.
 */
void CameraAngleVerticalCtrl::exeHackFlyerCtrl() {
    if (al::isFirstStep(this)) {
        mHackFlyerCtrl->reset();
    }

    f32 rotate = calcStickRotateDegree();

    if (al::isGreaterEqualStep(this, 5)) {
        if (!mInfo.mIsOnGround && al::isInRange(mTargetAngleDegree, 10.0f, 45.0f) &&
            al::isNearZero(rotate, CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
            if (mHackFlyerCtrl->waitStep > 0) {
                mHackFlyerCtrl->waitStep--;
            }

            mHackFlyerCtrl->resetStep = 0;

            if (mHackFlyerCtrl->waitStep <= 0) {
                f32 moveY = mInfo.mTargetTrans.y - mPrevTargetTrans.y;
                f32 speed = sead::Mathf::clamp((mHackFlyerCtrl->heightOffset + moveY) * 0.9f,
                                               -10.0f, 10.0f);
                mHackFlyerCtrl->heightSpeed =
                    al::lerpValue(0.2f, mHackFlyerCtrl->heightSpeed, speed);
                mHackFlyerCtrl->heightOffset = al::lerpValue(
                    0.2f, mHackFlyerCtrl->heightOffset, mHackFlyerCtrl->heightSpeed);

                f32 angle = al::normalizeAbs(mHackFlyerCtrl->heightOffset, 0.0f, 10.0f) * -0.75f;
                rotate = sead::Mathf::clamp(angle, 10.0f - mTargetAngleDegree,
                                            45.0f - mTargetAngleDegree);
                if (!al::isNearZero(rotate,
                                    CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
                    mHackFlyerCtrl->isNeedReset = true;
                }
            }
        } else {
            mHackFlyerCtrl->reset();
            mHackFlyerCtrl->waitStep = 30;

            if (mHackFlyerCtrl->isNeedReset && al::isNearZero(rotate, 0.1f)) {
                mHackFlyerCtrl->resetStep++;
                if (mHackFlyerCtrl->resetStep >= 30) {
                    f32 rate = al::normalize(static_cast<f32>(mHackFlyerCtrl->resetStep - 30),
                                             0.0f, 90.0f);
                    rotate = rate * sead::Mathf::clamp(mParam->defaultAngle - mTargetAngleDegree,
                                                       -0.75f, 0.75f);
                    if (mHackFlyerCtrl->resetStep >= 120) {
                        mHackFlyerCtrl->isNeedReset = false;
                    }
                }
            } else {
                mHackFlyerCtrl->resetStep = 0;
                mHackFlyerCtrl->isNeedReset = false;
            }
        }
    }

    mTargetAngleDegree = al::lerpValue(cTargetChaseRate, mTargetAngleDegree,
                                       clampAngleDegree(rotate + mTargetAngleDegree));
    mAngleDegree = al::lerpValue(cAngleChaseRate, mAngleDegree, mTargetAngleDegree);
}

/**
 * Interpolates the angle to the target angle.
 */
void CameraAngleVerticalCtrl::exeInterp() {
    f32 rate = al::easeInOut(al::normalize(static_cast<f32>(al::getNerveStep(this) + 1), 0.0f,
                                           static_cast<f32>(mInterpStep)));
    mAngleDegree = al::lerpValue(rate, mStartAngleDegree, mTargetAngleDegree);

    f32 rotate = calcStickRotateDegree();
    if (al::isNearZero(rotate, 0.001f)) {
        if (al::isGreaterEqualStep(this, mInterpStep - 1)) {
            setCtrlNerve();
        }

        return;
    }

    bool isAutoReset = al::isNerve(this, &NrvCameraAngleVerticalCtrlInterpAutoReset);
    if (rotate < 0.0f && isAutoReset) {
        mIsCancelAutoReset = true;
    }

    if (!mIsValidLowDegreeLimit || mTargetAngleDegree > mLowDegreeLimit) {
        mTargetAngleDegree = mAngleDegree;
    }

    if (mIsValidLowDegreeLimit && mTargetAngleDegree < mLowDegreeLimit) {
        mTargetAngleDegree = mLowDegreeLimit;
    }

    setCtrlNerve();
}

/**
 * Snaps the angle towards the snap angle.
 */
void CameraAngleVerticalCtrl::exeSnapStart() {
    if (al::isFirstStep(this)) {
        mStartAngleDegree = mAngleDegree;
        if (mIsSnapAfterClimb) {
            mTargetAngleDegree = mSnapAngleDegree;
        }
    }

    s32 step = al::getNerveStep(this);
    if (mIsSnapAfterClimb) {
        mAngleDegree = al::lerpValue(0.05f, mAngleDegree, mTargetAngleDegree);
    } else {
        f32 rate = al::easeInOut(static_cast<f32>(step + 1) / cSnapStep);
        mTargetAngleDegree = al::lerpValue(rate, mStartAngleDegree, mSnapAngleDegree);
        mAngleDegree = al::lerpValue(cAngleChaseRate, mAngleDegree, mTargetAngleDegree);
    }

    if (!mIsSnapAfterClimb && isStickInputV()) {
        setCtrlNerve();
        return;
    }

    if (al::isGreaterEqualStep(this, cSnapStep - 1)) {
        if (mIsSnapAfterClimb) {
            mIsSnapAfterClimb = false;
            al::setNerve(this, &NrvCameraAngleVerticalCtrlUserCtrl);
        } else {
            al::setNerve(this, &NrvCameraAngleVerticalCtrlSnap);
        }
    }
}

/**
 * Keeps the angle at the snap angle.
 */
void CameraAngleVerticalCtrl::exeSnap() {
    mAngleDegree = al::lerpValue(cAngleChaseRate, mAngleDegree, mTargetAngleDegree);
    if (isStickInputV()) {
        setCtrlNerve();
    }
}

/**
 * Snaps the angle back to the default angle.
 */
void CameraAngleVerticalCtrl::exeSnapEnd() {
    if (al::isFirstStep(this)) {
        mStartAngleDegree = mTargetAngleDegree;
    }

    f32 rate = al::easeInOut(static_cast<f32>(al::getNerveStep(this) + 1) / cSnapStep);
    mTargetAngleDegree = al::lerpValue(rate, mStartAngleDegree, mParam->defaultAngle);
    mAngleDegree = al::lerpValue(cAngleChaseRate, mAngleDegree, mTargetAngleDegree);

    if (isStickInputV()) {
        setCtrlNerve();
        return;
    }

    if (al::isGreaterEqualStep(this, cSnapStep - 1)) {
        setCtrlNerve();
        mIsSnapAfterClimb = false;
    }
}


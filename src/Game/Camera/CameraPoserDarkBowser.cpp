#include "Camera/CameraPoserDarkBowser.hpp"

#include <attributes.h>
#include <cmath>
#include <gfx/seadCamera.h>
#include <math/seadMathCalcCommon.h>

#include "Util/AreaObjUtil.hpp"
#include "Library/Camera/CameraPoserFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraVerticalAbsorber.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Camera/CameraAngleCtrlInfo.hpp"
#include "Project/Camera/CameraOffsetPreset.hpp"

namespace {

/// The follow rates of the vertical absorber of the camera.
f32 sVerticalAbsorberFollowRate[2] = {0.01f, 0.0f};

// The nerve keeper of a camera poser holds the poser itself, not its IUseNerve base.
#define POSER_NERVE_DECL(Class, Action)                                                            \
    class Class##Nrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            static_cast<Class*>(static_cast<void*>(pKeeper->getParent<al::IUseNerve>()))           \
                ->exe##Action();                                                                   \
        }                                                                                          \
    };

POSER_NERVE_DECL(CameraPoserDarkBowser, Auto)
POSER_NERVE_DECL(CameraPoserDarkBowser, AutoInput)
POSER_NERVE_DECL(CameraPoserDarkBowser, AerialEnd)
POSER_NERVE_DECL(CameraPoserDarkBowser, Aerial)
POSER_NERVE_DECL(CameraPoserDarkBowser, Focus)
POSER_NERVE_DECL(CameraPoserDarkBowser, FreeLook)

// The nerves of this camera are not constant: they are merged with the follow rate above.
#define NERVE_MAKE_MUTABLE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

FOR_EACH(NERVE_MAKE_MUTABLE, CameraPoserDarkBowser, Auto, AutoInput, AerialEnd, Aerial, Focus,
         FreeLook)

/**
 * Calculates the rotate speed rate of a stick sensitivity level.
 * @param level The stick sensitivity level.
 * @return The rate of the rotate speed.
 */
inline f32 calcStickSensitivityRate(s32 level) {
    if (level < -1) {
        return 0.7f;
    }

    switch (level) {
    case -1:
        return 0.85f;
    case 0:
        return 1.0f;
    case 1:
        return 1.2f;
    default:
        return 1.4f;
    }
}

}  // namespace

/**
 * Creates the camera.
 * @param pName The camera name.
 * @param pAxisPos The position of Fury Bowser that the camera circles around.
 */
CameraPoserDarkBowser::CameraPoserDarkBowser(const char* pName, const sead::Vector3f* pAxisPos)
    : al::CameraPoser_RS(pName), mAxisPosPtr(pAxisPos) {}

/**
 * Sets up the nerve and the camera helpers.
 */
void CameraPoserDarkBowser::init() {
    initNerve(&NrvCameraPoserDarkBowserAuto, 0);
    initLocalInterpole();
    initLookAtInterpole(0.2f);
    alCameraPoserFunction::initCameraVerticalAbsorber(this);
    alCameraPoserFunction::initCameraAngleCtrl(this);
    alCameraPoserFunction::initCameraDefaultAngleRangeV(this, 0.0f, 85.0f);
    alCameraPoserFunction::initCameraArrowCollider(this);
    mOffsetPreset = new al::CameraOffsetPreset();
    alCameraPoserFunction::initSnapShotCameraCtrlZoomRollMove(this, true, true);
    getCameraVerticalAbsorber()->setFollowRate(sVerticalAbsorberFollowRate);
}

/**
 * Does nothing, the camera is not placed in the stage.
 * @param rInfo Placement info of the camera.
 */
void CameraPoserDarkBowser::initByPlacementObj(const al::PlacementInfo& rInfo) {}

/**
 * Loads the camera parameters.
 * @param rIter Camera parameter iterator.
 */
void CameraPoserDarkBowser::loadParam(const al::ByamlIter& rIter) {
    mOffsetPreset->loadParam(rIter);
    al::tryGetByamlF32(&mMarginAngleH, rIter, "MarginAngleH");
    al::tryGetByamlF32(&mDistance, rIter, "Distance");

    if (!al::tryGetByamlF32(&mDistanceNear, rIter, "DistanceNear")) {
        mDistanceNear = mDistance;
    }

    if (!al::tryGetByamlF32(&mDistanceOut, rIter, "DistanceOut")) {
        mDistanceOut = mDistance;
    }

    // NOTE: the original code writes to the wrong member here.
    if (!al::tryGetByamlF32(&mDistanceNearOut, rIter, "DistanceNearOut")) {
        mDistanceNear = mDistanceOut;
    }

    al::tryGetByamlF32(&mSwitchToFollowDistance, rIter, "SwitchToFollowDistance");
    al::tryGetByamlF32(&mInterpRotateSpeedByFrame, rIter, "InterpRotateSpeedByFrame");
    mIsValidVelocityOffset = true;
    mIsClampInMarginInputOffsetAngleH =
        al::tryGetByamlKeyBoolOrFalse(rIter, "IsClampInMarginInputOffsetAngleH");
    al::tryGetByamlV3f(&mLocalAxisPos, rIter, "AxisPos");
}

/**
 * Places the camera behind the target, keeping the axis inside the horizontal margin.
 * @param rInfo Start info.
 */
void CameraPoserDarkBowser::start(const al::CameraStartInfo& rInfo) {
    mResetStep = -1;
    mResetStepNum = -1;
    mInputSpeedH = 0.0f;
    mVelocityOffset = 0.0f;
    mVelocityOffsetTarget = 0.0f;
    mCurrentMarginAngleH = mUserMarginAngleH >= 0.0f ? mUserMarginAngleH : calcMarginH();
    mIsFocus = false;
    alCameraPoserFunction::calcTargetTrans(&mPrevTargetTrans, this);
    mAt = mPrevTargetTrans + calcOffset();
    sead::Vector3f preDirH = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcPreCameraDirH(&preDirH, this);
    sead::Vector3f dirH = {0.0f, 0.0f, 0.0f};
    calcAxisPos(&mPrevAxisPos);

    if (al::calcDirH(&dirH, mPrevAxisPos, mAt)) {
        dirH.set(preDirH);
    }

    mAngleH = sead::Mathf::rad2deg(atan2f(preDirH.x, preDirH.z));
    mTargetAngleH = sead::Mathf::rad2deg(atan2f(dirH.x, dirH.z));
    f32 diff = al::diffNearAngleDegree(mTargetAngleH, mAngleH);

    if (!al::isInRange(diff, mCurrentMarginAngleH * -0.5f, mCurrentMarginAngleH * 0.5f)) {
        f32 halfMargin = mCurrentMarginAngleH * 0.5f;
        mAngleH = al::wrapValue(mAngleH + (diff > 0.0f ? halfMargin - diff : -diff - halfMargin),
                                360.0f);
    }

    sead::Vector3f dir;
    calcDirHV(&dir, mAngleH, alCameraPoserFunction::getCameraAngleV(this));
    mEye = mAt + dir * calcDistance();
    mTowerEye.set(mEye);
    al::setNerve(this, &NrvCameraPoserDarkBowserAuto);
}

/**
 * @return The horizontal margin angle, narrowed while the target is standing still.
 */
f32 CameraPoserDarkBowser::calcMarginH() const {
    return al::lerpValue(mMarginStep / 30.0f, mMarginAngleH, mMarginAngleH * 0.4f);
}

/**
 * @return Offset of the look at position, interpolated while zooming in.
 */
sead::Vector3f CameraPoserDarkBowser::calcOffset() {
    if (!mIsZoomIn) {
        return mOffsetPreset->getOffset();
    }

    sead::Vector3f offset = mOffsetPreset->getOffset();
    f32 rate = mZoomFrame == 0 ?
                   1.0f :
                   sead::Mathf::clamp(static_cast<f32>(mZoomStep) / mZoomFrame, 0.0f, 1.0f);
    offset.y = offset.y * (1.0f - rate) + rate * mZoomOffsetY;
    return offset;
}

/**
 * Calculates the position of the axis the camera circles around.
 * @param pAxisPos Where the axis position is written.
 */
void CameraPoserDarkBowser::calcAxisPos(sead::Vector3f* pAxisPos) const {
    if (mAxisPosPtr != nullptr) {
        pAxisPos->set(*mAxisPosPtr);
    } else if (mIsSetAxisPos) {
        pAxisPos->set(mAxisPos);
    } else {
        pAxisPos->setMul(getViewMtx(), mLocalAxisPos);
    }
}

/**
 * Calculates the camera direction from its angles.
 * @param pDir Where the direction is written.
 * @param angleH Horizontal angle in degrees.
 * @param angleV Vertical angle in degrees.
 */
void CameraPoserDarkBowser::calcDirHV(sead::Vector3f* pDir, f32 angleH, f32 angleV) const {
    pDir->set(sinf(sead::Mathf::deg2rad(angleH)) * cosf(sead::Mathf::deg2rad(angleV)),
              sinf(sead::Mathf::deg2rad(angleV)),
              cosf(sead::Mathf::deg2rad(angleH)) * cosf(sead::Mathf::deg2rad(angleV)));
    al::normalize(pDir);
}

/**
 * @return Camera distance depending on the vertical angle, interpolated while zooming in.
 */
f32 CameraPoserDarkBowser::calcDistance() {
    if (al::isNear(getDist(), getDistNear(), 0.001f)) {
        return getDist();
    }

    f32 distance = getDist();
    f32 distanceNear = getDistNear();

    if (mIsZoomIn) {
        f32 rate = mZoomFrame < 1 ? 1.0f :
                                    sead::Mathf::clamp(static_cast<f32>(mZoomStep) / mZoomFrame,
                                                       0.0f, 1.0f);
        distance = distance * (1.0f - rate) + rate * mZoomDistance;
        distanceNear = distanceNear * (1.0f - rate) + rate * mZoomDistanceNear;

        if (!alCameraPoserFunction::isSnapShotMode(this)) {
            mZoomStep++;
        }
    }

    const al::CameraAngleCtrlInfo* angleCtrlInfo = getAngleCtrlInfo();
    f32 minAngleV = angleCtrlInfo->getDefaultMinAngleV();
    f32 rangeV = angleCtrlInfo->getDefaultMaxAngleV() - minAngleV;
    f32 rateV = (angleCtrlInfo->getAngleV() - minAngleV) / rangeV;
    return distance * rateV + distanceNear * (1.0f - rateV);
}

/**
 * Stops the vertical absorber while the snapshot mode is active.
 */
void CameraPoserDarkBowser::startSnapShotMode() {
    alCameraPoserFunction::stopUpdateVerticalAbsorb(this);
}

/**
 * Restarts the vertical absorber after the snapshot mode.
 */
void CameraPoserDarkBowser::endSnapShotMode() {
    alCameraPoserFunction::restartUpdateVerticalAbsorb(this);
}

/**
 * Sets the horizontal margin angle and the vertical angle.
 * @param marginAngleH Horizontal margin angle.
 * @param angleV Vertical angle.
 */
void CameraPoserDarkBowser::setParams(f32 marginAngleH, f32 angleV) {
    mMarginAngleH = marginAngleH;
    getAngleCtrlInfo()->setAngleV(angleV);
}

/**
 * Goes back to the automatic camera if it is not turned by the player.
 */
void CameraPoserDarkBowser::requestAutoCamera() {
    if (al::isNerve(this, &NrvCameraPoserDarkBowserAuto) ||
        al::isNerve(this, &NrvCameraPoserDarkBowserAutoInput) ||
        al::isNerve(this, &NrvCameraPoserDarkBowserAerialEnd)) {
        return;
    }

    if (!al::isNearZero(mInputSpeedH, 0.1f)) {
        return;
    }

    mIsAuto = true;
    al::setNerve(this, &NrvCameraPoserDarkBowserAuto);
}

/**
 * Lets the camera move in while the target is on the ground.
 */
void CameraPoserDarkBowser::requestCameraIn() {
    mIsCameraIn = true;
}

/**
 * Keeps the camera out.
 */
void CameraPoserDarkBowser::requestCameraOut() {
    mIsCameraIn = false;
}

/**
 * Does nothing.
 * @param rTarget Unused.
 */
void CameraPoserDarkBowser::requestLookAtStaticTarget(const sead::Vector3f& rTarget) {}

/**
 * Starts the aerial camera.
 */
void CameraPoserDarkBowser::requestAerialCamera() {
    if (al::isNerve(this, &NrvCameraPoserDarkBowserAerial) ||
        al::isNerve(this, &NrvCameraPoserDarkBowserAerialEnd)) {
        return;
    }

    al::setNerve(this, &NrvCameraPoserDarkBowserAerial);
}

/**
 * Ends the aerial camera.
 */
void CameraPoserDarkBowser::endAerialCamera() {
    if (al::isNerve(this, &NrvCameraPoserDarkBowserAerial)) {
        al::setNerve(this, &NrvCameraPoserDarkBowserAerialEnd);
    }
}

/**
 * Toggles focusing on a fixed pose.
 * @param rLookAt The look at position while focusing.
 * @param rCameraPos The camera position while focusing.
 */
void CameraPoserDarkBowser::toggleFocus(const sead::Vector3f& rLookAt,
                                        const sead::Vector3f& rCameraPos) {
    mFocusLookAt.x = rLookAt.x;
    mFocusLookAt.y = rLookAt.y;
    mFocusLookAt.z = rLookAt.z;
    mFocusCameraPos.x = rCameraPos.x;
    mFocusCameraPos.y = rCameraPos.y;
    mFocusCameraPos.z = rCameraPos.z;
    mIsFocus = !mIsFocus;

    if (mIsFocus) {
        al::setNerve(this, &NrvCameraPoserDarkBowserFocus);
    } else {
        al::setNerve(this, &NrvCameraPoserDarkBowserAuto);
    }
}

/**
 * @return The maximum rate of the follow speed, raised while the target is standing still.
 */
f32 CameraPoserDarkBowser::calcMaxMarginRate() const {
    return al::lerpValue(mMarginStep / 30.0f, 0.2f, 0.4f);
}

/**
 * Tilts the camera towards the water surface while the camera is in water.
 */
ALWAYS_INLINE inline void CameraPoserDarkBowser::updateAngleVAboveWater() {
    f32 angleV = alCameraPoserFunction::getCameraAngleV(this);
    sead::Vector3f checkPos = mEye - sead::Vector3f::ey * 200.0f;
    al::AreaObj* waterArea = rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea, checkPos);

    if (waterArea != nullptr) {
        sead::Vector3f hitPos;
        sead::Vector3f normal;
        waterArea->getAreaShape()->checkArrowCollision(
            &hitPos, &normal, sead::Vector3f::ey * 3000.0f + mEye, checkPos);
        hitPos.y += 200.0f;
        f32 sign = hitPos.y - mAt.y < 0.0f ? -1.0f : 1.0f;
        // The normal is not used, its vector is reused for the horizontal look direction.
        sead::Vector3f& lookDirH = normal;
        lookDirH.set(mAt.x - mEye.x, 0.0f, mAt.z - mEye.z);
        al::normalizeOrDirZ(&lookDirH);
        sead::Vector3f& surfaceDir = hitPos;
        surfaceDir = mAt - hitPos;
        surfaceDir.normalize();
        f32 angle;
        al::tryCalcAngleDegree(&angle, surfaceDir, lookDirH);
        alCameraPoserFunction::setCameraAngleV(this, (angleV + sign * angle) * 0.5f);
    }
}

/**
 * Updates the vertical angle above water, the look at position and the horizontal angle input
 * before running the nerve.
 */
void CameraPoserDarkBowser::movement() {
    if (mIsFocus) {
        al::CameraPoser_RS::movement();
        return;
    }

    updateAngleVAboveWater();
    sead::Vector3f velocity;
    alCameraPoserFunction::calcTargetVelocity(&velocity, this);

    if (!alCameraPoserFunction::isSnapShotMode(this)) {
        if (velocity.squaredLength() < 1000.0f) {
            if (mMarginStep < 30) {
                mMarginStep++;
            }
        } else if (mMarginStep > 0) {
            mMarginStep--;
        }
    }

    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetTrans(&targetTrans, this);
    mAt.set(targetTrans + calcOffset());
    sead::Vector3f axisPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f dirH = {0.0f, 0.0f, 0.0f};
    calcAxisPos(&axisPos);

    if (!al::calcDirH(&dirH, axisPos, mAt)) {
        mTargetAngleH = sead::Mathf::rad2deg(atan2f(dirH.x, dirH.z));
    }

    if (!isResetting()) {
        f32 speed = calcStickRotateSpeedWithFrictionH();
        f32 inputSpeedH = al::lerpValue(0.2f, mInputSpeedH, speed);
        mInputSpeedH = al::lerpValue(0.5f, mInputSpeedH, inputSpeedH);
    }

    f32 marginAngleH = mUserMarginAngleH >= 0.0f ? mUserMarginAngleH : calcMarginH();
    f32 currentMarginAngleH = al::lerpValue(0.2f, mCurrentMarginAngleH, marginAngleH);
    mCurrentMarginAngleH = al::lerpValue(0.1f, mCurrentMarginAngleH, currentMarginAngleH);
    al::CameraPoser_RS::movement();
    mPrevTargetTrans.set(targetTrans);
    mPrevAxisPos.set(axisPos);
}

/**
 * @return The horizontal rotate speed by the stick, scaled by the stick sensitivity.
 */
f32 CameraPoserDarkBowser::calcStickRotateSpeedWithFrictionH() const {
    sead::Vector2f stick = {0.0f, 0.0f};
    alCameraPoserFunction::calcCameraRolledRotateStick(&stick, this);
    f32 stickH = stick.x;

    if (al::isNearZero(stickH, 0.3f)) {
        return 0.0f;
    }

    f32 rateV = al::normalize(alCameraPoserFunction::getCameraAngleV(this), 60.0f, 75.0f);
    f32 maxSpeed = al::lerpValue(rateV, 1.0f, 0.75f) * 1.8f;
    maxSpeed *= calcStickSensitivityRate(alCameraPoserFunction::getStickSensitivityLevel(this));
    f32 sensitivityRate =
        calcStickSensitivityRate(alCameraPoserFunction::getStickSensitivityLevel(this));
    return -(maxSpeed * al::normalizeAbs(stickH, 0.3f, 1.0f / sensitivityRate));
}

/**
 * Moves the camera in or out and places it around the look at position by the interpolated
 * horizontal angle.
 */
void CameraPoserDarkBowser::update() {
    bool isMoveIn = mIsCameraIn ? alCameraPoserFunction::isTargetCollideGround(this) : false;

    if (!alCameraPoserFunction::isSnapShotMode(this)) {
        if (isMoveIn) {
            if (mCameraInStep > 0) {
                mCameraInStep--;
            }
        } else if (mCameraInStep < 120) {
            mCameraInStep++;
        }
    }

    if (mIsFocus) {
        return;
    }

    if (alCameraPoserFunction::isTriggerCameraResetRotate(this) && mResetSpeed - 0.1f < 4.0f) {
        resetInputRotate(4.0f, 30);
    }

    sead::Vector3f dir;
    calcDirHV(&dir, mAngleH, alCameraPoserFunction::getCameraAngleV(this));
    mTowerEye = mAt + dir * calcDistance();

    if (isResetting()) {
        f32 prevRate = al::easeOut(al::easeInOut(al::normalize(
            static_cast<f32>(sead::Mathi::max(mResetStep - 1, 0)), 0.0f,
            static_cast<f32>(mResetStepNum))));
        f32 rate = al::easeOut(al::easeInOut(al::normalize(static_cast<f32>(mResetStep), 0.0f,
                                                           static_cast<f32>(mResetStepNum))));
        mInterpAngleH = al::lerpDegree(mInterpAngleH, mAngleH,
                                       al::normalize(rate - prevRate, 0.0f, 1.0f - prevRate));

        if (!alCameraPoserFunction::isSnapShotMode(this)) {
            mResetStep++;
        }

        if (!isResetting()) {
            mResetStep = -1;
            mResetStepNum = -1;
        }
    } else {
        mInterpAngleH = mAngleH;
    }

    if (al::isNerve(this, &NrvCameraPoserDarkBowserAerial) ||
        al::isNerve(this, &NrvCameraPoserDarkBowserAerialEnd)) {
        return;
    }

    calcDirHV(&dir, mInterpAngleH, alCameraPoserFunction::getCameraAngleV(this));
    mEye = mAt + dir * calcDistance();
}

/**
 * Rotates the camera back behind the target and goes back to the automatic camera.
 * @param speed Rotation speed in degrees per frame.
 * @param minStep Minimum number of steps of the rotation.
 */
void CameraPoserDarkBowser::resetInputRotate(f32 speed, s32 minStep) {
    startRotateInterp(mInterpAngleH, mTargetAngleH, speed, minStep);
    mTargetAngleH = mResetEndAngleH;
    mAngleH = mResetEndAngleH;
    mInputSpeedH = 0.0f;
    mInputOffsetAngleH = 0.0f;
    mIsAuto = true;

    if (!al::isNerve(this, &NrvCameraPoserDarkBowserAerial)) {
        al::setNerve(this, &NrvCameraPoserDarkBowserAuto);
    }
}

/**
 * Shifts the camera sideways by the velocity offset.
 * @param pCamera Camera to modify.
 */
void CameraPoserDarkBowser::makeLookAtCamera(sead::LookAtCamera* pCamera) const {
    if (!mIsValidVelocityOffset) {
        return;
    }

    sead::Vector3f sideDir = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcSideDir(&sideDir, this);
    sead::Vector3f offset = mVelocityOffset * sideDir;
    pCamera->addPos(offset);
    pCamera->addAt(offset);
}

/**
 * Turns the camera to keep the axis inside the horizontal margin.
 */
void CameraPoserDarkBowser::exeAuto() {
    if (al::isFirstStep(this)) {
        mFollowSpeedH = 0.0f;
    }

    if (isResetting()) {
        return;
    }

    sead::Vector3f dirH = {0.0f, 0.0f, 0.0f};

    if (al::calcDirH(&dirH, mAt, mTowerEye)) {
        dirH.set(sinf(sead::Mathf::deg2rad(mAngleH)), 0.0f,
                 cosf(sead::Mathf::deg2rad(mAngleH)));
    }

    f32 angleH = mAngleH;
    f32 cameraAngleH = sead::Mathf::rad2deg(atan2f(dirH.x, dirH.z));
    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetTrans(&targetTrans, this);

    if (!alCameraPoserFunction::isSnapShotMode(this)) {
        f32 halfMargin = mCurrentMarginAngleH * 0.5f;

        if (al::isNearZero(halfMargin, 0.001f)) {
            mAngleH = mTargetAngleH;
        } else {
            f32 diff = al::diffNearAngleDegree(mTargetAngleH, cameraAngleH);

            if (al::isInRange(diff, -halfMargin, halfMargin)) {
                mFollowSpeedH = mFollowSpeedH * 0.8f;
            } else {
                f32 overAngle = diff + (diff > 0.0f ? -halfMargin : halfMargin);
                f32 rate;

                if (al::isNearZero(halfMargin, 0.001f)) {
                    rate = 1.0f;
                } else {
                    rate = al::lerpValue(
                        al::normalize(sead::Mathf::abs(overAngle), 0.0f, halfMargin), 0.05f,
                        calcMaxMarginRate());
                }

                sead::Vector3f axisPos;
                calcAxisPos(&axisPos);
                f32 targetMove =
                    sead::Mathf::sqrt(sead::Mathf::square(mPrevTargetTrans.x - targetTrans.x) +
                                      sead::Mathf::square(mPrevTargetTrans.z - targetTrans.z));
                f32 axisMove =
                    sead::Mathf::sqrt(sead::Mathf::square(mPrevAxisPos.x - axisPos.x) +
                                      sead::Mathf::square(mPrevAxisPos.z - axisPos.z));
                f32 moveRate = al::normalize(sead::Mathf::max(targetMove, axisMove), -5.0f, 5.0f);
                mFollowSpeedH = moveRate * al::lerpValue(0.05f, mFollowSpeedH, overAngle * rate);
            }

            mAngleH =
                al::lerpDegree(angleH, al::wrapValue(cameraAngleH - mFollowSpeedH, 360.0f), 0.3f);
        }
    }

    if (mIsValidVelocityOffset) {
        sead::Vector3f lookDir = mEye - mAt;
        al::verticalizeVec(&lookDir, mUp, lookDir);

        if (al::tryNormalizeOrZero(&lookDir) && !al::isParallelDirection(lookDir, mUp, 0.01f)) {
            sead::Vector3f sideDir;
            sideDir.setCross(lookDir, mUp);
            al::normalize(&sideDir);
            sead::Vector3f move = targetTrans - mPrevTargetTrans;
            al::parallelizeVec(&move, sideDir, move);
            f32 dot = move.dot(sideDir);
            f32 moveRate = al::normalize(move.length(), 1.0f, 400.0f) * (dot > 0.0f ? 1.0f : -1.0f);
            f32 offset = sead::Mathf::clamp(mVelocityOffsetTarget + moveRate * 3.0f,
                                            -mVelocityOffsetMax, mVelocityOffsetMax);
            mVelocityOffsetTarget = al::lerpValue(0.7f, mVelocityOffsetTarget, offset);
            mVelocityOffset = al::lerpValue(0.2f, mVelocityOffset, mVelocityOffsetTarget);
        }
    }

    if (!mIsAuto) {
        al::setNerve(this, &NrvCameraPoserDarkBowserFreeLook);
        return;
    }

    if (!al::isNearZero(mInputSpeedH, 0.001f)) {
        al::setNerve(this, &NrvCameraPoserDarkBowserAutoInput);
    }
}

/**
 * Rotates the camera by the horizontal angle input until it reaches the margin.
 */
void CameraPoserDarkBowser::exeAutoInput() {
    if (al::isFirstStep(this)) {
        mInputOffsetAngleH = al::diffNearAngleDegree(mTargetAngleH, mInterpAngleH);
        mResetStep = -1;
        mResetStepNum = -1;
    }

    if (al::isNearZero(mInputSpeedH, 0.001f)) {
        mInputSpeedH = 0.0f;
        al::setNerve(this, &NrvCameraPoserDarkBowserAuto);
        return;
    }

    if (mIsClampInMarginInputOffsetAngleH) {
        if (!al::isNearZero(mInputSpeedH, 0.001f)) {
            f32 offsetAngle = sead::Mathf::clamp(mInputOffsetAngleH + mInputSpeedH * 1.8f,
                                                 mCurrentMarginAngleH * -0.5f,
                                                 mCurrentMarginAngleH * 0.5f);

            if (al::isNear(sead::Mathf::abs(offsetAngle), mCurrentMarginAngleH * 0.5f, 0.001f)) {
                mIsAuto = false;
                al::setNerve(this, &NrvCameraPoserDarkBowserFreeLook);
                return;
            }

            f32 rate = sead::Mathf::abs(offsetAngle / mCurrentMarginAngleH * 0.5f);
            offsetAngle = al::lerpValue(0.7f - rate, mInputOffsetAngleH, offsetAngle);
            mInputOffsetAngleH = al::lerpValue(0.5f - rate, mInputOffsetAngleH, offsetAngle);
        }
    } else {
        mInputOffsetAngleH = mInputSpeedH + mInputOffsetAngleH;
    }

    mAngleH = al::wrapValue(mTargetAngleH + mInputOffsetAngleH, 360.0f);
}

/**
 * Lets the player turn the camera freely, until the camera looks at the axis again.
 */
void CameraPoserDarkBowser::exeFreeLook() {
    if (mIsValidVelocityOffset) {
        mVelocityOffsetTarget = al::lerpValue(0.7f, mVelocityOffsetTarget, 0.0f);
        mVelocityOffset = al::lerpValue(0.2f, mVelocityOffset, mVelocityOffsetTarget);
    }

    if (checkStickyCamera()) {
        resetInputRotate(4.0f, 30);
        return;
    }

    if (isResetting()) {
        return;
    }

    sead::Vector3f dirH = {0.0f, 0.0f, 0.0f};

    if (al::calcDirH(&dirH, mAt, mTowerEye)) {
        dirH.set(sinf(sead::Mathf::deg2rad(mAngleH)), 0.0f,
                 cosf(sead::Mathf::deg2rad(mAngleH)));
    }

    mAngleH = sead::Mathf::rad2deg(atan2f(dirH.x, dirH.z));

    if (!isResetting()) {
        mAngleH = al::wrapValue(mAngleH + mInputSpeedH, 360.0f);
    }
}

/**
 * Checks whether the camera looks at Fury Bowser while the stick is released.
 * @return True if the camera should go back to the automatic camera.
 */
bool CameraPoserDarkBowser::checkStickyCamera() const {
    if (calcDistanceBetweenAxisPosAndTargetPos() >= mSwitchToFollowDistance &&
        al::isNearZero(mInputSpeedH, 0.1f)) {
        sead::Vector3f axisDir = *mAxisPosPtr + sead::Vector3f::ey * 1600.0f - mEye;
        sead::Vector3f lookDir;
        alCameraPoserFunction::calcLookDir(&lookDir, this);

        if (!al::normalizeOrZero(&axisDir) && 1.0f - axisDir.dot(lookDir) < 0.01f) {
            return true;
        }
    }

    return false;
}

/**
 * Looks at the middle of Fury Bowser and the target from above.
 */
void CameraPoserDarkBowser::exeAerial() {
    if (al::isFirstStep(this)) {
        mIsAerialAngleReached = false;
    }

    if (alCameraPoserFunction::getCameraAngleV(this) < -10.0f) {
        alCameraPoserFunction::setCameraAngleV(
            this, (alCameraPoserFunction::getCameraAngleV(this) - 10.0f) * 0.5f);
    }

    sead::Vector3f dir;
    sead::Vector3f midpoint;
    calcAerialMidpoint(&midpoint);
    mAt.set(midpoint);

    if (!isResetting()) {
        mAngleH = al::wrapValue(mAngleH + mInputSpeedH, 360.0f);
    }

    if (!mIsAerialAngleReached && alCameraPoserFunction::getCameraAngleV(this) < 30.0f) {
        alCameraPoserFunction::setCameraAngleV(this,
                                               alCameraPoserFunction::getCameraAngleV(this) + 0.5f);
    } else {
        mIsAerialAngleReached = true;
    }

    calcDirHV(&dir, mAngleH, alCameraPoserFunction::getCameraAngleV(this));
    f32 rate = sead::Mathf::clamp(al::getNerveStep(this) / 60.0f, 0.0f, 1.0f) * 0.25f;
    mEye = midpoint + dir * calcDistance() * (rate + 1.0f);
}

/**
 * Calculates the look at position of the aerial camera.
 * @param pMidpoint Where the position between Fury Bowser and the target is written.
 */
void CameraPoserDarkBowser::calcAerialMidpoint(sead::Vector3f* pMidpoint) const {
    sead::Vector3f targetTrans;
    alCameraPoserFunction::calcTargetTrans(&targetTrans, this);
    f32 maxDistance = 52000.0f;
    f32 divisor = al::isNearZero(maxDistance, 0.001f) ? 1.0f : maxDistance;
    f32 distanceH = sead::Mathf::sqrt(sead::Mathf::square(targetTrans.x - mAxisPosPtr->x) +
                                      sead::Mathf::square(targetTrans.z - mAxisPosPtr->z));
    f32 rate = sead::Mathf::clamp(distanceH / divisor, 0.0f, 1.0f) * 0.5f + 0.5f;
    *pMidpoint = *mAxisPosPtr * (1.0f - rate) + targetTrans * rate + mOffsetPreset->getOffset();
}

/**
 * Goes back from the aerial camera, then resets the angle once the target lands.
 */
void CameraPoserDarkBowser::exeAerialEnd() {
    if (al::isFirstStep(this)) {
        mIsAerialAngleReached = false;
    }

    if (alCameraPoserFunction::getCameraAngleV(this) < -10.0f) {
        alCameraPoserFunction::setCameraAngleV(
            this, (alCameraPoserFunction::getCameraAngleV(this) - 10.0f) * 0.5f);
    }

    sead::Vector3f midpoint;
    calcAerialMidpoint(&midpoint);
    mAt.set(midpoint);

    if (!mIsAerialAngleReached && alCameraPoserFunction::getCameraAngleV(this) > 8.0f) {
        alCameraPoserFunction::setCameraAngleV(this,
                                               alCameraPoserFunction::getCameraAngleV(this) - 1.0f);
    } else {
        mIsAerialAngleReached = true;
    }

    sead::Vector3f dir;
    calcDirHV(&dir, mAngleH, alCameraPoserFunction::getCameraAngleV(this));
    f32 rate = sead::Mathf::clamp(al::getNerveStep(this) / 30.0f, 0.0f, 1.0f) * 0.25f;
    mEye = midpoint + dir * calcDistance() * (1.0f - rate + 0.25f);

    if ((mIsAerialAngleReached && alCameraPoserFunction::isTargetCollideGround(this)) ||
        al::isGreaterStep(this, 180)) {
        sead::Vector2f stick = {0.0f, 0.0f};
        alCameraPoserFunction::calcCameraRolledRotateStick(&stick, this);
        resetInputRotate(4.0f, 30);
        getAngleCtrlInfo()->update(stick, alCameraPoserFunction::getStickSensitivityScale(this),
                                   true);
    }
}

/**
 * Moves the camera to the focused pose.
 */
void CameraPoserDarkBowser::exeFocus() {
    sead::Vector3f pos;
    al::lerpVec(&pos, mAt, mFocusLookAt, 0.1f);
    mAt.set(pos);
    al::lerpVec(&pos, mEye, mFocusCameraPos, 0.1f);
    mEye.set(pos);
}

/**
 * Starts zooming in.
 * @param distance Camera distance at the highest vertical angle.
 * @param distanceNear Camera distance at the lowest vertical angle.
 * @param offsetY Height offset of the look at position.
 * @param frame Number of frames of the zoom.
 * @param isUnused Unused.
 */
void CameraPoserDarkBowser::setZoomIn(f32 distance, f32 distanceNear, f32 offsetY, s32 frame,
                                      bool isUnused) {
    mZoomDistance = distance;
    mZoomDistanceNear = distanceNear;
    mZoomOffsetY = offsetY;
    mZoomFrame = frame;
    mZoomStep = 0;
    mIsZoomIn = true;
}

/**
 * Starts an interpolated rotation of the horizontal angle.
 * @param startAngleH Horizontal angle to rotate from.
 * @param endAngleH Horizontal angle to rotate to.
 * @param speed Rotation speed in degrees per frame.
 * @param minStep Minimum number of steps of the rotation.
 */
void CameraPoserDarkBowser::startRotateInterp(f32 startAngleH, f32 endAngleH, f32 speed,
                                              s32 minStep) {
    mResetStartAngleH = startAngleH;
    mResetEndAngleH = endAngleH;
    mResetStep = 0;
    mResetSpeed = speed;
    f32 diff = sead::Mathf::abs(al::diffNearAngleDegree(startAngleH, endAngleH));
    s32 step = minStep;

    if (!al::isNearZero(diff, 0.001f) && !al::isNearZero(speed, 0.001f)) {
        step = sead::Mathi::clampMin(static_cast<s32>(diff / speed), minStep);
    }

    mResetStepNum = step;
}

/**
 * @return The camera distance at the highest vertical angle, moved in on the ground.
 */
f32 CameraPoserDarkBowser::getDist() const {
    f32 rate = al::easeOut(mCameraInStep / 120.0f);
    return rate * mDistanceOut + (1.0f - rate) * mDistance;
}

/**
 * @return The camera distance at the lowest vertical angle, moved in on the ground.
 */
f32 CameraPoserDarkBowser::getDistNear() const {
    f32 rate = al::easeOut(mCameraInStep / 120.0f);
    return rate * mDistanceNearOut + (1.0f - rate) * mDistanceNear;
}

/**
 * @return Horizontal distance between the axis and the target.
 */
f32 CameraPoserDarkBowser::calcDistanceBetweenAxisPosAndTargetPos() const {
    sead::Vector3f axisPos;
    calcAxisPos(&axisPos);
    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetTrans(&targetTrans, this);
    sead::Vector3f diff = axisPos - targetTrans;
    al::verticalizeVec(&diff, mUp, diff);
    return diff.length();
}

#include "Player/Normal/PlayerActionSlide.hpp"

#include <cmath>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>

#include "Library/Math/MathUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerAudio.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerCollisionSize.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"

namespace {
/// Converts radians to degrees.
constexpr f32 cRadToDeg = 57.29578f;
/// Pi, to convert the maximum tilt from degrees to radians.
constexpr f32 cPi = 3.1415927f;

/**
 * @brief Sets the rotation part of a matrix from three axes (the translation is left as is).
 * @param pMtx The matrix to set.
 * @param rSide The x axis.
 * @param rUp The y axis.
 * @param rFront The z axis.
 */
ALWAYS_INLINE void makeMtxSideUpFront(sead::Matrix34f* pMtx, const sead::Vector3f& rSide,
                                      const sead::Vector3f& rUp, const sead::Vector3f& rFront) {
    pMtx->setBase(0, rSide);
    pMtx->setBase(1, rUp);
    pMtx->setBase(2, rFront);
}

/**
 * @brief Gets the angle between the floor and the player's up direction.
 * @param rNormal The floor normal.
 * @param pProperty The player's state.
 * @return The angle in degrees.
 */
ALWAYS_INLINE f32 calcSlopeAngle(const sead::Vector3f& rNormal, const PlayerProperty* pProperty) {
    f32 cos = rNormal.dot(pProperty->mUpDir);
    if (cos < -1.0f) {
        cos = -1.0f;
    } else if (cos > 1.0f) {
        cos = 1.0f;
    }

    return std::acos(cos) * cRadToDeg;
}

/**
 * @brief Checks if the floor is gentle enough to stop sliding on.
 * @param rNormal The floor normal.
 * @param pArg The player systems.
 * @return Whether the slope angle is at most the slide end angle.
 */
ALWAYS_INLINE bool isGentleSlope(const sead::Vector3f& rNormal, const PlayerActionArg* pArg) {
    const PlayerProperty* pProperty = pArg->mProperty;
    const PlayerConstParam* pParam = pArg->mConstParam;
    return calcSlopeAngle(rNormal, pProperty) <= pParam->getSlideSlopeEndAngle();
}

/**
 * @brief Checks if the player stands on a floor that forces sliding.
 * @param pCollision The player's collision.
 * @return Whether the floor's map code is "Slide".
 */
ALWAYS_INLINE bool isOnSlideFloor(const IUsePlayerCollision* pCollision) {
    IUsePlayerCollision::Info info;
    pCollision->getFloorInfo(&info);
    return al::isEqualString(info.mMapCode, "Slide");
}

/**
 * @brief Steers the velocity sideways with the stick, limiting the sideways speed.
 * @param pVelocity The velocity to change.
 * @param pInput The player's input.
 * @param pProperty The player's state.
 * @param accel The sideways acceleration at full stick.
 * @param maxSpeed The maximum sideways speed.
 */
NOINLINE void addSideVelocity(sead::Vector3f* pVelocity, const IUsePlayerInput* pInput,
                              const PlayerProperty* pProperty, f32 accel, f32 maxSpeed) {
    sead::Vector3f moveVec = pInput->getMoveVec();
    if (al::normalizeOrZero(&moveVec)) {
        return;
    }

    sead::Vector3f side;
    side.setCross(pProperty->mGroundUp, pProperty->mFront);
    al::normalize(&side);
    *pVelocity += side * (side.dot(moveVec) * accel);

    f32 sideSpeed = side.dot(*pVelocity);
    if (sead::Mathf::abs(sideSpeed) > maxSpeed) {
        sead::Vector3f sideVelocity = side * sideSpeed;
        *pVelocity -= sideVelocity;

        f32 length = sideVelocity.length();
        if (length > 0.0f) {
            sideVelocity *= maxSpeed / length;
        }

        *pVelocity += sideVelocity;
    }
}
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pCollisionSize The player's collision size, made short while sliding.
 */
PlayerActionSlide::PlayerActionSlide(const PlayerActionArg* pArg,
                                     IUsePlayerCollisionSize* pCollisionSize)
    : mArg(pArg), mCollisionSize(pCollisionSize) {}

/** @brief Snaps the player to the ground. */
void PlayerActionSlide::move() {
    mArg->mCollision->snapGround();
}

/** @brief Updates the velocity, the model posture and the sideways lean. */
void PlayerActionSlide::update() {
    calcVelocity();
    calcModelOffsetMtx();
    calcTilt();
}

/** @brief Accelerates down the slope (or brakes on level ground) and steers sideways. */
void PlayerActionSlide::calcVelocity() {
    PlayerProperty* pProperty;

    if (mArg->mCollision->isOnFloor()) {
        IUsePlayerCollision::Info info;
        mArg->mCollision->getFloorInfo(&info);
        PlayerActionFunc::calcDownward(&mDownward, mArg->mProperty, info.mNormal);
        mFloorNormal.e = info.mNormal.e;
    }

    PlayerActionFunc::decayVerticalVec(&mArg->mProperty->mVelocity, mDownward,
                                       mArg->mConstParam->getSlideSideBrake());

    if (!mArg->mCollision->isOnFloor() || !isGentleSlope(mFloorNormal, mArg)) {
        PlayerActionFunc::addAndAdjustVelocity(&mArg->mProperty->mVelocity, mDownward,
                                               mArg->mConstParam->getSlideAccel(),
                                               mArg->mConstParam->getSlideMaxSpeed());

        pProperty = mArg->mProperty;
        if (mDownward.dot(pProperty->mFront) > 0.5f &&
            mDownward.dot(pProperty->mVelocity) > 0.0f) {
            addSideVelocity(&pProperty->mVelocity, mArg->mInput, pProperty,
                            mArg->mConstParam->getSlideSideAccel(),
                            mArg->mConstParam->getSlideSideMaxSpeed());

            pProperty = mArg->mProperty;
            if (mDownward.dot(pProperty->mVelocity) < 0.0f) {
                al::verticalizeVec(&pProperty->mVelocity, mDownward, pProperty->mVelocity);
            }
        }

        return;
    }

    if (isOnSlideFloor(mArg->mCollision)) {
        PlayerActionFunc::decayVerticalVec(&mArg->mProperty->mVelocity, mFloorNormal,
                                           mArg->mConstParam->getForceSlideBrake());

        addSideVelocity(
            &mArg->mProperty->mVelocity, mArg->mInput, mArg->mProperty,
            mArg->mConstParam->getSlideSideAccelOnLevelLand(),
            sead::Mathf::min(mArg->mConstParam->getSlideSideMaxSpeedOnLevelLand(),
                             mArg->mProperty->mFront.dot(mArg->mProperty->mVelocity)));
    } else {
        PlayerActionFunc::decayVerticalVec(&mArg->mProperty->mVelocity, mFloorNormal,
                                           mArg->mConstParam->getSlideBrake());
    }
}

/** @brief Turns the model to lie along the slope, blending from its current posture. */
void PlayerActionSlide::calcModelOffsetMtx() {
    const PlayerProperty* pProperty = mArg->mProperty;
    sead::Vector3f side;
    side.setCross(pProperty->mGroundUp, pProperty->mFront);

    sead::Matrix34f baseMtx;
    makeMtxSideUpFront(&baseMtx, side, pProperty->mGroundUp, pProperty->mFront);
    sead::Matrix34f currentMtx = pProperty->mTurnMtx;
    currentMtx.setMul(baseMtx, currentMtx);
    sead::Vector3f currentFront;
    currentMtx.getBase(currentFront, 2);

    sead::Vector3f downward;
    al::verticalizeVec(&downward, mFloorNormal, mDownward);
    al::normalize(&downward);

    sead::Vector3f targetSide;
    sead::Vector3f targetFront;
    if (mArg->mProperty->mVelocity.dot(mDownward) > 0.0f) {
        bool isBackward = downward.dot(currentFront) < -0.9f;
        sead::Vector3f across;
        across.setCross(mFloorNormal, downward);
        if (isBackward) {
            targetSide = -downward;
            targetFront = across;
        } else {
            targetSide = across;
            targetFront = downward;
        }
    } else {
        targetSide.setCross(mFloorNormal, currentFront);
        al::normalize(&targetSide);
        targetFront.setCross(targetSide, mFloorNormal);
    }

    sead::Matrix34f targetMtx;
    makeMtxSideUpFront(&targetMtx, targetSide, mFloorNormal, targetFront);

    sead::Quatf currentQuat;
    sead::Quatf targetQuat;
    currentMtx.toQuat(currentQuat);
    targetMtx.toQuat(targetQuat);
    sead::Quatf quat;
    quat.slerpTo(currentQuat, targetQuat, mArg->mConstParam->getSlidePostureBlendRate());

    sead::Matrix34f rotateMtx;
    rotateMtx.fromQuat(quat);
    sead::Vector3f front;
    rotateMtx.getBase(front, 2);
    al::verticalizeVec(&front, mArg->mProperty->mUpDir, front);
    al::normalize(&front);
    mArg->mProperty->mFront.e = front.e;

    const sead::Vector3f& up = mArg->mProperty->mUpDir;
    sead::Vector3f poseSide;
    poseSide.setCross(up, front);
    sead::Matrix34f poseMtx;
    makeMtxSideUpFront(&poseMtx, poseSide, up, front);
    poseMtx.setBase(3, sead::Vector3f::zero);
    poseMtx.invert();
    mArg->mProperty->mTurnMtx.setMul(poseMtx, rotateMtx);
}

/** @brief Leans the player sideways towards the stick direction. */
void PlayerActionSlide::calcTilt() {
    sead::Vector3f moveVec = mArg->mInput->getMoveVec();
    f32 tilt;
    if (al::normalizeOrZero(&moveVec)) {
        tilt = 0.0f;
    } else {
        tilt = al::calcAngleOnPlaneDegree(mArg->mProperty->mFront, moveVec,
                                          mArg->mProperty->mUpDir) /
               90.0f;
        if (tilt < -1.0f) {
            tilt = -1.0f - (tilt + 1.0f);
        } else if (tilt > 1.0f) {
            tilt = 1.0f - (tilt - 1.0f);
        }

        tilt = tilt * (mArg->mConstParam->getSlideTiltMaxDegree() * cPi) / 180.0f;
    }

    PlayerProperty* pProperty = mArg->mProperty;
    pProperty->mTilt = pProperty->mTilt * (1.0f - mArg->mConstParam->getSlideTiltBlendRate()) +
                       tilt * mArg->mConstParam->getSlideTiltBlendRate();
}

/** @brief Turns the velocity onto the slope, starts the animation and squats down. */
void PlayerActionSlide::setup() {
    IUsePlayerCollision::Info info;
    mArg->mCollision->getFloorInfo(&info);
    PlayerActionFunc::calcDownward(&mDownward, mArg->mProperty, info.mNormal);
    mFloorNormal.e = info.mNormal.e;

    if (isOnSlideFloor(mArg->mCollision)) {
        if (!isGentleSlope(mFloorNormal, mArg)) {
            sead::Quatf rotate;
            rotate.makeVectorRotation(mArg->mProperty->mUpDir, mFloorNormal);
            mArg->mProperty->mVelocity.rotate(rotate);
            al::verticalizeVec(&mArg->mProperty->mVelocity, mFloorNormal,
                               mArg->mProperty->mVelocity);

            if (mArg->mProperty->mVelocity.length() > mArg->mConstParam->getForceSlideMaxSpeed()) {
                PlayerProperty* pProperty = mArg->mProperty;
                f32 maxSpeed = mArg->mConstParam->getForceSlideMaxSpeed();
                f32 length = pProperty->mVelocity.length();
                if (length > 0.0f) {
                    pProperty->mVelocity = (maxSpeed / length) * pProperty->mVelocity;
                }
            }
        } else {
            sead::Vector3f slopeVelocity;
            al::verticalizeVec(&slopeVelocity, mFloorNormal, mArg->mProperty->mVelocity);
            mArg->mProperty->mVelocity -= slopeVelocity;
            slopeVelocity *= mArg->mConstParam->getForceSlideSpeedUpRate();
            mArg->mProperty->mVelocity += slopeVelocity;
        }
    }

    mArg->mAnimator->startAnim("SlopeSlide");
    mArg->mAudio->startSe("PgSlopeSlide");
    mCollisionSize->squat();
}

/** @brief Stands up, faces along the ground and resets the model posture and lean. */
void PlayerActionSlide::teardown() {
    mCollisionSize->standUp();
    mArg->mAudio->stopSe("PgSlopeSlide");

    sead::Vector3f front = mArg->mProperty->mFront;
    al::verticalizeVec(&front, mArg->mProperty->mUpDir, front);
    al::normalize(&front);
    mArg->mProperty->setFrontVec(front);
    mArg->mProperty->setUpVec(mArg->mProperty->mUpDir);
    mArg->mProperty->mTurnMtx = sead::Matrix34f::ident;
    mArg->mProperty->mTilt = 0.0f;
}

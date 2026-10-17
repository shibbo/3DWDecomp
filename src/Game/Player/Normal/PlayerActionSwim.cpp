#include "Player/Normal/PlayerActionSwim.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"

namespace {
/// Stick tilt below which the stick doesn't steer the swim direction.
constexpr f32 cStickDeadZone = 0.1f;
/// Model offset while swimming, so the model rotates around the player's center.
constexpr f32 cSwimModelOffsetY = -75.0f;

/**
 * @brief Gets the sign of a value, treating NaN as negative.
 * @param value The value.
 * @return -1 if the value is negative, 1 otherwise.
 */
f32 calcSign(f32 value) {
    return !(value >= 0.0f) ? -1.0f : 1.0f;
}

/**
 * @brief Remaps a stick axis past the dead zone to [-1, 1].
 * @param stick The stick axis, outside of the dead zone.
 * @return The stick axis with the dead zone cut off.
 */
f32 calcStickRate(f32 stick) {
    return (stick + calcSign(stick) * -cStickDeadZone) / (1.0f - cStickDeadZone);
}

/**
 * @brief Scales a vector to a length, leaving a zero vector as is.
 * @param pVec The vector to scale.
 * @param length The new length.
 */
void setVecLength(sead::Vector3f* pVec, f32 length) {
    f32 oldLength = pVec->length();
    if (oldLength > 0.0f) {
        *pVec *= length / oldLength;
    }
}
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 */
PlayerActionSwim::PlayerActionSwim(const PlayerActionArg* pArg) : mArg(pArg) {}

/** @brief Moves the player through the water without snapping to the ground. */
void PlayerActionSwim::move() {
    mArg->mCollision->solveAir();
}

/** @brief Steers the swim direction with the stick and paddles, kicks or brakes. */
void PlayerActionSwim::update() {
    if (sead::Mathf::abs(mArg->getInput()->getStickX()) > cStickDeadZone) {
        f32 rate = calcStickRate(mArg->getInput()->getStickX());
        PlayerProperty* pProperty = mArg->mProperty;
        sead::Quatf quat;
        quat.setAxisAngle(pProperty->getUpDir(),
                          -(rate * mArg->mConstParam->getSwimHRotSpeed()));
        mArg->mProperty->mFront.rotate(quat);
        al::normalize(&mArg->mProperty->mFront);
        mArg->mProperty->mGroundUp.rotate(quat);
        al::normalize(&mArg->mProperty->mGroundUp);
    }

    if (sead::Mathf::abs(mArg->getInput()->getStickY()) > cStickDeadZone) {
        f32 rate = calcStickRate(mArg->getInput()->getStickY());
        PlayerProperty* pProperty = mArg->mProperty;
        sead::Vector3f side = pProperty->mGroundUp.cross(pProperty->mFront);
        al::normalize(&side);
        sead::Quatf quat;
        quat.setAxisAngle(side, rate * mArg->mConstParam->getSwimVRotSpeed());
        mArg->mProperty->mFront.rotate(quat);
        al::normalize(&mArg->mProperty->mFront);
        mArg->mProperty->mGroundUp.rotate(quat);
        al::normalize(&mArg->mProperty->mGroundUp);

        pProperty = mArg->mProperty;
        if (pProperty->mFront.y >= 0.0f) {
            pProperty->mFront.y = 0.0f;
            al::normalize(&mArg->mProperty->mFront);
            pProperty = mArg->mProperty;
            f32 dot = pProperty->mGroundUp.dot(pProperty->getUpDir());
            pProperty->mGroundUp.x = 0.0f;
            pProperty->mGroundUp.z = 0.0f;
            pProperty->mGroundUp.y = calcSign(dot);
        }
    }

    if (mArg->getInput()->isSwimPaddleTrigOn()) {
        mPaddleFrame = mArg->mConstParam->getSwimPaddleFrame();
        // the game queries the paddle sub animation here and ignores the result
        mArg->mAnimator->isSubAnim("SwimPaddle");
        PlayerProperty* pProperty = mArg->mProperty;
        pProperty->mVelocity += pProperty->mGroundUp * mArg->mConstParam->getSwimPaddleAccel();
        if (mArg->mProperty->mVelocity.length() > mArg->mConstParam->getSwimPaddleSpeedMax()) {
            setVecLength(&mArg->mProperty->mVelocity,
                         mArg->mConstParam->getSwimPaddleSpeedMax());
        }
    }

    if (mArg->mAnimator->isSubAnim("SwimPaddle") && mArg->mAnimator->isSubAnimEnd()) {
        mArg->mAnimator->endSubAnim();
    }

    if (mPaddleFrame != 0) {
        mPaddleFrame--;
        turnVelocityToFront();
    } else if (mArg->getInput()->isSwimPaddleButtonOn()) {
        if (!mArg->mAnimator->isAnim("Swim")) {
            mArg->mAnimator->startAnim("Swim");
        }

        turnVelocityToFront();
        if (mArg->mProperty->mVelocity.length() > mArg->mConstParam->getSwimKickSpeedMax()) {
            PlayerProperty* pProperty = mArg->mProperty;
            pProperty->mVelocity *= mArg->mConstParam->getSwimKickBrake();
            if (mArg->mProperty->mVelocity.length() < mArg->mConstParam->getSwimKickSpeedMax()) {
                setVecLength(&mArg->mProperty->mVelocity,
                             mArg->mConstParam->getSwimKickSpeedMax());
            }
        } else {
            PlayerProperty* pProperty = mArg->mProperty;
            pProperty->mVelocity += pProperty->mGroundUp * mArg->mConstParam->getSwimKickAccel();
            if (mArg->mProperty->mVelocity.length() > mArg->mConstParam->getSwimKickSpeedMax()) {
                setVecLength(&mArg->mProperty->mVelocity,
                             mArg->mConstParam->getSwimKickSpeedMax());
            }
        }
    } else {
        PlayerProperty* pProperty = mArg->mProperty;
        pProperty->mVelocity *= mArg->mConstParam->getSwimBrake();
        if (!mArg->mAnimator->isAnim("SwimWait")) {
            mArg->mAnimator->startAnim("SwimWait");
        }
    }

    PlayerProperty* pProperty = mArg->mProperty;
    f32 frontSpeed = pProperty->mGroundUp.dot(pProperty->mVelocity);
    pProperty->mVelocity -= pProperty->mGroundUp * frontSpeed;
    pProperty = mArg->mProperty;
    pProperty->mVelocity *= mArg->mConstParam->getSwimSideBrake();
    pProperty = mArg->mProperty;
    pProperty->mVelocity += pProperty->mGroundUp * frontSpeed;
}

/** @brief Points the velocity along the swim direction, keeping its speed. */
void PlayerActionSwim::turnVelocityToFront() {
    PlayerProperty* pProperty = mArg->mProperty;
    if (pProperty->mGroundUp.dot(pProperty->mVelocity) != 0.0f) {
        pProperty->mVelocity = pProperty->mGroundUp * pProperty->mVelocity.length();
    }
}

/** @brief Offsets the model so it turns around the player's center while swimming. */
void PlayerActionSwim::setup() {
    mArg->mProperty->mTurnMtx = sead::Matrix34f(1.0f, 0.0f, 0.0f, 0.0f,
                                                 0.0f, 1.0f, 0.0f, cSwimModelOffsetY,
                                                 0.0f, 0.0f, 1.0f, 0.0f);
}

/** @brief Removes the swim model offset. */
void PlayerActionSwim::teardown() {
    mArg->mProperty->mTurnMtx = sead::Matrix34f::ident;
}

#include "Player/Normal/PlayerActionSinkSandMove.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerProperty.hpp"

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pFigureDirector The player's power-up, used to slow the animation down when mini.
 */
PlayerActionSinkSandMove::PlayerActionSinkSandMove(PlayerActionArg* pArg,
                                                   const PlayerFigureDirector* pFigureDirector)
    : mArg(pArg), mFigureDirector(pFigureDirector) {}

/** @brief Moves the player through the air (the quicksand holds them up). */
void PlayerActionSinkSandMove::move() {
    mArg->mCollision->solveAir();
}

/** @brief Accelerates, brakes and turns the player along the ground and updates the animation. */
void PlayerActionSinkSandMove::update() {
    const sead::Vector3f& rMoveVec = mArg->getInput()->getMoveVec();
    sead::Vector3f floorNormal = mArg->mProperty->getUpDir();

    if (mArg->mCollision->isOnFloor()) {
        IUsePlayerCollision::Info info;
        mArg->mCollision->getFloorInfo(&info);
        floorNormal.set(info.mNormal);

        sead::Quatf rotate;
        al::makeQuatRotationRate(&rotate, mFloorNormal, floorNormal, 1.0f);
        mFloorNormal.set(floorNormal);
        mArg->mProperty->mVelocity.rotate(rotate);
        al::verticalizeVec(&mArg->mProperty->mVelocity, floorNormal, mArg->mProperty->mVelocity);
    }

    al::verticalizeVec(&mArg->mProperty->mVelocity, mArg->mProperty->getUpDir(),
                       mArg->mProperty->mVelocity);

    sead::Vector3f front = mArg->mProperty->getFront();
    sead::Vector3f up = mArg->mProperty->getUpDir();
    sead::Vector3f side;
    side.setCross(up, front);
    al::normalize(&side);
    front.setCross(side, up);
    al::normalize(&front);

    sead::Vector3f floorFront = front;
    sead::Vector3f floorUp = floorNormal;
    sead::Vector3f floorSide = side;
    sead::Quatf floorRotate;
    al::makeQuatRotationRate(&floorRotate, up, floorNormal, 1.0f);
    floorFront.rotate(floorRotate);
    floorSide.rotate(floorRotate);

    sead::Vector3f velocity = mArg->mProperty->getVelocity();
    f32 frontSpeed = floorFront.dot(velocity);
    f32 upSpeed = floorUp.dot(velocity);
    f32 sideSpeed = floorSide.dot(velocity);
    u32 brakeFrame = getConstParam()->getBrakeFrame();

    if (frontSpeed > getConstParam()->getSinkSandMoveMaxSpeed() * 0.95f) {
        mFastFrame++;
    } else if (mFastFrame < 10) {
        mFastFrame = 0;
    }

    sideSpeed = PlayerActionFunc::brake(sideSpeed, brakeFrame,
                                        getConstParam()->getSinkSandMoveMaxSpeed());

    if (frontSpeed < -0.1f) {
        frontSpeed = PlayerActionFunc::brake(frontSpeed, brakeFrame,
                                             getConstParam()->getSinkSandMoveMaxSpeed());
    } else if (mArg->getInput()->isStickOn()) {
        sead::Vector3f dir = rMoveVec;
        sead::Vector3f normDir = dir;
        al::normalize(&normDir);

        if (normDir.dot(front) < -0.17365f) {
            frontSpeed = PlayerActionFunc::brake(frontSpeed, brakeFrame,
                                                 getConstParam()->getSinkSandMoveMaxSpeed());
            frontSpeed = PlayerActionFunc::cutOff(frontSpeed, 0.1f);
        } else {
            sead::Vector3f moveDir = dir;
            al::verticalizeVec(&moveDir, up, moveDir);
            al::normalize(&moveDir);

            f32 maxSpeed = mArg->getInput()->isDashButtonOn() ?
                               getConstParam()->getSinkSandMoveMaxDashSpeed() :
                               getConstParam()->getSinkSandMoveMaxSpeed();
            f32 walkRate = getConstParam()->getWalkMinSpeedRate() +
                           (1.0f - getConstParam()->getWalkMinSpeedRate()) * dir.length();
            f32 targetSpeed = maxSpeed * walkRate;

            if (targetSpeed > frontSpeed) {
                f32 accel = getConstParam()->getSinkSandMoveMaxSpeed() /
                            static_cast<u32>(getConstParam()->getAccelFrame());
                frontSpeed = PlayerActionFunc::accel(frontSpeed, maxSpeed, accel);
            }

            if (frontSpeed < 0.0f) {
                frontSpeed = 0.0f;
            } else if (frontSpeed > targetSpeed) {
                frontSpeed = PlayerActionFunc::brake(frontSpeed, 1,
                                                     getConstParam()->getSinkSandMoveMaxSpeed());
                if (frontSpeed < targetSpeed) {
                    frontSpeed = targetSpeed;
                }
            }

            sead::Quatf turn;
            al::makeQuatRotationLimit(
                &turn, front, moveDir,
                sead::Mathf::deg2rad(getConstParam()->getRoundLimitDegreeMax()));
            front.rotate(turn);
            al::normalize(&front);
            mArg->mProperty->mFront.set(front);
        }
    } else {
        frontSpeed =
            PlayerActionFunc::brake(frontSpeed, 1, getConstParam()->getSinkSandMoveMaxSpeed());
    }

    floorFront.setRotated(floorRotate, front);
    floorSide.setCross(floorUp, floorFront);
    al::normalize(&floorSide);

    mArg->mProperty->mVelocity =
        floorFront * frontSpeed + floorUp * upSpeed + floorSide * sideSpeed;

    sead::Vector3f horizontalVel = mArg->mProperty->getVelocity();
    al::verticalizeVec(&horizontalVel, floorUp, horizontalVel);
    f32 speed = horizontalVel.length();

    if (mArg->mSubAction != nullptr) {
        if (speed == 0.0f) {
            mArg->mSubAction->setThrowAnimCancel(false);
        } else {
            mArg->mSubAction->setThrowAnimCancel(true);
        }
    }

    mArg->mAnimator->setWeightSixfold(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    updateAnimRate(speed, false);
}

/**
 * @brief Sets the animation rate from the speed.
 * @param speed The player's horizontal speed.
 * @param isBrake Whether the player brakes (unused).
 */
void PlayerActionSinkSandMove::updateAnimRate(f32 speed, bool isBrake) {
    f32 rate = getConstParam()->getRunAnimRateMax() * speed / 10.0f;
    if (mFigureDirector->getFigure() == EPlayerFigure::Mini) {
        rate *= getConstParam()->getShortAnimRateEff();
    }

    mArg->mAnimator->setAnimRate(rate);
}

/** @brief Starts the move animation and validates the sub actions. */
void PlayerActionSinkSandMove::setup() {
    if (mArg->mCollision->isOnFloor()) {
        IUsePlayerCollision::Info info;
        mArg->mCollision->getFloorInfo(&info);
        mFloorNormal.set(info.mNormal);
    } else {
        mFloorNormal = mArg->mProperty->getUpDir();
    }

    sead::Vector3f horizontalVel;
    al::verticalizeVec(&horizontalVel, mArg->mProperty->getGroundUp(),
                       mArg->mProperty->getVelocity());
    f32 speed = horizontalVel.length();

    mArg->mAnimator->startAnim("Move");
    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->validateAll();
    }

    mFastFrame = speed > getConstParam()->getSinkSandMoveMaxSpeed() * 0.95f ? 10 : 0;
}

/** @brief Invalidates the sub actions. */
void PlayerActionSinkSandMove::teardown() {
    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->invalidateAll();
        mArg->mSubAction->setThrowAnimCancel(false);
    }
}

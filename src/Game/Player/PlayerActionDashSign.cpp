#include "Player/PlayerActionDashSign.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pProperty The player's physical state.
 */
PlayerActionDashSign::PlayerActionDashSign(PlayerActionArg* pArg, PlayerProperty* pProperty)
    : mArg(pArg), mProperty(pProperty) {}

/** @brief Keeps the player on the ground. */
void PlayerActionDashSign::move() {
    mArg->mCollision->snapGround();
}

/** @brief Counts the animation loops and accelerates and turns the player along the ground. */
void PlayerActionDashSign::update() {
    mAnimFrame = static_cast<u32>(mAnimRate + static_cast<f32>(mAnimFrame));
    if (mAnimFrame >= static_cast<u32>(getConstParam()->getDashSignAnimFrameMax())) {
        mLoopCount++;
        mAnimFrame = 0;
    }

    if (static_cast<f32>(mLoopCount) >= getConstParam()->getDashSignMaxLoop()) {
        mIsEnd = true;
        mArg->mAnimator->setAnimRate(1.0f);
        return;
    }

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

    if (mArg->getInput()->isStickOn()) {
        sead::Vector3f dir = mArg->getInput()->getMoveVec();
        al::normalize(&dir);
        mArg->mProperty->mFront.set(dir);
    }

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
    floorSide.rotate(floorRotate);

    sead::Vector3f velocity = mArg->mProperty->getVelocity();

    if (!mArg->getInput()->isStickOn()) {
        mIsEnd = true;
        mArg->mAnimator->setAnimRate(1.0f);
        return;
    }

    floorFront.rotate(floorRotate);
    f32 frontSpeed = floorFront.dot(velocity);
    f32 upSpeed = floorUp.dot(velocity);
    f32 sideSpeed = floorSide.dot(velocity);

    sead::Vector3f dir = mArg->getInput()->getMoveVec();
    al::normalize(&dir);
    sead::Vector3f moveDir = dir;
    al::verticalizeVec(&moveDir, up, moveDir);
    al::normalize(&moveDir);

    // The first half of the loops runs at the dash sign speed, the rest at the normal speed.
    f32 maxSpeed = static_cast<f32>(mLoopCount) >= getConstParam()->getDashSignMaxLoop() * 0.5f ?
                       getConstParam()->getNormalMaxSpeed() :
                       getConstParam()->getDashSignMaxSpeed();
    f32 walkRate = getConstParam()->getWalkMinSpeedRate() +
                   (1.0f - getConstParam()->getWalkMinSpeedRate()) *
                       mArg->getInput()->getMoveVec().length();
    f32 targetSpeed = maxSpeed * walkRate;

    if (targetSpeed > frontSpeed) {
        f32 accel = getConstParam()->getNormalMaxSpeed() /
                    static_cast<u32>(getConstParam()->getAccelFrame());
        frontSpeed = PlayerActionFunc::accel(frontSpeed, targetSpeed, accel);
    }

    sideSpeed = PlayerActionFunc::brake(sideSpeed, getConstParam()->getBrakeFrame(),
                                        getConstParam()->getDashSignMaxSpeed());

    floorFront.setRotated(floorRotate, front);
    floorSide.setCross(floorUp, floorFront);
    al::normalize(&floorSide);

    f32 rate = (frontSpeed - getConstParam()->getDashSignMaxSpeed()) /
               (getConstParam()->getNormalMaxSpeed() - getConstParam()->getDashSignMaxSpeed());
    if (rate < 0.0f) {
        rate = 0.0f;
    } else if (rate > 1.0f) {
        rate = 1.0f;
    }

    f32 degree = rate * getConstParam()->getRoundLimitDegreeMax() +
                 (1.0f - rate) * getConstParam()->getRoundLimitDegreeMax();
    sead::Quatf turn;
    al::makeQuatRotationLimit(&turn, front, moveDir, sead::Mathf::deg2rad(degree));
    front.rotate(turn);
    al::normalize(&front);
    mArg->mProperty->mFront.set(front);

    mArg->mProperty->mVelocity =
        floorUp * upSpeed + floorFront * frontSpeed + floorSide * sideSpeed;
}

/** @brief Starts the move animation and validates the sub actions. */
void PlayerActionDashSign::setup() {
    mArg->mAnimator->startAnim("Move");
    mAnimRate = getConstParam()->getDashSignAnimRate();
    mArg->mAnimator->setAnimRate(mAnimRate);
    mArg->mAnimator->setWeightSixfold(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    if (mArg->mCollision->isOnFloor()) {
        IUsePlayerCollision::Info info;
        mArg->mCollision->getFloorInfo(&info);
        mFloorNormal.set(info.mNormal);
    } else {
        mFloorNormal = mArg->mProperty->getUpDir();
    }

    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->validateAll();
    }

    mAnimFrame = 0;
}

/** @brief Resets the loop count, stops the player if the stick is released and invalidates
 * the sub actions. */
void PlayerActionDashSign::teardown() {
    mLoopCount = 0;
    mIsEnd = false;

    if (!mArg->getInput()->isStickOn()) {
        mArg->mProperty->mVelocity.set(0.0f, 0.0f, 0.0f);
    }

    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->invalidateAll();
    }
}

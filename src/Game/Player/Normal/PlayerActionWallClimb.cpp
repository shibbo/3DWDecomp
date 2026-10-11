#include "Player/Normal/PlayerActionWallClimb.hpp"

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerEventReceiver.hpp"
#include "Player/IUsePlayerFlag.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerSnapWallInfo.hpp"
#include "Player/IUsePlayerWallClimbInfo.hpp"
#include "Player/IUsePlayerWallClimbInput.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerTrigger.hpp"
#include "Player/Normal/PlayerWallJumpDirection.hpp"

namespace {
/// Sensor trigger raised when the player lets go of the wall.
constexpr auto cWallClimbEndTrigger = static_cast<PlayerTrigger::ESensorTrigger>(21);

/**
 * @brief Copies a vector as one block (the trivial copy of the underlying x/y/z struct).
 * @param pDst The destination.
 * @param rSrc The source.
 */
inline void copyVec(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    pDst->e = rSrc.e;
}

/**
 * @brief Gets the absolute value of a float.
 * @param value The value.
 * @return The absolute value.
 */
inline f32 absValue(f32 value) {
    return value > 0.0f ? value : -value;
}

/**
 * @brief Clamps a rate to [0, 1].
 * @param pRate The rate.
 */
inline void clampRate(f32* pRate) {
    if (*pRate < 0.0f) {
        *pRate = 0.0f;
    } else if (*pRate > 1.0f) {
        *pRate = 1.0f;
    }
}

/**
 * @brief Shortens a vector to the given length if it is longer (compares squared lengths).
 * @param pVec The vector to limit.
 * @param maxLength The maximum length.
 */
inline void limitLengthSquared(sead::Vector3f* pVec, f32 maxLength) {
    if (pVec->squaredLength() > maxLength * maxLength) {
        f32 length = pVec->length();
        if (length > 0.0f) {
            *pVec *= maxLength / length;
        }
    }
}

/**
 * @brief Shortens a vector to the given length if it is longer.
 * @param pVec The vector to limit.
 * @param maxLength The maximum length.
 */
inline void limitLength(sead::Vector3f* pVec, f32 maxLength) {
    if (pVec->length() > maxLength) {
        f32 length = pVec->length();
        if (length > 0.0f) {
            *pVec *= maxLength / length;
        }
    }
}
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pSnapWallInfo The wall the player snapped to.
 * @param pWallJumpDirection The wall jump direction, updated while climbing.
 * @param pWallClimbInfo How long the player may climb.
 * @param pWallClimbInput The stick input on the wall.
 * @param pTrigger The player's one-frame events.
 * @param pFlingPoleFlag Flag cleared when the climb starts (may be null).
 */
PlayerActionWallClimb::PlayerActionWallClimb(const PlayerActionArg* pArg,
                                             const IUsePlayerSnapWallInfo* pSnapWallInfo,
                                             PlayerWallJumpDirection* pWallJumpDirection,
                                             const IUsePlayerWallClimbInfo* pWallClimbInfo,
                                             const IUsePlayerWallClimbInput* pWallClimbInput,
                                             PlayerTrigger* pTrigger,
                                             const IUsePlayerFlag* pFlingPoleFlag)
    : mArg(pArg), mSnapWallInfo(pSnapWallInfo), mWallJumpDirection(pWallJumpDirection),
      mWallClimbInfo(pWallClimbInfo), mWallClimbInput(pWallClimbInput), mTrigger(pTrigger),
      mFlingPoleFlag(pFlingPoleFlag) {}

/** @brief Keeps the player snapped to the wall. */
void PlayerActionWallClimb::move() {
    mArg->mCollision->snapWall(false);
}

/** @brief Follows the wall, then climbs, brakes and slides along it following the stick. */
void PlayerActionWallClimb::update() {
    mWallJumpDirection->update();

    sead::Vector3f wallNormal = mWallNormal * 0.8f + mSnapWallInfo->getSnapWallNormal() * 0.2f;
    if (!al::normalizeOrZero(&wallNormal)) {
        copyVec(&mWallNormal, wallNormal);
    }

    PlayerActionFunc::forceFaceTo(mArg->mProperty, -mWallNormal);
    sead::Vector3f* pVelocity = &mArg->mProperty->mVelocity;
    al::verticalizeVec(pVelocity, mSnapWallInfo->getSnapWallNormal(), *pVelocity);

    const sead::Vector3f& rMoveVec = mWallClimbInput->getMoveVec();
    PlayerProperty* pProperty = mArg->mProperty;
    f32 climbInput = rMoveVec.dot(pProperty->getUpDir());
    sead::Vector3f climbDir =
        pProperty->getGroundUp() -
        mSnapWallInfo->getSnapWallNormal() *
            mArg->mProperty->getGroundUp().dot(mSnapWallInfo->getSnapWallNormal());
    if (al::normalizeOrZero(&climbDir)) {
        copyVec(&climbDir, mArg->mProperty->getGroundUp());
    }

    if (climbInput > 0.0f) {
        climbUp(climbInput, climbDir);
    } else if (climbInput < 0.0f) {
        climbDown(-climbInput, climbDir);
    } else {
        brake(climbDir);
    }

    controlSlide(climbDir);
    controlAnim(climbDir);
}

/**
 * @brief Accelerates up the wall.
 * @param speedRate How far the stick is tilted up, scaling the maximum speed.
 * @param rClimbDir The up direction along the wall.
 */
void PlayerActionWallClimb::climbUp(f32 speedRate, const sead::Vector3f& rClimbDir) {
    sead::Vector3f climbVel;
    al::parallelizeVec(&climbVel, rClimbDir, mArg->mProperty->mVelocity);
    mArg->mProperty->mVelocity -= climbVel;
    climbVel += mArg->mConstParam->getWallClimbAccel() * rClimbDir;
    limitLengthSquared(&climbVel, getMaxClimbSpeed() * speedRate);
    mArg->mProperty->mVelocity += climbVel;
    mState = EState::Climb;
    incrementClimbCount();
}

/**
 * @brief Accelerates down the wall.
 * @param speedRate How far the stick is tilted down, scaling the maximum speed.
 * @param rClimbDir The up direction along the wall.
 */
void PlayerActionWallClimb::climbDown(f32 speedRate, const sead::Vector3f& rClimbDir) {
    sead::Vector3f climbVel;
    al::parallelizeVec(&climbVel, rClimbDir, mArg->mProperty->mVelocity);
    mArg->mProperty->mVelocity -= climbVel;
    climbVel += -rClimbDir * mArg->mConstParam->getWallClimbAccel();
    limitLengthSquared(&climbVel, getMaxClimbSpeed() * speedRate);
    mArg->mProperty->mVelocity += climbVel;
    mState = EState::Climb;
    incrementClimbCount();
}

/**
 * @brief Slows down the vertical movement along the wall.
 * @param rClimbDir The up direction along the wall.
 */
void PlayerActionWallClimb::brake(const sead::Vector3f& rClimbDir) {
    sead::Vector3f climbVel;
    al::parallelizeVec(&climbVel, rClimbDir, mArg->mProperty->mVelocity);
    mArg->mProperty->mVelocity -= climbVel;
    climbVel *= mArg->mConstParam->getWallClimbBrakeRate();
    mArg->mProperty->mVelocity += climbVel;
    mState = EState::Keep;
    mBrakeCount++;
}

/**
 * @brief Moves sideways along the wall following the stick.
 * @param rClimbDir The up direction along the wall.
 */
void PlayerActionWallClimb::controlSlide(const sead::Vector3f& rClimbDir) {
    sead::Vector3f slideVel;
    calcSlideVec(&slideVel, mArg->mProperty->mVelocity, rClimbDir);
    mArg->mProperty->mVelocity -= slideVel;

    sead::Vector3f slideInput;
    calcSlideVec(&slideInput, mWallClimbInput->getMoveVec(), rClimbDir);
    f32 inputLength = slideInput.length();
    if (inputLength > 0.1f) {
        f32 rate = (inputLength - 0.1f) / 0.9f;
        clampRate(&rate);
        al::normalize(&slideInput);
        slideInput *= mArg->mConstParam->getWallClimbSideAccel();
        slideVel += slideInput;
        limitLength(&slideVel, rate * getMaxSideSpeed());
    } else {
        slideVel = {0.0f, 0.0f, 0.0f};
    }

    mArg->mProperty->mVelocity += slideVel;

    sead::Vector3f side;
    side.setCross(rClimbDir, mArg->mProperty->getFront());
    al::normalizeOrZero(&side);

    const sead::Vector3f& rVelocity = mArg->mProperty->getVelocity();
    if (absValue(rVelocity.dot(rClimbDir)) < 2.0f) {
        f32 sideSpeed = rVelocity.dot(side);
        if (sideSpeed < -0.5f) {
            mState = EState::RightMove;
        } else if (sideSpeed > 0.5f) {
            mState = EState::LeftMove;
        }
    }
}

/**
 * @brief Starts the animation of the new state and scales the climb animation by the speed.
 * @param rClimbDir The up direction along the wall.
 */
void PlayerActionWallClimb::controlAnim(const sead::Vector3f& rClimbDir) {
    if (mState != EState::None) {
        if (mState != mLastState) {
            switch (mState) {
            case EState::Climb:
                mArg->mAnimator->startAnim("ClimbWallRun");
                break;
            case EState::Keep:
                mArg->mAnimator->startAnim("ClimbWallKeep");
                break;
            case EState::LeftMove:
                mArg->mAnimator->startAnim("ClimbWallLeftMove");
                break;
            case EState::RightMove:
                mArg->mAnimator->startAnim("ClimbWallRightMove");
                break;
            default:
                break;
            }
        }

        mLastState = mState;
    }

    mState = EState::None;
    if (mLastState == EState::Climb) {
        f32 speed = absValue(rClimbDir.dot(mArg->mProperty->getVelocity()));
        f32 rate = (speed - mArg->mConstParam->getWallClimbMaxSpeed()) /
                   (mArg->mConstParam->getWallClimbDashMaxSpeed() -
                    mArg->mConstParam->getWallClimbMaxSpeed());
        clampRate(&rate);
        mArg->mAnimator->setAnimRate(
            rate * mArg->mConstParam->getWallClimbDashAnimRate() +
            (1.0f - rate) * mArg->mConstParam->getWallClimbNormalAnimRate());
    }
}

/** @brief Faces the player along the wall and resets the climb state. */
void PlayerActionWallClimb::setup() {
    mArg->mProperty->setFrontVec(-mSnapWallInfo->getSnapWallNormal());
    copyVec(&mWallNormal, mSnapWallInfo->getSnapWallNormal());
    mIsEnd = false;
    _49 = true;
    mClimbCount = 0;
    mBrakeCount = 0;
    copyVec(&mStartFront, mArg->mProperty->getFront());
    mStartStick = {mArg->getInput()->getStickX(), mArg->getInput()->getStickY()};
    mLastState = EState::None;
    mState = EState::None;

    if (mFlingPoleFlag != nullptr && mFlingPoleFlag->isOn()) {
        mArg->mEventReceiver->requestFlingPoleFlagClear();
    }
}

/** @brief Stands the player back upright and raises the wall climb end trigger. */
void PlayerActionWallClimb::teardown() {
    mArg->mProperty->mGroundUp = {0.0f, 1.0f, 0.0f};
    al::verticalizeVec(&mArg->mProperty->mFront, mArg->mProperty->mGroundUp,
                       mArg->mProperty->mFront);
    if (al::normalizeOrZero(&mArg->mProperty->mFront)) {
        mArg->mProperty->mFront = {0.0f, 0.0f, 1.0f};
    }

    mTrigger->set(cWallClimbEndTrigger);
}

/**
 * @brief Checks whether the climb is over.
 * @return Whether the climb time ran out or the player stayed still for too long.
 */
bool PlayerActionWallClimb::isEnd() const {
    if (mIsEnd) {
        return true;
    }

    return mBrakeCount >= static_cast<u32>(mArg->mConstParam->getWallClimbStopFrame());
}

/**
 * @brief Gets the maximum climb speed (faster while dashing).
 * @return The speed.
 */
f32 PlayerActionWallClimb::getMaxClimbSpeed() const {
    if (mArg->getInput()->isDashButtonOn()) {
        return mArg->mConstParam->getWallClimbDashMaxSpeed();
    }

    return mArg->mConstParam->getWallClimbMaxSpeed();
}

/** @brief Counts a climbing frame and ends the climb once the climb time ran out. */
void PlayerActionWallClimb::incrementClimbCount() {
    mClimbCount++;
    if (mClimbCount >= static_cast<u32>(getClimbFrame())) {
        mIsEnd = true;
    }
}

/**
 * @brief Gets how many frames the player may climb (longer while dashing).
 * @return The frame count.
 */
s32 PlayerActionWallClimb::getClimbFrame() const {
    if (mArg->getInput()->isDashButtonOn()) {
        return mWallClimbInfo->getWallClimbDashFrame();
    }

    return mWallClimbInfo->getWallClimbFrame();
}

/**
 * @brief Gets the sideways part of a vector along the wall.
 * @param pOut The sideways part.
 * @param rVec The vector.
 * @param rClimbDir The up direction along the wall.
 */
void PlayerActionWallClimb::calcSlideVec(sead::Vector3f* pOut, const sead::Vector3f& rVec,
                                         const sead::Vector3f& rClimbDir) const {
    al::verticalizeVec(pOut, mSnapWallInfo->getSnapWallNormal(), rVec);
    al::verticalizeVec(pOut, rClimbDir, *pOut);
}

/**
 * @brief Gets the maximum sideways speed (faster while dashing).
 * @return The speed.
 */
f32 PlayerActionWallClimb::getMaxSideSpeed() const {
    if (mArg->getInput()->isDashButtonOn()) {
        return mArg->mConstParam->getWallClimbDashMaxSideSpeed();
    }

    return mArg->mConstParam->getWallClimbMaxSideSpeed();
}

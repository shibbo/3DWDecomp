#include "Player/PlayerActionAirMove.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <attributes.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCheckArea.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerEffect.hpp"
#include "Player/IUsePlayerHolded.hpp"
#include "Player/IUsePlayerHorizontalSpeedAverage.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerLongFallCheck.hpp"
#include "Player/IUsePlayerRaccoonDogFallTask.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerJumpExtension.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerTrigger.hpp"

namespace {
/// Sensor trigger set when the player gets released from being held.
constexpr PlayerTrigger::ESensorTrigger cTriggerReleaseHold =
    static_cast<PlayerTrigger::ESensorTrigger>(7);
/// Sensor trigger that starts the next ground move with a dash start.
constexpr PlayerTrigger::ESensorTrigger cTriggerDashStart =
    static_cast<PlayerTrigger::ESensorTrigger>(22);
/// Collision trigger set when the player lands in water.
constexpr PlayerTrigger::ECollisionTrigger cTriggerLandInWater =
    static_cast<PlayerTrigger::ECollisionTrigger>(5);

/// cos(85 degrees): inputs closer than this to the side don't accelerate forward.
constexpr f32 cFrontInputCos = 0.0871557f;
/// cos(135 degrees): inputs further back than this turn the player around.
constexpr f32 cTurnCos = -0.70710677f;
/// cos(90 degrees) (computed in double precision).
constexpr f32 cSideCos = 6.123234e-17f;
/// Fall speed damping applied each frame the fall speed is capped.
constexpr f32 cFallSpeedMaxBrake = 0.99f;
/// Fall speed cap above which a cancelled jump is braked (old player demo).
constexpr f32 cOverrideJumpCancelMinSpeed = 20.0f;

/**
 * @brief Calculates how far the stick is tilted, ignoring a small dead zone.
 * @param rMoveVec The stick direction.
 * @return 0 at the dead zone edge, 1 at full tilt (not clamped).
 */
inline f32 calcInputRate(const sead::Vector3f& rMoveVec) {
    return (rMoveVec.length() - 0.1f) / 0.9f;
}

/**
 * @brief Scales a vector to the given length (a zero vector stays zero).
 * @param pVec The vector to scale.
 * @param length The new length.
 */
inline void setLength(sead::Vector3f* pVec, f32 length) {
    f32 curLength = pVec->length();
    if (curLength > 0.0f) {
        *pVec *= length / curLength;
    }
}

/**
 * @brief Shortens a vector to the given length if it is longer.
 * @param pVec The vector to limit.
 * @param maxLength The maximum length.
 */
inline void limitLength(sead::Vector3f* pVec, f32 maxLength) {
    if (pVec->length() > maxLength) {
        setLength(pVec, maxLength);
    }
}
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pTrigger The player's one-frame events (may be null).
 * @param pCheckArea The area queries (may be null).
 * @param pHolded The state while held by another player (may be null).
 * @param isIgnoreHoldRelease Whether to ignore the release-from-hold trigger.
 */
PlayerActionAirMove::PlayerActionAirMove(PlayerActionAirMoveArg* pArg, PlayerTrigger* pTrigger,
                                         const IUsePlayerCheckArea* pCheckArea,
                                         IUsePlayerHolded* pHolded, bool isIgnoreHoldRelease)
    : mArg(pArg->mActionArg), mLongFallCheck(pArg->mLongFallCheck),
      mRaccoonDogFallTask(pArg->mRaccoonDogFallTask), mIsFlightDuration(pArg->mIsFlightDuration),
      mFlightDurationRotBlendRate(pArg->mFlightDurationRotBlendRate), mTrigger(pTrigger),
      mHolded(pHolded), mIsIgnoreHoldRelease(isIgnoreHoldRelease), mCheckArea(pCheckArea) {}

/** @brief Moves the player through the air and solves the collision. */
void PlayerActionAirMove::move() {
    mArg->mCollision->solveAir();
}

/** @brief Updates the horizontal and vertical velocity and the facing direction. */
void PlayerActionAirMove::update() {
    if (mArg->mCollision->isOnFrontWall() && !mIsOnFrontWall) {
        mFrontWallHitCount++;
    }

    mIsOnFrontWall = mArg->mCollision->isOnFrontWall();

    if (mArg->mCollision->isOnCeiling()) {
        mStickOnFrame = getConstParam()->getJumpAccelAddFrame();
    } else if (mFrontWallHitCount == 1 && mArg->mCollision->isOnFrontWall() &&
               mArg->mProperty->getVelocity().dot(mArg->mProperty->getGravity()) > 0.0f) {
        mStickOnFrame = 0;
        mSpeedRate = 0.0f;
    } else if (mArg->getInput()->isStickOn()) {
        if (mStickOnFrame < static_cast<u32>(getConstParam()->getJumpAccelAddFrame())) {
            mStickOnFrame++;
        }
    } else {
        mStickOnFrame /= 2;
    }

    checkEndTurn();

    PlayerProperty* pProperty = mArg->mProperty;
    const sead::Vector3f& rUp = pProperty->getUpDir();
    sead::Vector3f horizontalVel;
    al::verticalizeVec(&horizontalVel, rUp, pProperty->getVelocity());
    f32 verticalSpeed = rUp.dot(mArg->mProperty->getVelocity());
    mArg->mProperty->mVelocity.set(horizontalVel);

    sead::Vector3f moveVec;
    calcMoveVec(&moveVec);
    f32 inputRate = calcInputRate(moveVec);

    bool isControlled = false;
    if (mInhibitControlFrame != 0) {
        mInhibitControlFrame--;
    } else if (!mIsInhibitControl) {
        if (inputRate > 1.0f) {
            inputRate = 1.0f;
        } else if (inputRate < 0.0f) {
            inputRate = 0.0f;
        }

        if (inputRate > 0.0f) {
            sead::Vector3f moveDir = moveVec;
            al::normalize(&moveDir);
            if (mIsFlightDuration && mIsFlightDurationActive) {
                controlDirectionForFlightDuration();
            } else {
                controlDirection();
            }

            controlHorizontalVelocity(moveDir, inputRate, horizontalVel);
            isControlled = true;
        }
    }

    if (!isControlled && mIsBrakeStickOff) {
        sead::Vector3f side;
        side.setCross(mArg->mProperty->getGroundUp(), mArg->mProperty->getFront());
        al::normalize(&side);

        PlayerProperty* pCurProperty = mArg->mProperty;
        const sead::Vector3f& rVelocity = pCurProperty->getVelocity();
        sead::Vector3f frontVel;
        al::parallelizeVec(&frontVel, pCurProperty->getFront(), rVelocity);
        sead::Vector3f sideVel;
        al::parallelizeVec(&sideVel, side, rVelocity);
        sead::Vector3f restVel = rVelocity - frontVel - sideVel;
        frontVel *= getStickOffBrakeRate();
        sideVel *= getStickOffBrakeRate();
        mArg->mProperty->mVelocity = restVel + (sideVel + frontVel);
    }

    controlVerticalVelocity(&verticalSpeed, rUp);

    IUsePlayerSubAction* pSubAction = mArg->mSubAction;
    if (pSubAction != nullptr) {
        const PlayerProperty* pNewProperty = mArg->mProperty;
        if (pNewProperty->getUpDir().dot(pNewProperty->getVelocity()) >= 0.0f) {
            pSubAction->setMainAnimAfterTailAttack(nullptr);
        } else {
            pSubAction->setMainAnimAfterTailAttack(getFallAnimAfterTailAttack());
        }
    }
}

/** @brief Ends the mid-air turn rotation if one is running. */
void PlayerActionAirMove::checkEndTurn() {
    if (mIsTurning) {
        mIsTurning = false;
        mArg->mProperty->mTurnMtx = sead::Matrix34f::ident;
    }
}

/**
 * @brief Accelerates the player horizontally towards the stick input.
 * @param rMoveDir The normalized stick direction.
 * @param inputRate How far the stick is tilted (0 to 1).
 * @param rHorizontalVelocity The velocity without its vertical part.
 */
void PlayerActionAirMove::controlHorizontalVelocity(const sead::Vector3f& rMoveDir, f32 inputRate,
                                                    const sead::Vector3f& rHorizontalVelocity) {
    mSpeedRate = sead::Mathf::clamp(
        mSpeedRate + (1.0f - mSpeedRate) * mStickOnFrame /
                         static_cast<u32>(getConstParam()->getJumpAccelAddFrame()),
        0.0f, 1.0f);

    f32 frontDot = rMoveDir.dot(mFront);
    f32 frontRate;
    if (frontDot > 0.0f) {
        frontRate = frontDot > cFrontInputCos ?
                        (frontDot - cFrontInputCos) / (1.0f - cFrontInputCos) :
                        0.0f;
    } else {
        frontRate = frontDot < -cFrontInputCos ?
                        (frontDot - cFrontInputCos) / (1.0f - cFrontInputCos) :
                        0.0f;
    }

    f32 frontAccel;
    f32 maxSpeed;
    f32 sideRate;
    if (getConstParam()->isOverride()) {
        frontAccel = frontRate * getConstParam()->getNormalMaxSpeed() * mSpeedRate * inputRate /
                     static_cast<u32>(getConstParam()->getJumpAccelFrame());
        maxSpeed = getConstParam()->getNormalMaxSpeed();
        sideRate = getConstParam()->getJumpSideVelRate();
    } else {
        u32 accelFrame = mIsReleasedFromHold ? getConstParam()->getReleaseAccelFrame() :
                                               getConstParam()->getJumpAccelFrame();
        frontAccel = frontRate * getNormalMaxSpeed() * mSpeedRate * inputRate / accelFrame;
        maxSpeed = getNormalMaxSpeed();
        sideRate = getSideVelocityRate();
    }

    f32 sideSpeed = maxSpeed * sideRate * mSide.dot(rMoveDir);
    u32 sideAccelFrame = getConstParam()->getJumpAccelFrame();
    f32 sideAccel = mIsInhibitSideAccel ? 0.0f : sideSpeed * inputRate / sideAccelFrame;

    sead::Vector3f velocity =
        mFront * frontAccel + mArg->mProperty->getVelocity() + mSide * sideAccel;
    sead::Vector3f frontVel;
    al::parallelizeVec(&frontVel, mFront, velocity);
    sead::Vector3f sideVel;
    al::parallelizeVec(&sideVel, mSide, velocity);
    sead::Vector3f restVel = velocity - frontVel - sideVel;

    f32 horizontalFrontSpeed = sead::Mathf::abs(mFront.dot(rHorizontalVelocity));
    f32 normalMaxSpeed = getConstParam()->isOverride() ? getConstParam()->getNormalMaxSpeed() :
                                                         getNormalMaxSpeed();
    f32 frontMaxSpeed = normalMaxSpeed * calcInputRate(rMoveDir);
    if (frontMaxSpeed < horizontalFrontSpeed) {
        frontMaxSpeed = horizontalFrontSpeed;
    }

    limitLength(&frontVel, frontMaxSpeed);

    if (frontRate == 0.0f) {
        frontVel *= 1.0f - getConstParam()->getJumpFrontBrakeRate() * inputRate;
    }

    if (mIsInhibitBackward && frontVel.dot(mFront) < 0.0f) {
        frontVel.set(0.0f, 0.0f, 0.0f);
    }

    if (mIsFlightDuration && mIsFlightDurationActive) {
        sideVel *= getConstParam()->getFlightDurationSideDamper();
    } else {
        sideVel *= mSideBrakeRate;
    }

    if (getConstParam()->isOverride()) {
        if (sideVel.length() > getConstParam()->getNormalMaxSpeed()) {
            setLength(&sideVel, getConstParam()->getNormalMaxSpeed());
        }
    } else if (sideVel.length() > getNormalMaxSpeed()) {
        setLength(&sideVel, getNormalMaxSpeed());
    }

    mArg->mProperty->mVelocity = restVel + (frontVel + sideVel);
}

/**
 * @brief Applies gravity, the jump extension and the flight duration to the vertical speed and
 * adds it back to the velocity.
 * @param pSpeed The vertical speed (along the up direction).
 * @param rUp The player's up direction.
 */
void PlayerActionAirMove::controlVerticalVelocity(f32* pSpeed, const sead::Vector3f& rUp) {
    f32 gravityRate = 1.0f;
    if (mJumpExtension != nullptr) {
        mJumpExtension->checkInput(mArg->getInput()->isJumpButtonOn() &&
                                   !mArg->mCollision->isOnCeiling());
        if (mJumpExtension->isExtend()) {
            gravityRate = getConstParam()->getJumpExtensionGravityRate();
        }

        if (mJumpExtension->isCanceled()) {
            f32 speed = *pSpeed;
            if (getConstParam()->isOverride()) {
                if (speed > cOverrideJumpCancelMinSpeed) {
                    *pSpeed *= getConstParam()->getJumpCancelBrakeRate();
                }
            } else if (speed > getJumpCancelMinSpeed()) {
                *pSpeed *= getJumpCancelBrakeRate();
            }
        }
    }

    *pSpeed -= gravityRate * getGravity();
    f32 speed = *pSpeed;
    if (speed < -getFallSpeedMax()) {
        *pSpeed = -getFallSpeedMax();
        if (mIsBrakeAtFallSpeedMax) {
            mArg->mProperty->mVelocity *= cFallSpeedMaxBrake;
        }
    }

    if (mIsFlightDuration && mInhibitControlFrame == 0 && !mIsInhibitControl) {
        mIsFlightDurationActive = false;
        if (mFlightDurationCount != 0 && *pSpeed < 0.0f) {
            if (!mRaccoonDogFallTask->isFirstFalling() && mArg->getInput()->isJumpButtonOn()) {
                mIsFlightDurationUsed = true;
                mFlightDurationCount--;
                *pSpeed = 0.0f;
                mIsFlightDurationActive = true;
                if (!mArg->mAnimator->isAnim(mJumpKeepAnim)) {
                    mArg->mAnimator->startAnim(mJumpKeepAnim);
                }
            } else if (mIsFlightDurationUsed) {
                mFlightDurationCount = 0;
            }
        }

        if (mIsFlightDurationUsed && mFlightDurationCount == 0) {
            if (mArg->mAnimator->isAnim(mJumpKeepAnim)) {
                mArg->mAnimator->startAnim(mFallAfterJumpKeepAnim);
            }

            if (!mArg->mCollision->isOnFloor()) {
                mRaccoonDogFallTask->setup();
            }
        }
    }

    mArg->mProperty->mVelocity += rUp * *pSpeed;
}

/** @brief Sets the move direction to the horizontal velocity (or the front if not moving). */
inline void PlayerActionAirMove::calcFrontFromVelocity() {
    mFront.set(mArg->mProperty->getVelocity());
    al::normalizeOrZero(&mFront);
    al::verticalizeVec(&mFront, mArg->mProperty->getGroundUp(), mFront);
    if (al::normalizeOrZero(&mFront)) {
        mFront = mArg->mProperty->getFront();
    }
}

/** @brief Starts the action: sets the move direction and the speed from the current velocity. */
void PlayerActionAirMove::setup() {
    if (getConstParam()->isOverride()) {
        calcFrontFromVelocity();
    } else {
        if (mTrigger != nullptr && !mIsIgnoreHoldRelease) {
            mIsReleasedFromHold = mTrigger->isOn(cTriggerReleaseHold);
        }

        if (mIsReleasedFromHold) {
            if (mHolded != nullptr) {
                mHolded->queryHoldedHostMoveDir(mFront);
                mHolded->requestClearHoldedHost();
            } else {
                mFront.set(mArg->mProperty->getVelocity());
            }

            if (al::normalizeOrZero(&mFront)) {
                mFront = mArg->mProperty->getFront();
            }
        } else {
            calcFrontFromVelocity();
        }
    }

    f32 speed = sead::Mathf::abs(mFront.dot(mArg->mProperty->getVelocity()));
    f32 maxSpeed = getConstParam()->isOverride() ? getConstParam()->getNormalMaxSpeed() :
                                                   getNormalMaxSpeed();
    mSpeedRate = sead::Mathf::clamp(speed / (maxSpeed * 0.9f), 0.0f, 1.0f);

    mSide.setCross(mArg->mProperty->getGroundUp(), mFront);
    al::normalize(&mSide);

    mArg->mHorizontalSpeedAverage->resetHorizontalSpeedAverage();
    mStickOnFrame = 0;
    mIsTurning = false;
    mLongFallCheck->resetLongFall();
    mFrontWallHitCount = 0;
    mIsOnFrontWall = false;

    IUsePlayerSubAction* pSubAction = mArg->mSubAction;
    if (pSubAction != nullptr) {
        pSubAction->validateAll();
        mArg->mSubAction->setMainAnimAfterThrow("Fall");
    }

    if (mIsFlightDuration) {
        mFlightDurationCount = getConstParam()->getFlightDurationCount();
        mIsFlightDurationUsed = false;
        mIsFlightDurationActive = false;
    } else {
        mRaccoonDogFallTask->setup();
    }
}

/** @brief Ends the action: resets the turn and the sub actions and sets the landing triggers. */
void PlayerActionAirMove::teardown() {
    mLongFallCheck->resetLongFall();
    mArg->mProperty->mTurnMtx = sead::Matrix34f::ident;

    if (mArg->mAnimator->isAnim("TurnAirL") || mArg->mAnimator->isAnim("TurnAirR")) {
        mArg->mAnimator->startAnim("TurnAirEnd");
    }

    IUsePlayerSubAction* pSubAction = mArg->mSubAction;
    if (pSubAction != nullptr) {
        pSubAction->invalidateAll();
        mArg->mSubAction->setMainAnimAfterThrow(nullptr);
        mArg->mSubAction->setMainAnimAfterTailAttack(nullptr);
    }

    if (mIsFlightDuration && mIsFlightDurationUsed && !mArg->mCollision->isOnFloor()) {
        mRaccoonDogFallTask->setup();
    }

    if (getConstParam()->isOverride() || mTrigger == nullptr) {
        return;
    }

    if (mArg->getInput()->isStickOn()) {
        mTrigger->set(cTriggerDashStart);
    }

    if (mCheckArea != nullptr && (mCheckArea->isInWater(mArg->mProperty->getTrans()) ||
                                  mCheckArea->isInWaterNoSink(mArg->mProperty->getTrans()))) {
        mTrigger->set(cTriggerLandInWater);
    }
}

/**
 * @brief Creates the jump extension (lower gravity while the button is held).
 * @param maxFrame How long the jump can be extended.
 */
void PlayerActionAirMove::createJumpExtension(u32 maxFrame) {
    mJumpExtension = new PlayerJumpExtension(maxFrame);
}

/** @brief Restarts the jump extension. */
void PlayerActionAirMove::resetJumpExtension() {
    mJumpExtension->reset();
}

/**
 * @brief Replaces the animation played while the flight duration keeps the player up.
 * @param pAnimName The animation name.
 */
void PlayerActionAirMove::replaceJumpKeep(const char* pAnimName) {
    mJumpKeepAnim = pAnimName;
}

/**
 * @brief Replaces the animation played once the flight duration is used up.
 * @param pAnimName The animation name.
 */
void PlayerActionAirMove::replaceFallAfterJumpKeep(const char* pAnimName) {
    mFallAfterJumpKeepAnim = pAnimName;
}

/** @brief Turns the player towards the stick input, starting a mid-air turn when reversing. */
void PlayerActionAirMove::controlDirection() {
    if (mIsFixDirection) {
        return;
    }

    const sead::Vector3f& rUp = mArg->mProperty->getUpDir();
    sead::Vector3f moveDir;
    calcMoveVec(&moveDir);
    al::normalize(&moveDir);

    sead::Vector3f prevFront = mArg->mProperty->getFront();
    PlayerActionFunc::vertAndNormVec(&prevFront, rUp, moveDir);
    mArg->mProperty->mFront = moveDir;

    if (!mIsTurning && !isSubActionRunning() &&
        prevFront.dot(mArg->mProperty->getFront()) < cTurnCos) {
        startTurn(prevFront, rUp);
        return;
    }

    f32 blendRate = getConstParam()->getJumpRotBlendRate();
    sead::Vector3f& rFront = mArg->mProperty->mFront;
    rFront = rFront * blendRate + prevFront * (1.0f - blendRate);
    al::normalize(&mArg->mProperty->mFront);
}

/**
 * @brief Checks whether a sub action (like throwing) is running.
 * @return True if one is running.
 */
bool PlayerActionAirMove::isSubActionRunning() const {
    IUsePlayerSubAction* pSubAction = mArg->mSubAction;
    if (pSubAction != nullptr) {
        return pSubAction->isRunning();
    }

    return false;
}

/**
 * @brief Starts the mid-air turn: rotates the model by 90 degrees towards the turn.
 * @param rFront The front direction before the turn.
 * @param rUp The player's up direction.
 */
void PlayerActionAirMove::startTurn(const sead::Vector3f& rFront, const sead::Vector3f& rUp) {
    mIsTurning = true;

    sead::Vector3f axis;
    axis.setCross(rFront, mArg->mProperty->getFront());
    // the matrix is built in both branches (hoisting it changes the code generation)
    sead::Quatf rotate;
    sead::Matrix34f turnMtx;
    if (al::normalizeOrZero(&axis) || rUp.dot(axis) > 0.0f) {
        rotate.setAxisAngle(rUp, 90.0f);
        turnMtx.fromQuat(rotate);
    } else {
        rotate.setAxisAngle(rUp, -90.0f);
        turnMtx.fromQuat(rotate);
    }

    mArg->mProperty->mTurnMtx = turnMtx;
    mArg->mEffect->emitEffect("TurnAir");
}

/** @brief Turns the player towards the stick input while the flight duration keeps it up. */
void PlayerActionAirMove::controlDirectionForFlightDuration() {
    PlayerProperty* pProperty = mArg->mProperty;
    sead::Vector3f moveDir;
    calcMoveVec(&moveDir);
    al::normalize(&moveDir);

    sead::Vector3f prevFront = mArg->mProperty->getFront();
    mArg->mProperty->mFront.set(moveDir);

    f32 dot = prevFront.dot(mArg->mProperty->getFront());
    if (dot < cTurnCos) {
        mArg->mProperty->mFront = prevFront;
    } else {
        if (dot < cSideCos) {
            sead::Vector3f side;
            side.setCross(pProperty->getUpDir(), prevFront);
            PlayerActionFunc::vertAndNormVec(&side, pProperty->getUpDir(), side);
            if (side.dot(moveDir) < 0.0f) {
                side = -side;
            }

            mArg->mProperty->mFront.set(side);
        }

        sead::Vector3f& rFront = mArg->mProperty->mFront;
        rFront = rFront * mFlightDurationRotBlendRate +
                 prevFront * (1.0f - mFlightDurationRotBlendRate);
        al::normalize(&mArg->mProperty->mFront);
    }

    mFront = mArg->mProperty->getFront();
    mSide.setCross(mArg->mProperty->getGroundUp(), mFront);
    al::normalize(&mSide);
}

/**
 * @brief Gets the gravity applied each frame.
 * @return The gravity.
 */
f32 PlayerActionAirMove::getGravity() const {
    return getConstParam()->getGravity();
}

/**
 * @brief Gets the maximum fall speed.
 * @return The maximum fall speed.
 */
f32 PlayerActionAirMove::getFallSpeedMax() const {
    return getConstParam()->getFallSpeedMax();
}

/**
 * @brief Gets the horizontal speed factor applied each frame while the stick is released.
 * @return The brake rate.
 */
f32 PlayerActionAirMove::getStickOffBrakeRate() const {
    return 1.0f;
}

/**
 * @brief Gets the animation to return to after a tail attack while falling.
 * @return The animation name.
 */
const char* PlayerActionAirMove::getFallAnimAfterTailAttack() const {
    return "Fall";
}

/**
 * @brief Calculates the move input: the stick, or the release direction after being held.
 * @param pOut The move vector.
 */
void PlayerActionAirMove::calcMoveVec(sead::Vector3f* pOut) const {
    if (mIsReleasedFromHold) {
        *pOut = mFront * getNormalMaxSpeed();
    } else {
        pOut->set(mArg->getInput()->getMoveVec());
    }
}

/**
 * @brief Gets the normal horizontal speed limit.
 * @return The speed.
 */
f32 PlayerActionAirMove::getNormalMaxSpeed() const {
    return getConstParam()->getNormalMaxSpeed();
}

/**
 * @brief Gets the vertical speed factor applied after the jump button is released early.
 * @return The brake rate.
 */
f32 PlayerActionAirMove::getJumpCancelBrakeRate() const {
    return getConstParam()->getJumpCancelBrakeRate();
}

/**
 * @brief Gets the vertical speed above which a released jump gets braked.
 * @return The speed.
 */
f32 PlayerActionAirMove::getJumpCancelMinSpeed() const {
    return getConstParam()->getJumpCancelMinSpeed();
}

/**
 * @brief Gets the sideways speed factor.
 * @return The rate.
 */
f32 PlayerActionAirMove::getSideVelocityRate() const {
    return getConstParam()->getJumpSideVelRate();
}

/** @brief Enables the sub actions (if there are any). */
void PlayerActionAirMove::validateSubAction() {
    IUsePlayerSubAction* pSubAction = mArg->mSubAction;
    if (pSubAction != nullptr) {
        pSubAction->validateAll();
    }
}

/** @brief Disables the sub actions (if there are any). */
void PlayerActionAirMove::invalidateSubAction() {
    IUsePlayerSubAction* pSubAction = mArg->mSubAction;
    if (pSubAction != nullptr) {
        pSubAction->invalidateAll();
    }
}

/**
 * @brief Changes how long the jump can be extended.
 * @param maxFrame The new limit.
 */
void PlayerActionAirMove::updateJumpExtensionMaxFrame(u32 maxFrame) {
    mJumpExtension->setMaxFrame(maxFrame);
}

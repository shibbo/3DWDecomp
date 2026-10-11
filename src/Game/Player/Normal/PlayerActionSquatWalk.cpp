#include "Player/Normal/PlayerActionSquatWalk.hpp"

#include <cmath>
#include <math/seadQuat.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerAudio.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerCollisionSize.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"

namespace {
/// The animation of each step of the squat walk (indexed by PlayerActionSquatWalk::EState).
const char* const cAnimNames[] = {"SquatStart", "SquatWait", "JumpBackStart", "SquatWalk",
                                  "SquatWait"};

/// Sub action group allowed while squat walking.
constexpr u32 cSubActionSquat = 2;
/// Frames the squat turn takes.
constexpr u32 cTurnFrame = 4;
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pCeilingCheck Whether there is room to stand up.
 * @param pCollisionSize The player's collision size, made short while squatting.
 */
PlayerActionSquatWalk::PlayerActionSquatWalk(const PlayerActionArg* pArg,
                                             const IUsePlayerCeilingCheck* pCeilingCheck,
                                             IUsePlayerCollisionSize* pCollisionSize)
    : mCollisionSize(pCollisionSize), mCeilingCheck(pCeilingCheck),
      mState(new StateHolder(EState::Wait)), mArg(pArg) {}

/** @brief Snaps the player to the ground or moves them through the air. */
void PlayerActionSquatWalk::move() {
    PlayerActionFunc::snapGroundOrSolveAir(mArg->mCollision, mArg->mProperty);
}

/** @brief Updates the velocity, the step, the animation and the charge sound. */
void PlayerActionSquatWalk::update() {
    if (mArg->mCollision->isOnFloor()) {
        PlayerActionFunc::removeVelocityParallelToUpVec(mArg->mProperty);
    }

    controlState();
    updateAnimation();
    updateEffectAndSound();
    mState->mIsChanged = false;
}

/** @brief Runs the current step and shifts to the next one. */
void PlayerActionSquatWalk::controlState() {
    switch (mState->mState) {
    case EState::Start:
        updateWalk();
        if (isStartEnd()) {
            if (!checkShiftWalk()) {
                checkShiftWait();
            }
        }

        break;
    case EState::Wait:
        updateWalk();
        updateEnergy();
        if (checkShiftTurn()) {
            break;
        }

        if (checkShiftWalk()) {
            break;
        }

        checkShiftJumpReady();
        break;
    case EState::JumpReady:
        updateWalk();
        updateEnergy();
        if (checkShiftTurn()) {
            break;
        }

        checkShiftWalk();
        break;
    case EState::Walk:
        updateWalk();
        updateEnergy();
        if (checkShiftTurn()) {
            break;
        }

        checkShiftWait();
        break;
    case EState::Turn:
        updateTurn();
        updateEnergy();
        if (isTurnEnd()) {
            if (!checkShiftWalk()) {
                checkShiftWait();
            }
        }

        break;
    }
}

/** @brief Starts the animation of the step if the step changed this frame. */
void PlayerActionSquatWalk::updateAnimation() {
    if (mState->mIsChanged) {
        IUsePlayerAnimator* pAnimator = mArg->mAnimator;
        if (!pAnimator->isAnim(getAnimNames()[static_cast<u32>(mState->mState)])) {
            mArg->mAnimator->startAnim(getAnimNames()[static_cast<u32>(mState->mState)]);
        }
    }
}

/** @brief Plays the charge sound while the squat is fully charged. */
void PlayerActionSquatWalk::updateEffectAndSound() {
    if (!mIsEnergyFull && getEnergy() == 1.0f) {
        mIsEnergyFull = true;
    } else if (mIsEnergyFull && getEnergy() < 1.0f) {
        mIsEnergyFull = false;
    }

    if (mIsEnergyFull) {
        mArg->mAudio->holdSe(getSquatChargeSoundName());
    }
}

/** @brief Squats down and starts the first step. */
void PlayerActionSquatWalk::setup() {
    mCollisionSize->squat();
    mEnergy = 0.0f;
    mIsEnergyFull = false;
    mTurnFrame = 0;

    EState state;
    if (setupSquatStartAnim()) {
        state = EState::Start;
    } else if (mArg->getInput()->isStickOn() && mArg->mCollision->isOnFloor()) {
        mArg->mAnimator->startAnim("SquatWalk");
        state = EState::Walk;
    } else {
        mArg->mAnimator->startAnim("SquatWait");
        state = EState::Wait;
    }

    mState->reset(state);
    PlayerActionFunc::applyGravity(mArg->mProperty, nullptr, mArg->mConstParam->getGravity(),
                                   mArg->mConstParam->getFallSpeedMax());
    mArg->mSubAction->validate(cSubActionSquat);
}

/** @brief Stands the player back up. */
void PlayerActionSquatWalk::teardown() {
    mCollisionSize->standUp();
    mArg->mSubAction->invalidateAll();
}

/** @brief Walks towards the stick input (sideways when it points backwards). */
void PlayerActionSquatWalk::updateWalk() {
    sead::Vector3f horizontalVel;
    sead::Vector3f verticalVel;
    divideVelocity(&horizontalVel, &verticalVel);

    if (mArg->getInput()->isStickOn()) {
        sead::Vector3f moveDir = mArg->getInput()->getMoveVec();
        al::normalize(&moveDir);

        PlayerProperty* pProperty = mArg->mProperty;
        if (pProperty->getFront().dot(moveDir) < 0.0f) {
            sead::Vector3f side;
            side.setCross(pProperty->getGroundUp(), pProperty->getFront());
            al::normalize(&side);
            if (side.dot(moveDir) < 0.0f) {
                side = -side;
            }

            moveDir = side;
        }

        moveDir = moveDir * (1.0f - mArg->mConstParam->getSquatWalkFrontVecBlend()) +
                  mArg->mProperty->getFront() * mArg->mConstParam->getSquatWalkFrontVecBlend();
        al::normalize(&moveDir);
        mArg->mProperty->setFrontVec(moveDir);
        horizontalVel = moveDir * getSquatWalkSpeed();
    } else {
        horizontalVel = {0.0f, 0.0f, 0.0f};
    }

    mArg->mProperty->mVelocity = horizontalVel + verticalVel;
    PlayerActionFunc::applyGravity(mArg->mProperty, mArg->mCollision,
                                   mArg->mConstParam->getGravity(),
                                   mArg->mConstParam->getFallSpeedMax());
}

/**
 * @brief Checks whether the squat start animation is over.
 * @return Whether the start animation ended or isn't playing.
 */
bool PlayerActionSquatWalk::isStartEnd() const {
    if (!mArg->mAnimator->isAnim(getAnimNames()[static_cast<u32>(EState::Start)])) {
        return true;
    }

    return mArg->mAnimator->isAnimEnd();
}

/**
 * @brief Shifts to walking if the stick is tilted on the ground.
 * @return Whether the action shifted to walking.
 */
bool PlayerActionSquatWalk::checkShiftWalk() {
    if (mArg->getInput()->isStickOn() && mArg->mCollision->isOnFloor()) {
        mState->change(EState::Walk);
        return true;
    }

    return false;
}

/**
 * @brief Shifts to waiting (or the jump ready when charged) unless walking on the ground.
 * @return Whether the action shifted.
 */
bool PlayerActionSquatWalk::checkShiftWait() {
    if (mArg->getInput()->isStickOn() && mArg->mCollision->isOnFloor()) {
        return false;
    }

    if (mEnergy == 1.0f) {
        mState->change(EState::JumpReady);
    } else {
        mState->change(EState::Wait);
    }

    return true;
}

/** @brief Charges the squat while the squat button is held, resets it otherwise. */
void PlayerActionSquatWalk::updateEnergy() {
    if (mArg->getInput()->isSquatButtonOn()) {
        mEnergy += 1.0f / static_cast<u32>(mArg->mConstParam->getSquatEnergyAccelFrame());
        if (mEnergy > 1.0f) {
            mEnergy = 1.0f;
        }
    } else {
        mEnergy = 0.0f;
    }
}

/**
 * @brief Shifts to turning around if the stick points behind the player.
 * @return Whether the action shifted to turning.
 */
bool PlayerActionSquatWalk::checkShiftTurn() {
    if (!mArg->getInput()->isStickOn()) {
        return false;
    }

    sead::Vector3f moveDir = mArg->getInput()->getMoveVec();
    al::verticalizeVec(&moveDir, mArg->mProperty->getUpDir(), moveDir);
    al::normalizeOrZero(&moveDir);
    sead::Vector3f front = mArg->mProperty->getFront();
    al::verticalizeVec(&front, mArg->mProperty->getUpDir(), front);
    al::normalizeOrZero(&front);
    if (!PlayerActionFunc::isOppositeSide(moveDir, front)) {
        return false;
    }

    mState->change(EState::Turn);
    mTurnFrame = 0;
    mTurnDir = moveDir;

    sead::Vector3f side;
    side.setCross(mArg->mProperty->getUpDir(), front);
    f32 sideDot = side.dot(mTurnDir);
    f32 angle = std::acos(front.dot(mTurnDir));
    if (sideDot >= 0.0f) {
        angle = -angle;
    }

    mTurnAngle = angle;
    return true;
}

/**
 * @brief Shifts to the jump ready step once the squat is fully charged.
 * @return Whether the action shifted.
 */
bool PlayerActionSquatWalk::checkShiftJumpReady() {
    if (mEnergy == 1.0f) {
        mState->change(EState::JumpReady);
        return true;
    }

    return false;
}

/** @brief Stops and rotates the player towards the turn direction. */
void PlayerActionSquatWalk::updateTurn() {
    mTurnFrame++;

    sead::Vector3f horizontalVel;
    sead::Vector3f verticalVel;
    divideVelocity(&horizontalVel, &verticalVel);
    horizontalVel = {0.0f, 0.0f, 0.0f};
    mArg->mProperty->mVelocity = verticalVel + horizontalVel;
    PlayerActionFunc::applyGravity(mArg->mProperty, mArg->mCollision,
                                   mArg->mConstParam->getGravity(),
                                   mArg->mConstParam->getFallSpeedMax());

    f32 rate = static_cast<u32>(cTurnFrame - mTurnFrame) * (1.0f / cTurnFrame);
    if (rate > 1.0f) {
        rate = 1.0f;
    }

    sead::Quatf rotate;
    rotate.setAxisRadian(mArg->mProperty->getUpDir(), mTurnAngle * rate);
    sead::Vector3f front;
    front.setRotated(rotate, mTurnDir);
    mArg->mProperty->mFront = front;
    mArg->mProperty->mGroundUp = mArg->mProperty->getUpDir();
}

/**
 * @brief Checks whether the turn is over.
 * @return Whether the action isn't turning or the turn frames passed.
 */
bool PlayerActionSquatWalk::isTurnEnd() const {
    if (mState->mState != EState::Turn) {
        return true;
    }

    return mTurnFrame >= cTurnFrame;
}

/**
 * @brief Splits the player's velocity along the up direction.
 * @param pHorizontal The velocity perpendicular to the up direction.
 * @param pVertical The velocity along the up direction.
 */
void PlayerActionSquatWalk::divideVelocity(sead::Vector3f* pHorizontal,
                                           sead::Vector3f* pVertical) const {
    al::verticalizeVec(pHorizontal, mArg->mProperty->getUpDir(), mArg->mProperty->getVelocity());
    al::parallelizeVec(pVertical, mArg->mProperty->getUpDir(), mArg->mProperty->getVelocity());
}

/**
 * @brief Gets the walking speed while squatting.
 * @return The speed.
 */
f32 PlayerActionSquatWalk::getSquatWalkSpeed() {
    return mArg->mConstParam->getSquatWalkSpeed();
}

/**
 * @brief Gets the sound held while the squat is fully charged.
 * @return The sound name.
 */
const char* PlayerActionSquatWalk::getSquatChargeSoundName() const {
    return "PgSquatCharged";
}

/**
 * @brief Gets the animation of each step.
 * @return The animation names, indexed by EState.
 */
const char* const* PlayerActionSquatWalk::getAnimNames() const {
    return cAnimNames;
}

/**
 * @brief Starts the squat start animation if the player isn't squatting already.
 * @return Whether the start animation was started.
 */
bool PlayerActionSquatWalk::setupSquatStartAnim() {
    return PlayerActionFunc::setupSquatStartAnim(mArg->mAnimator, "SquatStart");
}

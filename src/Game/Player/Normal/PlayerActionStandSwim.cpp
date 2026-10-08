#include "Player/Normal/PlayerActionStandSwim.hpp"

#include <algorithm>
#include <math/seadVector.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCheckArea.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerEventReceiver.hpp"
#include "Player/IUsePlayerFlagControl.hpp"
#include "Player/IUsePlayerForwardBent.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/IUsePlayerWaterFlowField.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerTrigger.hpp"

namespace {
/// Sensor trigger the player gets while touching the water surface from below.
constexpr auto cSensorTriggerSolveAir = static_cast<PlayerTrigger::ESensorTrigger>(13);
/// Sensor trigger of a trample from underwater.
constexpr auto cSensorTriggerSwimTrample = static_cast<PlayerTrigger::ESensorTrigger>(19);
/// Sensor trigger of a hit that turns the player flag on.
constexpr auto cSensorTriggerFlagOn = static_cast<PlayerTrigger::ESensorTrigger>(4);
/// Sensor trigger set when the stand swim starts with a trample.
constexpr auto cSensorTriggerTrampleStart = static_cast<PlayerTrigger::ESensorTrigger>(20);
/// Collision trigger of a touch that turns the player flag on.
constexpr auto cCollisionTriggerFlagOn = static_cast<PlayerTrigger::ECollisionTrigger>(2);

/// Frames the player flag stays on after the last hit.
constexpr u32 cFlagFrame = 5;
/// Frames SwimStandMove is kept after the stick is released.
constexpr u32 cMoveAnimKeepFrame = 15;
/// Paddles closer together than this (in frames) don't restart the animation.
constexpr u32 cPaddleFrameMin = 6;
/// Forward speed below which the player doesn't walk on the bottom.
constexpr f32 cWalkSpeedMin = 2.0f;
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pTrigger The one-frame events the player got.
 * @param pWaterFlowField The water current pushing the player.
 * @param pForwardBent The player bending forward while walking.
 * @param pFlagControl The flag turned on while the player gets hit from above.
 * @param pCheckArea Area queries at the player's position.
 */
PlayerActionStandSwim::PlayerActionStandSwim(const PlayerActionArg* pArg,
                                             const PlayerTrigger* pTrigger,
                                             const IUsePlayerWaterFlowField* pWaterFlowField,
                                             IUsePlayerForwardBent* pForwardBent,
                                             IUsePlayerFlagControl* pFlagControl,
                                             const IUsePlayerCheckArea* pCheckArea)
    : mArg(pArg), mTrigger(pTrigger), mWaterFlowField(pWaterFlowField), mCheckArea(pCheckArea),
      mForwardBent(pForwardBent), mFlagControl(pFlagControl) {}

/** @brief Moves the player through the water, snapping to the bottom when standing on it. */
void PlayerActionStandSwim::move() {
    if (mTrigger->isOn(cSensorTriggerSolveAir)) {
        mArg->mCollision->solveAir();
    } else {
        PlayerActionFunc::snapGroundOrSolveAir(mArg->mCollision, mArg->mProperty);
    }
}

/** @brief Updates the velocity, the facing direction and the animation. */
void PlayerActionStandSwim::update() {
    mIsInWaterNoSink = mCheckArea->isInWaterNoSink(mArg->mProperty->getTrans());
    updateVelocity();
    PlayerActionFunc::calcSwimFrontVec(mArg->mProperty, mArg->getInput(),
                                       mArg->mConstParam->getStandSwimRotSpeed());
    if (mHighAccelFrame != 0) {
        mHighAccelFrame--;
    }

    if (!isSwimTramplePlaying()) {
        controlAnim();
    }

    mIsOnFloorPrev = mArg->mCollision->isOnFloor();
    calcForwardBent();
    if (mIsInWaterNoSink &&
        mFromDiveFrame <= static_cast<u32>(mArg->mConstParam->getStandSwimFromDiveTimer())) {
        mFromDiveFrame++;
        if (mIsInWaterNoSink &&
            mFromDiveFrame == static_cast<u32>(mArg->mConstParam->getStandSwimFromDiveTimer())) {
            mArg->mAnimator->startAnim("SwimWalk");
        }
    }
}

/** @brief Updates the vertical and horizontal swim velocity and the player flag. */
void PlayerActionStandSwim::updateVelocity() {
    if (mArg->mCollision->isOnFloor()) {
        PlayerProperty* pProperty = mArg->mProperty;
        if (pProperty->getUpDir().dot(pProperty->getVelocity()) < 0.0f) {
            al::verticalizeVec(&pProperty->mVelocity, pProperty->getUpDir(),
                               pProperty->getVelocity());
        }
    }

    sead::Vector3f velocity =
        mArg->mProperty->getVelocity() - mWaterFlowField->getFlowField();
    // TODO: the game uses verticalizeVelocity's return value (the removed speed) here
    f32 verticalSpeed = mArg->mProperty->getUpDir().dot(velocity);
    PlayerActionFunc::verticalizeVelocity(&velocity, mArg->mProperty->getUpDir());

    bool isTrample = false;
    if (mIsInWaterNoSink &&
        mFromDiveFrame >= static_cast<u32>(mArg->mConstParam->getStandSwimFromDiveTimer())) {
        verticalSpeed = mArg->mConstParam->getStandSwimFromDiveRisePower();
    } else if (mTrigger->isOn(PlayerTrigger::cTrample) ||
               mTrigger->isOn(cSensorTriggerSwimTrample)) {
        verticalSpeed = mArg->mConstParam->getStandSwimTramplePower();
        isTrample = true;
    } else {
        verticalSpeed = PlayerActionFunc::calcSwimVerticalVelocity(
            verticalSpeed, mArg->getInput(), mArg->mConstParam->getStandSwimGravity(),
            mArg->mConstParam);
    }

    if (!isTrample && mArg->getInput()->isPrecedingSwimPaddleTrigOn()) {
        mHighAccelFrame = mArg->mConstParam->getStandSwimHighAccelPermitFrame();
    }

    if (mTrigger->isOn(cSensorTriggerFlagOn) || mTrigger->isOn(cCollisionTriggerFlagOn)) {
        mFlagControl->turnOn();
        mFlagFrame = cFlagFrame;
    } else if (mFlagFrame != 0) {
        mFlagFrame--;
        if (mFlagFrame == 0) {
            mFlagControl->turnOff();
        }
    }

    PlayerActionFunc::calcSwimHorizontalVelocity(
        &velocity, mArg->getInput(), mArg->mProperty, mArg->mCollision, mHighAccelFrame != 0,
        mArg->mConstParam, mIsInWaterNoSink);
    PlayerProperty* pProperty = mArg->mProperty;
    pProperty->mVelocity = pProperty->getUpDir() * verticalSpeed + velocity +
                           mWaterFlowField->getFlowField();
}

/**
 * @brief Checks whether the swim trample animation is still playing.
 * @return Whether SwimTrample is playing and not over.
 */
bool PlayerActionStandSwim::isSwimTramplePlaying() {
    return mArg->mAnimator->isAnim("SwimTrample") && !mArg->mAnimator->isAnimEnd();
}

/** @brief Picks the animation: paddling, walking on the bottom, landing, moving or waiting. */
void PlayerActionStandSwim::controlAnim() {
    if (mMoveAnimKeepFrame != 0) {
        mMoveAnimKeepFrame--;
    }

    mPaddleFrame++;
    if (mArg->getInput()->isPrecedingSwimPaddleTrigOn()) {
        u32 paddleFrame = mPaddleFrame;
        if (paddleFrame >= cPaddleFrameMin) {
            f32 elapsed = paddleFrame - mArg->mConstParam->getStandSwimPaddleAnimRateIntervalMin();
            u32 intervalMax = mArg->mConstParam->getStandSwimPaddleAnimRateIntervalMax();
            f32 range = intervalMax - mArg->mConstParam->getStandSwimPaddleAnimRateIntervalMin();
            f32 rate = 1.0f - std::min(elapsed / range, 1.0f);
            mPaddleAnimRate =
                rate * (mArg->mConstParam->getStandSwimPaddleAnimMaxRate() - 1.0f) + 1.0f;
            mPaddleFrame = 0;
            mIsPaddle = true;
        }
    }

    IUsePlayerAnimator* pAnimator = mArg->mAnimator;
    if (pAnimator->isAnim("SwimStand")) {
        mArg->mAnimator->setAnimRate(mPaddleAnimRate);
    }

    if (mArg->mAnimator->isAnim("SwimStand")) {
        f32 frame = mArg->mAnimator->getAnimFrame();
        if (frame < static_cast<u32>(mArg->mConstParam->getStandSwimPaddleAnimInterval())) {
            return;
        }
    }

    if (mIsPaddle) {
        mArg->mAnimator->startAnim("SwimStand");
        mArg->mAnimator->setAnimRate(mPaddleAnimRate);
        mIsPaddle = false;
        if (mArg->mCollision->isOnFloor()) {
            mArg->mEventReceiver->onKickGroundInWater();
        }

        return;
    }

    if (mIsInWaterNoSink &&
        mFromDiveFrame > static_cast<u32>(mArg->mConstParam->getStandSwimFromDiveTimer()) &&
        mArg->mAnimator->isAnim("SwimStand")) {
        mArg->mAnimator->startAnim("SwimWalk");
    }

    if (mStartAnimName != nullptr && mArg->mAnimator->isAnim(mStartAnimName) &&
        !mArg->mAnimator->isAnimEnd()) {
        return;
    }

    if (mArg->mCollision->isOnFloor() && mArg->mAnimator->isAnim("SwimLand") &&
        !mArg->mAnimator->isAnimEnd() && !mArg->getInput()->isStickOn()) {
        return;
    }

    sead::Vector3f velocity =
        mArg->mProperty->getVelocity() - mWaterFlowField->getFlowField();
    f32 frontSpeed = velocity.dot(mArg->mProperty->getFront());
    bool isOnFloor = mArg->mCollision->isOnFloor();
    if (frontSpeed > cWalkSpeedMin && isOnFloor) {
        if (!mArg->mAnimator->isAnim("SwimWalk")) {
            mArg->mAnimator->startAnim("SwimWalk");
        }

        f32 rate = (frontSpeed - cWalkSpeedMin) /
                   (mArg->mConstParam->getStandSwimWalkMaxSpeed() - cWalkSpeedMin);
        if (rate < 0.0f) {
            rate = 0.0f;
        } else if (rate > 1.0f) {
            rate = 1.0f;
        }

        f32 animRate = rate * mArg->mConstParam->getStandSwimWalkAnimMaxRate() +
                       mArg->mConstParam->getStandSwimWalkAnimMinRate() * (1.0f - rate);
        mArg->mAnimator->setAnimRate(animRate);
        return;
    }

    if (mArg->mCollision->isOnFloor() && !mIsOnFloorPrev && !mArg->getInput()->isStickOn()) {
        mArg->mAnimator->startAnim("SwimLand");
        return;
    }

    if (mArg->getInput()->isStickOn() &&
        mFromDiveFrame < static_cast<u32>(mArg->mConstParam->getStandSwimFromDiveTimer())) {
        mMoveAnimKeepFrame = cMoveAnimKeepFrame;
        if (!mArg->mAnimator->isAnim("SwimStandMove")) {
            mArg->mAnimator->startAnim("SwimStandMove");
        }

        return;
    }

    if (mMoveAnimKeepFrame != 0 || mArg->getInput()->isStickOn()) {
        return;
    }

    if (mArg->mAnimator->isAnim("SwimTrample") || mArg->mAnimator->isAnim("SwimStandWait")) {
        return;
    }

    if (mFromDiveFrame < static_cast<u32>(mArg->mConstParam->getStandSwimFromDiveTimer())) {
        mArg->mAnimator->startAnim("SwimStandWait");
    }
}

/** @brief Bends the player forward while walking on the bottom, but not during a sub action. */
void PlayerActionStandSwim::calcForwardBent() {
    if (mArg->mAnimator->isAnim("SwimWalk")) {
        mForwardBent->clearForwardBent();
    }

    if (mArg->mSubAction->isRunning()) {
        mForwardBent->forceClearForwardBend();
    }
}

/** @brief Starts the stand swim animation and limits the vertical speed. */
void PlayerActionStandSwim::setup() {
    if (mTrigger->isOn(cSensorTriggerSwimTrample)) {
        mArg->mAnimator->startAnim("SwimTrample");
    } else if (mStartAnimName == nullptr || !mArg->mAnimator->isAnim(mStartAnimName)) {
        mArg->mAnimator->startAnim("SwimStandWait");
    }

    _34 = 0;
    mArg->mSubAction->validateAll();
    mIsOnFloorPrev = mArg->mCollision->isOnFloor();
    mForwardBent->validateForwardBent();
    mFlagFrame = 0;
    mPaddleFrame = 0;
    mPaddleAnimRate = 1.0f;
    mIsPaddle = false;
    mIsTrampleStart = mTrigger->isOn(cSensorTriggerTrampleStart);
    mFromDiveFrame = 0;

    PlayerProperty* pProperty = mArg->mProperty;
    // TODO: the game uses verticalizeVelocity's return value (the removed speed) here
    f32 verticalSpeed = pProperty->getUpDir().dot(pProperty->getVelocity());
    PlayerActionFunc::verticalizeVelocity(&pProperty->mVelocity, pProperty->getUpDir());
    if (verticalSpeed > mArg->mConstParam->getStandSwimRiseSpeedMax()) {
        verticalSpeed = mArg->mConstParam->getStandSwimRiseSpeedMax();
    } else if (verticalSpeed < -mArg->mConstParam->getStandSwimFallSpeedMax()) {
        verticalSpeed = -mArg->mConstParam->getStandSwimFallSpeedMax();
    }

    if (mArg->mCollision->isOnFloor()) {
        verticalSpeed = PlayerActionFunc::calcSwimFallVelocity(
            verticalSpeed, mArg->mConstParam->getStandSwimGravity(), mArg->mConstParam);
    }

    pProperty = mArg->mProperty;
    pProperty->mVelocity += pProperty->getUpDir() * verticalSpeed;
}

/** @brief Ends the sub actions, the forward bend and the player flag. */
void PlayerActionStandSwim::teardown() {
    mArg->mSubAction->invalidateAll();
    mForwardBent->invalidateForwardBent();
    mFlagControl->turnOff();
}

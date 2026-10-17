#include "Player/Giga/PlayerActionPropellerJump.hpp"

#include <math/seadVector.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerAudio.hpp"
#include "Player/IUsePlayerEventReceiver.hpp"
#include "Player/IUsePlayerFlag.hpp"
#include "Player/IUsePlayerFlagControl.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerPropellerInhibitor.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"

namespace {
/// How much sideways velocity is kept each frame.
constexpr f32 cSideVelocityRate = 0.95f;
}  // namespace

/**
 * @brief Constructs the propeller jump action.
 * @param pArg The player systems.
 * @param pFlagControl The flag that is on while the action runs.
 * @param pPropellerInhibitor Blocks a new flight until the player lands.
 * @param pGlideFlag Whether a restarted flight goes straight into the glide.
 * @param pTrigger The player's triggers.
 * @param pCheckArea The area checker.
 */
PlayerActionPropellerJump::PlayerActionPropellerJump(
    PlayerActionAirMoveArg* pArg, IUsePlayerFlagControl* pFlagControl,
    IUsePlayerPropellerInhibitor* pPropellerInhibitor, const IUsePlayerFlag* pGlideFlag,
    PlayerTrigger* pTrigger, const IUsePlayerCheckArea* pCheckArea)
    : PlayerActionAirMove(pArg, pTrigger, pCheckArea, nullptr, true), mFlagControl(pFlagControl),
      mPropellerInhibitor(pPropellerInhibitor), mGlideFlag(pGlideFlag) {
    mIsTurning = false;
}

/**
 * @brief Runs the current phase of the flight, then moves the player and damps its sideways
 * velocity.
 */
void PlayerActionPropellerJump::update() {
    mSideBrakeRate = getConstParam()->getPropellerSideDamper();

    switch (mState) {
    case EState::Drop: {
        mIsBrakeStickOff = false;
        PlayerProperty* pProperty = mArg->mProperty;
        if (pProperty->mVelocity.dot(pProperty->mGravity) >=
                getConstParam()->getPropellerEngineBrakeVel() &&
            mArg->getInput()->isJumpButtonOn()) {
            pProperty = mArg->mProperty;
            pProperty->mVelocity -=
                pProperty->mGravity * pProperty->mGravity.dot(pProperty->mVelocity);
            mState = EState::Rise;
        }
        break;
    }
    case EState::EngineBrake:
        mIsBrakeStickOff = false;
        if (mArg->getInput()->isJumpButtonOn()) {
            PlayerProperty* pProperty = mArg->mProperty;
            PlayerActionFunc::scaleVecOfDir(&pProperty->mVelocity, pProperty->mGravity,
                                            getConstParam()->getPropellerEngineBrakeRate());
            pProperty = mArg->mProperty;
            if (pProperty->mVelocity.dot(pProperty->mGravity) <
                getConstParam()->getPropellerEngineBrakeEndVel()) {
                mState = EState::Rise;
            }
        } else {
            mState = EState::Fall;
        }

        updateSound();
        break;
    case EState::Rise:
        mIsBrakeStickOff = false;
        if (updateEngine()) {
            mState = EState::Glide;
        }

        updateSound();
        break;
    case EState::Glide:
        if (mSeFrame <= static_cast<u32>(getConstParam()->getSePropellerBeginStep())) {
            mArg->mAudio->stopSe("PropellerFlyStart");
        }

        mIsBrakeStickOff = true;
        if (mArg->getInput()->isJumpButtonOn()) {
            mArg->mAudio->holdSe("PropellerFlyDown");
        }
        break;
    case EState::Fall:
        mIsBrakeStickOff = false;
        break;
    case EState::Hover:
        mIsBrakeStickOff = false;
        if (mArg->getInput()->isJumpButtonOn()) {
            PlayerProperty* pProperty = mArg->mProperty;
            al::verticalizeVec(&pProperty->mVelocity, pProperty->mUpDir, pProperty->mVelocity);
            mState = EState::Glide;
        }
        break;
    }

    mGravity = calcGravity();
    PlayerActionAirMove::update();

    const PlayerProperty* pProperty = mArg->mProperty;
    sead::Vector3f side;
    side.setCross(pProperty->mGroundUp, pProperty->mFront);
    if (!al::normalizeOrZero(&side)) {
        PlayerActionFunc::scaleVecOfDir(&mArg->mProperty->mVelocity, side, cSideVelocityRate);
    }
}

/**
 * @brief Starts the flight sound on the first frame and counts the frames.
 */
void PlayerActionPropellerJump::updateSound() {
    if (mSeFrame == 0) {
        mArg->mAudio->startSe("PropellerFlyStart");
    }

    mSeFrame++;
}

/**
 * @brief Lifts the player while the engine has power left. Releasing the button after the
 * minimum sustain time cuts the power.
 * @return Whether the engine ran out of power.
 */
bool PlayerActionPropellerJump::updateEngine() {
    u32 maxFrame = getPropellerPowMaxFrame();
    if (!mArg->getInput()->isJumpButtonOn()) {
        mIsHoldingButton = false;
    }

    if (mIsHoldingButton ||
        mPowFrame < static_cast<u32>(getConstParam()->getPropellerPowSustainMin())) {
        if (mPowFrame < maxFrame) {
            mPowFrame++;
        }
    } else if (mPowFrame != 0) {
        mPowFrame = maxFrame;
    }

    if (mPowFrame == 0) {
        return false;
    }

    if (mPowFrame >= maxFrame) {
        return true;
    }

    f32 rate;
    if (mPowFrame > static_cast<u32>(getConstParam()->getPropellerPowSustain())) {
        rate = static_cast<f32>(maxFrame - mPowFrame) /
               static_cast<f32>(static_cast<u32>(getConstParam()->getPropellerPowRelease()));
    } else {
        rate = 1.0f;
    }

    PlayerProperty* pProperty = mArg->mProperty;
    pProperty->mVelocity += pProperty->mUpDir * getConstParam()->getPropellerRisePow() * rate;
    return false;
}

/**
 * @brief Calculates the gravity of the current phase.
 * @return The gravity to apply this frame.
 */
f32 PlayerActionPropellerJump::calcGravity() const {
    if (mState == EState::Hover || mState == EState::Drop) {
        return getConstParam()->getPropellerBeforeDropGravity();
    }

    if (mState == EState::EngineBrake) {
        return 0.0f;
    }

    const PlayerProperty* pProperty = mArg->mProperty;
    if (pProperty->mVelocity.dot(pProperty->mUpDir) > 0.0f ||
        mPowFrame < getPropellerPowMaxFrame()) {
        return getConstParam()->getPropellerRiseGravity();
    }

    if (mArg->getInput()->isJumpButtonOn()) {
        return getConstParam()->getPropellerAfterDropGravity();
    }

    return getConstParam()->getGravity();
}

/**
 * @brief Starts the flight: drops first, or glides right away if the engine was used up.
 */
void PlayerActionPropellerJump::setup() {
    PlayerActionAirMove::setup();
    mPowFrame = 0;
    mSeFrame = 0;
    mFlagControl->turnOn();
    if (mPropellerInhibitor->isInhibit()) {
        mPowFrame = getPropellerPowMaxFrame();
        if (mGlideFlag->isOn()) {
            mState = EState::Glide;
        } else {
            mState = EState::Hover;
        }
    } else {
        mState = EState::Drop;
    }

    mIsHoldingButton = true;
    mIsPropellerJumping = true;
    mPropellerInhibitor->inhibitPropeller();
    mArg->mAnimator->startAnim("PropellerJump");
    mArg->mEventReceiver->onCancelJumpAudio();
}

/**
 * @brief Gets how long the engine runs at most.
 * @return The full power frames plus the power release frames.
 */
u32 PlayerActionPropellerJump::getPropellerPowMaxFrame() const {
    return getConstParam()->getPropellerPowSustain() + getConstParam()->getPropellerPowRelease();
}

/**
 * @brief Ends the flight.
 */
void PlayerActionPropellerJump::teardown() {
    mFlagControl->turnOff();
    mIsPropellerJumping = false;
    PlayerActionAirMove::teardown();
}

/**
 * @brief Checks if another action may take over.
 * @return Whether the button is released or the engine ran out of power.
 */
bool PlayerActionPropellerJump::isPossibleToCancel() const {
    if (!mArg->getInput()->isJumpButtonOn()) {
        return true;
    }

    return mPowFrame >= getPropellerPowMaxFrame();
}

/**
 * @brief Checks if the flight is running.
 * @return Whether the flight is running.
 */
bool PlayerActionPropellerJump::isPropellerJumping() const {
    return mIsPropellerJumping;
}

/**
 * @brief Checks if the engine lifts the player.
 * @return Whether the flight is in the rise phase.
 */
bool PlayerActionPropellerJump::isPropellerJumpRising() const {
    return mIsPropellerJumping && mState == EState::Rise;
}

/**
 * @brief Checks if the player glides down with the button held.
 * @return Whether the flight is in the glide phase with the button held.
 */
bool PlayerActionPropellerJump::isPropellerJumpGlide() const {
    return mIsPropellerJumping && mState == EState::Glide && mArg->getInput()->isJumpButtonOn();
}

/**
 * @brief Turns the player towards the stick and updates the front and side directions.
 */
void PlayerActionPropellerJump::controlDirection() {
    PlayerActionFunc::controlDirection(mArg->mProperty, mArg->getInput(),
                                       getConstParam()->getPropellerRotBlendRate());
    mFront = mArg->mProperty->mFront;
    mSide.setCross(mArg->mProperty->mGroundUp, mFront);
    al::normalize(&mSide);
}

/**
 * @brief Gets the gravity of the current phase (calculated in update).
 * @return The gravity.
 */
f32 PlayerActionPropellerJump::getGravity() const {
    return mGravity;
}

/**
 * @brief Gets the fall speed cap, lower while the button is held.
 * @return The fall speed cap.
 */
f32 PlayerActionPropellerJump::getFallSpeedMax() const {
    if (mArg->getInput()->isJumpButtonOn()) {
        return getConstParam()->getPropellerFallSpeedMax();
    }

    return getConstParam()->getPropellerButtonOffFallSpeedMax();
}

/**
 * @brief Gets how hard the player brakes with the stick released.
 * @return The brake rate.
 */
f32 PlayerActionPropellerJump::getStickOffBrakeRate() const {
    return getConstParam()->getPropellerStickOffBrakeRate();
}

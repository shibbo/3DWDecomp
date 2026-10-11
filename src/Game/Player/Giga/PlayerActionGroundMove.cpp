#include "Player/Giga/PlayerActionGroundMove.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/Math/MathUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerAudio.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerDoubleMarioSpeed.hpp"
#include "Player/IUsePlayerEventReceiver.hpp"
#include "Player/IUsePlayerFlag.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerInvincibleDash.hpp"
#include "Player/IUsePlayerMoveSpeedScaler.hpp"
#include "Player/IUsePlayerReaction.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerTrigger.hpp"
#include "Player/Normal/PlayerUprightMtxCalc.hpp"

namespace {
/// Sensor trigger that makes the ground move start out dashing.
constexpr PlayerTrigger::ESensorTrigger cTriggerForceDash =
    static_cast<PlayerTrigger::ESensorTrigger>(21);
/// Sensor trigger that starts the ground move with a dash start.
constexpr PlayerTrigger::ESensorTrigger cTriggerDashStart =
    static_cast<PlayerTrigger::ESensorTrigger>(22);

/**
 * @brief Calculates how far a slope runs downhill between the downhill acceleration angles.
 * @param rFront The front direction along the floor.
 * @param rUp The player's up direction.
 * @param startDegree The angle (from the up direction) where the acceleration starts.
 * @param endDegree The angle where the acceleration is full.
 * @return The rate between 0 and 1.
 */
inline f32 calcDownHillRate(const sead::Vector3f& rFront, const sead::Vector3f& rUp,
                            f32 startDegree, f32 endDegree) {
    f32 dot = rFront.dot(rUp);
    f32 rate = (dot - std::cos(sead::Mathf::deg2rad(startDegree))) /
               (std::cos(sead::Mathf::deg2rad(endDegree)) -
                std::cos(sead::Mathf::deg2rad(startDegree)));
    return sead::Mathf::clamp(rate, 0.0f, 1.0f);
}
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pDoubleMarioSpeed The speed factor while doubled.
 * @param pInvincibleDash The invincible dash state.
 * @param pMoveSpeedScaler The move speed scale.
 * @param pFigureDirector The player's power-up.
 * @param pPanelDashFlag Whether a dash panel boosts the player.
 * @param pModifiedPanelDashFlag Whether a modified dash panel boosts the player.
 * @param pFlingPoleDashFlag Whether a fling pole boosts the player.
 * @param pTrigger The player's one-frame events (may be null).
 * @param pHoldingFlag Whether the player holds something (changes the tilt).
 */
PlayerActionGroundMove::PlayerActionGroundMove(
    PlayerActionArg* pArg, const IUsePlayerDoubleMarioSpeed* pDoubleMarioSpeed,
    const IUsePlayerInvincibleDash* pInvincibleDash,
    const IUsePlayerMoveSpeedScaler* pMoveSpeedScaler,
    const PlayerFigureDirector* pFigureDirector, const IUsePlayerFlag* pPanelDashFlag,
    const IUsePlayerFlag* pModifiedPanelDashFlag, const IUsePlayerFlag* pFlingPoleDashFlag,
    const PlayerTrigger* pTrigger, const IUsePlayerFlag* pHoldingFlag)
    : mArg(pArg), mDoubleMarioSpeed(pDoubleMarioSpeed), mInvincibleDash(pInvincibleDash),
      mMoveSpeedScaler(pMoveSpeedScaler), mPanelDashFlag(pPanelDashFlag),
      mModifiedPanelDashFlag(pModifiedPanelDashFlag), mFlingPoleDashFlag(pFlingPoleDashFlag),
      mHoldingFlag(pHoldingFlag), mTrigger(pTrigger), mFigureDirector(pFigureDirector) {
    mUprightMtxCalc = new PlayerUprightMtxCalc(pArg->mCollision, pArg->mProperty);
}

/** @brief Snaps the player to the ground. */
void PlayerActionGroundMove::move() {
    mArg->mCollision->snapGround();
}

/** @brief Accelerates, brakes and turns the player along the ground and updates the animation. */
void PlayerActionGroundMove::update() {
    const sead::Vector3f& rMoveVec = mArg->getInput()->getMoveVec();
    sead::Vector3f floorNormal = mArg->mProperty->getUpDir();
    bool isSkate =
        mArg->mCollision->isOnFloor() && PlayerActionFunc::isMapCodeSkate(mArg->mCollision);

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

    sead::Vector3f prevFront = mArg->mProperty->getFront();
    sead::Vector3f front = mArg->mProperty->getFront();
    sead::Vector3f up = mArg->mProperty->getUpDir();
    sead::Vector3f side;
    side.setCross(up, front);
    al::normalize(&side);
    front.setCross(side, up);
    al::normalize(&front);

    sead::Quatf floorRotate;
    al::makeQuatRotationRate(&floorRotate, up, floorNormal, 1.0f);
    sead::Vector3f floorFront;
    floorFront.setRotated(floorRotate, front);
    sead::Vector3f floorSide = side;
    floorSide.rotate(floorRotate);

    sead::Vector3f velocity = mArg->mProperty->getVelocity();
    f32 frontSpeed = floorFront.dot(velocity);
    u32 brakeFrame = mArg->getInput()->isDashButtonOn() ? getConstParam()->getDashBrakeFrame() :
                                                          getConstParam()->getBrakeFrame();
    if (isSkate) {
        brakeFrame = getConstParam()->getBrakeFrameOnIce();
    }

    f32 sideSpeed = floorSide.dot(velocity);

    if (frontSpeed > getNormalMaxSpeed() * 0.95f || isSkate) {
        mFastFrame++;
    } else if (mFastFrame < 10) {
        mFastFrame = 0;
    }

    bool isBrake = false;
    if (isSkate && frontSpeed < -0.1f) {
        mArg->mAudio->holdSe("PgSlipOnIce");
    }

    sideSpeed = PlayerActionFunc::brake(sideSpeed, brakeFrame, getNormalMaxSpeed());

    if (frontSpeed < -0.1f) {
        frontSpeed = PlayerActionFunc::brake(frontSpeed, brakeFrame, getNormalMaxSpeed());
        isBrake = true;
    } else if (mArg->getInput()->isStickOn() || isAnyDashFlagOn()) {
        sead::Vector3f dir = rMoveVec;
        if (isAnyDashFlagOn() && al::isNearZero(dir, 0.001f)) {
            dir = front;
        }

        sead::Vector3f normDir = dir;
        al::normalize(&normDir);

        if (normDir.dot(front) < -0.17365f) {
            if (isAnyDashFlagOn()) {
                frontSpeed *= 0.99f;
                if (frontSpeed < 2.0f) {
                    frontSpeed = 2.0f;
                }
            } else {
                frontSpeed = PlayerActionFunc::brake(frontSpeed, brakeFrame, getNormalMaxSpeed());
                frontSpeed = PlayerActionFunc::cutOff(frontSpeed, 0.1f);
                isBrake = true;
                mSuperDashFrame = 0;
            }
        } else {
            sead::Vector3f moveDir = dir;
            al::verticalizeVec(&moveDir, up, moveDir);
            al::normalize(&moveDir);

            f32 maxSpeed = calcMaxSpeed();
            f32 inputRate = dir.length();
            f32 walkRate = getConstParam()->getWalkMinSpeedRate() +
                           inputRate * (1.0f - getConstParam()->getWalkMinSpeedRate());
            f32 accelStartDegree = getConstParam()->getDownHillAccelStartDegree() + 90.0f;
            f32 accelEndDegree = getConstParam()->getDownHillAccelEndDegree() + 90.0f;
            f32 addRate;
            f32 downHillRate;
            if (getConstParam()->isOverride()) {
                addRate = getConstParam()->getDownHillAccelAddRate();
                downHillRate = calcDownHillRate(floorFront, mArg->mProperty->getUpDir(),
                                                accelStartDegree, accelEndDegree);
            } else {
                addRate = getDownHillAccelAddRate();
                downHillRate = calcDownHillRate(floorFront, mArg->mProperty->getUpDir(),
                                                accelStartDegree, accelEndDegree);
            }

            f32 downHillScale = downHillRate * addRate + 1.0f;

            f32 limitSpeed = maxSpeed * downHillScale;
            f32 targetSpeed = maxSpeed * walkRate * downHillScale;
            if (isAnyDashFlagOn()) {
                f32 panelSpeed = getDashPanelSpeed();
                if (frontSpeed < panelSpeed) {
                    frontSpeed = panelSpeed;
                }

                mSuperDashFrame = getSuperDashTimer();
                mDashStartFrame = getDashPanelStartFrame();
                limitSpeed = panelSpeed;
                targetSpeed = panelSpeed;
            }

            f32 doubleMarioRate = mDoubleMarioSpeed->getSpeedRate();
            limitSpeed *= doubleMarioRate;
            targetSpeed *= doubleMarioRate;
            if (mIsSpeedScaleEnabled) {
                limitSpeed *= mMoveSpeedScaler->getSpeedScale();
                targetSpeed *= mMoveSpeedScaler->getSpeedScale();
            } else if (mMoveSpeedScaler->getSpeedScale() == 1.0f) {
                mIsSpeedScaleEnabled = true;
            }

            if (targetSpeed > frontSpeed) {
                f32 accel;
                if (frontSpeed < getNormalMaxSpeed()) {
                    if (isSkate) {
                        accel = 0.1f;
                    } else if (getConstParam()->isOverride()) {
                        accel = getNormalMaxSpeed() /
                                static_cast<u32>(getConstParam()->getAccelFrame());
                    } else if (mArg->mCollision->isOnFrontWall()) {
                        accel = getNormalMaxSpeed();
                    } else {
                        accel = getNormalMaxSpeed() / getAccelFrame();
                    }
                } else {
                    accel = getDashMaxSpeed() /
                            static_cast<u32>(getConstParam()->getDashAccelFrame());
                }

                if (getConstParam()->isOverride()) {
                    frontSpeed = PlayerActionFunc::accel(frontSpeed, targetSpeed, accel);
                } else {
                    frontSpeed = accelerate(frontSpeed, targetSpeed, accel);
                }
            }

            if (getConstParam()->isOverride()) {
                frontSpeed = OldPlayerDemoAdjustSpeed(frontSpeed);
            }

            if (frontSpeed < 0.0f) {
                frontSpeed = 0.0f;
            } else if (frontSpeed > targetSpeed) {
                if (frontSpeed <= getDashMaxSpeed()) {
                    brakeFrame = getConstParam()->getStickOnBrakeFrame();
                }

                frontSpeed = PlayerActionFunc::brake(frontSpeed, brakeFrame, getNormalMaxSpeed());
                if (frontSpeed < targetSpeed) {
                    frontSpeed = targetSpeed;
                }
            }

            if (!isSkate) {
                f32 dashRate = sead::Mathf::clamp((frontSpeed - getNormalMaxSpeed()) /
                                                      (getDashMaxSpeed() - getNormalMaxSpeed()),
                                                  0.0f, 1.0f);
                f32 limitDegreeMin;
                f32 limitDegreeMax;
                if (getConstParam()->isOverride()) {
                    limitDegreeMin = getConstParam()->getRoundLimitDegreeMin();
                    limitDegreeMax = getConstParam()->getRoundLimitDegreeMax();
                } else {
                    limitDegreeMin = getRoundLimitDegreeMin();
                    limitDegreeMax = getRoundLimitDegreeMax();
                }

                f32 limitDegree = dashRate * limitDegreeMin + (1.0f - dashRate) * limitDegreeMax;
                sead::Quatf turn;
                al::makeQuatRotationLimit(&turn, front, moveDir,
                                          sead::Mathf::deg2rad(limitDegree));
                front.rotate(turn);
                al::normalize(&front);
                mArg->mProperty->mFront = front;
            }

            // A dash panel keeps the super dash timer where it set it.
            if (!isAnyDashFlagOn()) {
                if (getConstParam()->isOverride()) {
                    const IUsePlayerInput* input = mArg->getInput();
                    if (input->isDashButtonOn() && input->isStickOn()) {
                        if (mSuperDashFrame < static_cast<u32>(getSuperDashTimer())) {
                            if (frontSpeed >= limitSpeed * 0.95f) {
                                mSuperDashFrame++;
                            } else {
                                mSuperDashFrame = 0;
                            }
                        }
                    } else if (!mIsDashStart) {
                        mSuperDashFrame = 0;
                    } else if (mSuperDashFrame < static_cast<u32>(getSuperDashTimer())) {
                        mSuperDashFrame++;
                    }
                } else if (mArg->mCollision->isOnAnyWall() && !isSuperDashSuccess()) {
                    mSuperDashFrame = 0;
                } else if ((mArg->getInput()->isDashButtonOn() || mIsDashStart) &&
                           mArg->getInput()->isStickOn()) {
                    if (mSuperDashFrame < static_cast<u32>(getSuperDashTimer())) {
                        if (frontSpeed >= limitSpeed * getConstParam()->getMaxSpeedScale() ||
                            (downHillScale > 1.0f &&
                             frontSpeed >= limitSpeed * getConstParam()->getSlopeMaxSpeedScale())) {
                            mSuperDashFrame++;
                        } else {
                            mSuperDashFrame = 0;
                        }
                    }
                } else {
                    mSuperDashFrame = 0;
                }
            }
        }
    } else {
        frontSpeed = PlayerActionFunc::brake(frontSpeed, mFastFrame > 9 ? brakeFrame : 1,
                                             getNormalMaxSpeed());
        isBrake = true;
        mSuperDashFrame = 0;
    }

    f32 upSpeed = floorNormal.dot(velocity);
    if (!isSkate) {
        floorFront.setRotated(floorRotate, front);
        floorSide.setCross(floorNormal, floorFront);
        al::normalize(&floorSide);
    }

    mArg->mProperty->mVelocity =
        floorFront * frontSpeed + floorNormal * upSpeed + floorSide * sideSpeed;

    sead::Vector3f horizontalVel = mArg->mProperty->getVelocity();
    al::verticalizeVec(&horizontalVel, floorNormal, horizontalVel);
    f32 speed = horizontalVel.length();

    if (isSkate) {
        sead::Vector3f moveDir = mArg->getInput()->getMoveVec();
        if (!al::normalizeOrZero(&moveDir)) {
            mArg->mProperty->mFront = moveDir;
        }
    }

    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->setThrowAnimCancel(speed != 0.0f);
    }

    updateMoveState(speed);
    updateBlendWeight(speed, isBrake);
    updateAnimRate(speed, isSkate || isBrake);
    calcTilt(prevFront);

    if (PlayerActionFunc::isClimb(mFigureDirector) &&
        !mArg->mAnimator->isUpperBodyAnimAttached()) {
        PlayerUprightMtxCalc* uprightMtxCalc = mUprightMtxCalc;
        uprightMtxCalc->update(getConstParam()->isOverride() ? 0.8f : getClimbUprightBlendRate());
    } else {
        mUprightMtxCalc->clear();
    }

    if (mSuperDashStartAnimFrame != 0) {
        mSuperDashStartAnimFrame--;
    }
}

/**
 * @brief Tests whether a dash panel, modified dash panel or fling pole boosts the player.
 * @return True if any of the three flags is on.
 */
bool PlayerActionGroundMove::isAnyDashFlagOn() const {
    return isPanelDashOn() || isModifiedPanelDashOn() || isFlingPoleDashOn();
}

/**
 * @brief Calculates the speed the player may reach right now.
 * @return The invincible dash, super dash, dash or normal maximum speed.
 */
f32 PlayerActionGroundMove::calcMaxSpeed() const {
    if (mInvincibleDash->getRate() > 0.0f) {
        f32 rate = mInvincibleDash->getRate();
        return rate * getInvincibleDashSpeed() + getDashMaxSpeed() * (1.0f - rate);
    }

    if (!mArg->getInput()->isDashButtonOn()) {
        return getNormalMaxSpeed();
    }

    if (isSuperDashSuccess()) {
        return getSuperDashSpeed();
    }

    return getDashMaxSpeed();
}

/**
 * @brief Gets the speed the active dash panel or fling pole boosts the player to.
 * @return The boost speed, or 0 if nothing boosts the player.
 */
f32 PlayerActionGroundMove::getDashPanelSpeed() const {
    if (isPanelDashOn()) {
        return getConstParam()->getDashPanelSpeed();
    }

    if (isModifiedPanelDashOn()) {
        return getConstParam()->getModifiedDashPanelSpeed();
    }

    if (isFlingPoleDashOn()) {
        return getConstParam()->getFlingPoleSpeed();
    }

    return 0.0f;
}

/**
 * @brief Gets how many frames the player must dash to reach a super dash.
 * @return The power-up's super dash timer.
 */
s32 PlayerActionGroundMove::getSuperDashTimer() const {
    switch (mFigureDirector->getFigure()) {
    case EPlayerFigure::Climb:
    case EPlayerFigure::Manekineko:
    case EPlayerFigure::ClimbWhite:
    case EPlayerFigure::ClimbGiga:
        return getConstParam()->getSuperDashTimerClimb();
    case EPlayerFigure::Super:
        return getConstParam()->getSuperDashTimer();
    case EPlayerFigure::Mini:
        return getConstParam()->getSuperDashTimerMini();
    case EPlayerFigure::Fire:
        return getConstParam()->getSuperDashTimerFire();
    case EPlayerFigure::RaccoonDog:
        return getConstParam()->getSuperDashTimerRaccoonDog();
    case EPlayerFigure::Boomerang:
        return getConstParam()->getSuperDashTimerBoomerang();
    case EPlayerFigure::RaccoonDogWhite:
        return getConstParam()->getSuperDashTimerRaccoonDogWhite();
    default:
        return getConstParam()->getSuperDashTimer();
    }
}

/**
 * @brief Gets the dash start frames of the active dash panel or fling pole.
 * @return The frames, or 0 if nothing boosts the player.
 */
s32 PlayerActionGroundMove::getDashPanelStartFrame() const {
    if (isPanelDashOn()) {
        return getConstParam()->getDashStartFrame();
    }

    if (isModifiedPanelDashOn()) {
        return getConstParam()->getModifiedDashStartFrame();
    }

    if (isFlingPoleDashOn()) {
        return getConstParam()->getDashStartFrame();
    }

    return 0;
}

/**
 * @brief Tests whether the player dashed long enough for a super dash.
 * @return True if the super dash timer ran out and super dashes are not inhibited.
 */
bool PlayerActionGroundMove::isSuperDashSuccess() const {
    if (mIsSuperDashInhibited) {
        return false;
    }

    return mSuperDashFrame >= static_cast<u32>(getSuperDashTimer());
}

/**
 * @brief Moves between walking, dashing and super dashing.
 * @param speed The player's horizontal speed.
 */
void PlayerActionGroundMove::updateMoveState(f32 speed) {
    if (isSuperDashSuccess()) {
        if (mMoveState != EMoveState::SuperDash) {
            if (mIsDashStart) {
                mIsDashStart = false;
                if (!isDashing()) {
                    mIsDashStart = false;
                    mMoveState = EMoveState::Walk;
                    mSuperDashStartAnimFrame = 0;
                    return;
                }
            }

            mMoveState = EMoveState::SuperDash;
            mArg->mEventReceiver->onSuperDash();
            mSuperDashStartAnimFrame = getConstParam()->getSuperDashStartAnimFrame();
        }
    } else if (isDashSuccess(speed) || mIsForceDash) {
        if (mMoveState != EMoveState::Dash) {
            mMoveState = EMoveState::Dash;
            mSuperDashStartAnimFrame = 0;
        }
    } else if (mMoveState != EMoveState::Walk) {
        mMoveState = EMoveState::Walk;
        mSuperDashStartAnimFrame = 0;
    }
}

/**
 * @brief Blends the walk, run, dash, super dash, invincible dash and dash start animations.
 * @param speed The player's horizontal speed.
 * @param isBrake Whether the player brakes (only the run and dash start animations play).
 */
void PlayerActionGroundMove::updateBlendWeight(f32 speed, bool isBrake) {
    if (isDashSuccess(speed) &&
        mDashStartFrame < static_cast<u32>(getConstParam()->getDashStartFrame())) {
        mDashStartFrame++;
        f32 blendRate =
            static_cast<f32>(mDashStartFrame) /
            static_cast<f32>(static_cast<u32>(getConstParam()->getDashStartBlendFrame()));
        mDashStartBlendRate = sead::Mathf::clampMax(blendRate, 1.0f);
    } else {
        mDashStartBlendRate *= 0.8f;
        mIsDashStart = false;

        if (mInvincibleDash->isPossibleToPlayDashAnim()) {
            mBlendLevel = sead::lerp(mBlendLevel, 4.0f, 0.2f);
        } else if (isSuperDashSuccess()) {
            mBlendLevel = sead::lerp(mBlendLevel, 3.0f, 0.2f);
        } else if (isDashSuccess(speed)) {
            mBlendLevel = sead::lerp(mBlendLevel, 2.0f, 0.2f);
        } else {
            mDashStartFrame = 0;
            f32 level = sead::Mathf::clamp(speed / getNormalMaxSpeed(), 0.0f, 1.0f);
            mBlendLevel = sead::lerp(mBlendLevel, level, 0.2f);
        }
    }

    f32 dashStartRate = mDashStartBlendRate;
    f32 moveRate = 1.0f - dashStartRate;
    if (isBrake) {
        mArg->mAnimator->setWeightSixfold(0.0f, moveRate, 0.0f, 0.0f, 0.0f, dashStartRate);
        return;
    }

    f32 level = mBlendLevel;
    if (level > 3.0f) {
        mArg->mAnimator->setWeightSixfold(0.0f, 0.0f, 0.0f, moveRate * (4.0f - level),
                                          moveRate * (level - 3.0f), dashStartRate);
    } else if (level > 2.0f) {
        mArg->mAnimator->setWeightSixfold(0.0f, 0.0f, moveRate * (3.0f - level),
                                          moveRate * (level - 2.0f), 0.0f, dashStartRate);
    } else if (level > 1.0f) {
        mArg->mAnimator->setWeightSixfold(0.0f, moveRate * (2.0f - level),
                                          moveRate * (level - 1.0f), 0.0f, 0.0f, dashStartRate);
    } else {
        f32 runRate = sead::Mathf::clamp((level - 0.6f) / 0.4f, 0.0f, 1.0f);
        mArg->mAnimator->setWeightSixfold(moveRate * (1.0f - runRate), moveRate * runRate, 0.0f,
                                          0.0f, 0.0f, dashStartRate);
    }
}

/**
 * @brief Sets the animation rate from the blended slots.
 * @param speed The player's horizontal speed.
 * @param isBrake Whether the player brakes or slides (uses a fixed rate).
 */
void PlayerActionGroundMove::updateAnimRate(f32 speed, bool isBrake) {
    f32 rate;
    if (isBrake) {
        rate = 3.0f;
    } else {
        rate = speed;
        if (isPanelDashOn()) {
            rate = getConstParam()->getPanelDashAnimRate();
        } else if (isModifiedPanelDashOn()) {
            rate = getConstParam()->getModifiedPanelDashAnimRate();
        } else if (isFlingPoleDashOn()) {
            rate = getConstParam()->getPanelDashAnimRate();
        } else {
            f32 level = mBlendLevel;
            if (level > 3.0f) {
                rate = calcAnimRate(ESlotIndex::SuperDash) * (4.0f - mBlendLevel) +
                       calcAnimRate(ESlotIndex::InvincibleDash) * (mBlendLevel - 3.0f);
            } else if (level > 2.0f) {
                rate = calcAnimRate(ESlotIndex::Dash) * (3.0f - mBlendLevel) +
                       calcAnimRate(ESlotIndex::SuperDash) * (mBlendLevel - 2.0f);
            } else {
                rate = calcAnimRate(ESlotIndex::Run) * rate;
                if (level > 1.0f) {
                    rate = rate * (2.0f - mBlendLevel) +
                           calcAnimRate(ESlotIndex::Dash) * (mBlendLevel - 1.0f);
                }
            }
        }
    }

    if (mSuperDashStartAnimFrame != 0) {
        rate *= getConstParam()->getSuperDashStartAnimRate();
    }

    f32 dashStartRate = calcAnimRate(ESlotIndex::DashStart);
    if (mFigureDirector->getFigure() == EPlayerFigure::Mini) {
        rate *= getConstParam()->getShortAnimRateEff();
        dashStartRate *= getConstParam()->getShortAnimRateEff();
    } else if (PlayerActionFunc::isClimb(mFigureDirector)) {
        f32 climbRate =
            mBlendLevel > 1.0f ? sead::Mathf::clamp(2.0f - mBlendLevel, 0.0f, 1.0f) : 1.0f;
        f32 climbRunRate = climbRate * (rate * getConstParam()->getClimbRunAnimRateEff());
        rate = (1.0 - climbRate) * rate + climbRunRate;
    }

    f32 moveRate = rate * getAnimRateEff();
    mArg->mAnimator->setAnimRate(dashStartRate * mDashStartBlendRate +
                                 moveRate * (1.0f - mDashStartBlendRate));
}

/** @brief Starts the move animation and carries the speed of the previous action over. */
void PlayerActionGroundMove::setup() {
    if (!getConstParam()->isOverride()) {
        const char* animName = mArg->mAnimator->getAnimName();
        if (al::isEqualSubString(animName, "Jump")) {
            mArg->mReaction->notifyReaction("JumpLand");
        }

        if (al::isEqualSubString(animName, "Air")) {
            mArg->mReaction->notifyReaction("JumpLand");
        }

        if (al::isEqualSubString(animName, "Fall")) {
            mArg->mReaction->notifyReaction("JumpLand");
        }
    }

    mIsForceDash = mTrigger != nullptr && mTrigger->isOn(cTriggerForceDash);
    mIsDashStart = mTrigger != nullptr && mTrigger->isOn(cTriggerDashStart);
    if (isDashing()) {
        mIsDashStart = false;
    }

    if (getConstParam()->isOverride()) {
        mIsForceDash = false;
    } else if (mIsForceDash && isDashing()) {
        mArg->mProperty->mVelocity.y *= 0.5f;
    }

    if (mArg->mCollision->isOnFloor()) {
        IUsePlayerCollision::Info info;
        mArg->mCollision->getFloorInfo(&info);
        mFloorNormal.set(info.mNormal);
    } else {
        mFloorNormal = mArg->mProperty->getUpDir();
    }

    mUprightMtxCalc->clear();
    mIsSpeedScaleEnabled = mMoveSpeedScaler->getSpeedScale() == 1.0f;

    sead::Vector3f floorVel;
    al::verticalizeVec(&floorVel, mFloorNormal, mArg->mProperty->getVelocity());
    if (!PlayerActionFunc::isMapCodeSkate(mArg->mCollision) &&
        floorVel.dot(mArg->mProperty->getFront()) < 0.0f) {
        mArg->mProperty->mVelocity -= floorVel;
    }

    sead::Vector3f horizontalVel;
    al::verticalizeVec(&horizontalVel, mArg->mProperty->getGroundUp(),
                       mArg->mProperty->getVelocity());
    f32 speed = horizontalVel.length();

    if (!mArg->mAnimator->isAnim(getMoveAnimName()) || getConstParam()->isOverride()) {
        mArg->mAnimator->startAnim(getMoveAnimName());
    }

    mSuperDashFrame = 0;
    mMoveState = EMoveState::Walk;
    if (mInvincibleDash->isPossibleToPlayDashAnim()) {
        mBlendLevel = 4.0f;
        mDashStartFrame = getConstParam()->getDashStartFrame();
    } else if (mSuperDashKeepInfo != nullptr && mSuperDashKeepInfo->mIsKeep) {
        mArg->mProperty->mVelocity -= horizontalVel;
        f32 superDashSpeed = getSuperDashSpeed();
        f32 length = horizontalVel.length();
        if (length > 0.0f) {
            horizontalVel *= superDashSpeed / length;
        }

        mArg->mProperty->mVelocity += horizontalVel;
        mBlendLevel = 3.0f;
        mSuperDashFrame = getSuperDashTimer();
        mDashStartFrame = getConstParam()->getDashStartFrame();
        mMoveState = EMoveState::SuperDash;
        mArg->mEventReceiver->onSuperDashLand();
    } else if (isDashSuccess(speed) || mIsForceDash) {
        mBlendLevel = 2.0f;
        mDashStartFrame = getConstParam()->getDashStartFrame();
        mMoveState = EMoveState::Dash;
    } else {
        mBlendLevel = sead::Mathf::clamp(speed / getNormalMaxSpeed(), 0.0f, 1.0f);
        mDashStartFrame = 0;
    }

    if (isAnyDashFlagOn()) {
        mArg->mProperty->mVelocity -= horizontalVel;
        if (al::isNearZero(horizontalVel, 0.001f)) {
            horizontalVel = mArg->mProperty->getFront() * getDashPanelSpeed();
        } else {
            f32 panelSpeed = getDashPanelSpeed();
            f32 length = horizontalVel.length();
            if (length > 0.0f) {
                horizontalVel *= panelSpeed / length;
            }
        }

        mArg->mProperty->mVelocity += horizontalVel;
        mSuperDashFrame = getSuperDashTimer();
        mDashStartFrame = getDashPanelStartFrame();
    }

    mDashStartBlendRate = 0.0f;
    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->validateAll();
    }

    mFastFrame = speed > getNormalMaxSpeed() * 0.95f ? 10 : 0;
}

/**
 * @brief Tests whether the player dashes fast enough or a dash start was triggered.
 * @param speed The player's horizontal speed.
 * @return True if the player dashes faster than the normal maximum speed or starts dashing.
 */
bool PlayerActionGroundMove::isDashSuccess(f32 speed) const {
    if (mArg->getInput()->isDashButtonOn() && getNormalMaxSpeed() < speed) {
        return true;
    }

    return mIsDashStart;
}

/** @brief Stops the sub actions and keeps a running super dash for the next ground move. */
void PlayerActionGroundMove::teardown() {
    if (mArg->mSubAction != nullptr) {
        mArg->mSubAction->invalidateAll();
        mArg->mSubAction->setThrowAnimCancel(false);
    }

    if (mSuperDashKeepInfo != nullptr) {
        bool isKeep = isSuperDashSuccess();
        SuperDashKeepInfo* info = mSuperDashKeepInfo;
        if (isKeep) {
            sead::Vector3f front = mArg->mProperty->getFront();
            info->mFront = front;
        }

        info->mIsKeep = isKeep;
    }

    mUprightMtxCalc->clear();
    mArg->mProperty->mTilt = 0.0f;
    mIsDashStart = false;
}

/**
 * @brief Tests whether the player holds the dash button.
 * @return True if the dash button is held.
 */
bool PlayerActionGroundMove::isDashing() const {
    return mArg->getInput()->isDashButtonOn();
}

/**
 * @brief Tests whether the player super dashes.
 * @return True if the player dashed long enough for a super dash.
 */
bool PlayerActionGroundMove::isDashingFast() const {
    return isSuperDashSuccess();
}

/**
 * @brief Tests whether the player runs on the ground.
 * @return True if the horizontal speed is between half the normal and the super dash speed.
 */
bool PlayerActionGroundMove::isRunningOnGround() const {
    if (getConstParam()->isOverride()) {
        return false;
    }

    sead::Vector3f velocity = mArg->mProperty->getVelocity();
    al::verticalizeVec(&velocity, mArg->mProperty->getUpDir(), velocity);
    f32 speed = velocity.length();
    return speed >= getNormalMaxSpeed() * 0.5f && speed <= getSuperDashSpeed();
}

/**
 * @brief Tests whether the player moves faster than half the super dash speed.
 * @return True if the horizontal speed is at least half the super dash speed.
 */
bool PlayerActionGroundMove::isGreaterSuperDashMaxSpeed() const {
    sead::Vector3f velocity = mArg->mProperty->getVelocity();
    al::verticalizeVec(&velocity, mArg->mProperty->getUpDir(), velocity);
    return velocity.length() >= getSuperDashSpeed() * 0.5f;
}

/**
 * @brief Gets the maximum walking speed.
 * @return The normal maximum speed.
 */
f32 PlayerActionGroundMove::getNormalMaxSpeed() const {
    return getConstParam()->getNormalMaxSpeed();
}

/**
 * @brief Gets the maximum dashing speed.
 * @return The dash maximum speed.
 */
f32 PlayerActionGroundMove::getDashMaxSpeed() const {
    return getConstParam()->getDashMaxSpeed();
}

/**
 * @brief Gets the super dash speed.
 * @return The super dash speed.
 */
f32 PlayerActionGroundMove::getSuperDashSpeed() const {
    return getConstParam()->getSuperDashSpeed();
}

/**
 * @brief Gets the speed of the invincible dash.
 * @return The invincible dash speed.
 */
f32 PlayerActionGroundMove::getInvincibleDashSpeed() const {
    return getConstParam()->getInvincibleDashSpeed();
}

/**
 * @brief Gets how many frames the player takes to reach the normal maximum speed.
 * @return The acceleration frames.
 */
s32 PlayerActionGroundMove::getAccelFrame() const {
    return getConstParam()->getAccelFrame();
}

/**
 * @brief Accelerates the player towards a speed.
 * @param speed The current speed.
 * @param maxSpeed The speed to accelerate to.
 * @param accel The speed added per frame.
 * @return The new speed.
 */
f32 PlayerActionGroundMove::accelerate(f32 speed, f32 maxSpeed, f32 accel) {
    return PlayerActionFunc::accel(speed, maxSpeed, accel);
}

/**
 * @brief Calculates the animation rate of one blend slot.
 * @param index The slot.
 * @return The slot's animation rate (the run rate is per speed unit).
 */
f32 PlayerActionGroundMove::calcAnimRate(ESlotIndex index) const {
    f32 rate = 0.0f;
    switch (index) {
    case ESlotIndex::SuperDash:
    case ESlotIndex::InvincibleDash:
    case ESlotIndex::DashStart:
        rate = 30.0f / 11.0f;
        break;
    case ESlotIndex::Walk:
        break;
    default:
        return 1.0f;
    case ESlotIndex::Run:
        return getConstParam()->getRunAnimRateMax() / 10.0f;
    case ESlotIndex::Dash:
        return 3.0f;
    }

    return rate;
}

/**
 * @brief Tests whether a dash panel boosts the player.
 * @return True if the dash panel flag is on.
 */
bool PlayerActionGroundMove::isPanelDashOn() const {
    return mPanelDashFlag != nullptr && mPanelDashFlag->isOn();
}

/**
 * @brief Tests whether a modified dash panel boosts the player.
 * @return True if the modified dash panel flag is on.
 */
bool PlayerActionGroundMove::isModifiedPanelDashOn() const {
    return mModifiedPanelDashFlag != nullptr && mModifiedPanelDashFlag->isOn();
}

/**
 * @brief Tests whether a fling pole boosts the player.
 * @return True if the fling pole flag is on.
 */
bool PlayerActionGroundMove::isFlingPoleDashOn() const {
    return mFlingPoleDashFlag != nullptr && mFlingPoleDashFlag->isOn();
}

/**
 * @brief Leans the player into turns while running.
 * @param rPrevFront The front direction before this frame's turn.
 */
void PlayerActionGroundMove::calcTilt(const sead::Vector3f& rPrevFront) {
    f32 maxAngle;
    if (!getConstParam()->isOverride() && mHoldingFlag != nullptr && mHoldingFlag->isOn()) {
        maxAngle = getConstParam()->getHoldingTiltMaxFrontAngle();
    } else {
        maxAngle = getConstParam()->getTiltMaxFrontAngle();
    }

    f32 tilt;
    if (al::isNearZero(maxAngle, 0.001f)) {
        tilt = 0.0f;
    } else {
        f32 angle = al::calcAngleOnPlaneDegree(rPrevFront, mArg->mProperty->getFront(),
                                               mArg->mProperty->getUpDir());
        f32 frontSpeed = mArg->mProperty->getVelocity().dot(mArg->mProperty->getFront());
        f32 speedRate = sead::Mathf::clamp(
            (frontSpeed - getConstParam()->getTiltStartSpeed()) /
                (getConstParam()->getTiltEndSpeed() - getConstParam()->getTiltStartSpeed()),
            0.0f, 1.0f);
        tilt = speedRate * (sead::Mathf::clamp(angle, -maxAngle, maxAngle) / maxAngle);
        tilt = tilt * getConstParam()->getTiltMaxDegree() * sead::Mathf::pi() / 180.0f;
    }

    PlayerProperty* property = mArg->mProperty;
    property->mTilt = property->mTilt * (1.0f - getConstParam()->getTiltBlendRate()) +
                      tilt * getConstParam()->getTiltBlendRate();
}

/**
 * @brief Gets how much faster the player gets running downhill.
 * @return The downhill acceleration add rate.
 */
f32 PlayerActionGroundMove::getDownHillAccelAddRate() {
    return getConstParam()->getDownHillAccelAddRate();
}

/**
 * @brief Gets how fast the up direction follows the floor while climbing.
 * @return The blend rate.
 */
f32 PlayerActionGroundMove::getClimbUprightBlendRate() {
    return 0.8f;
}

/**
 * @brief Gets the turn limit at full dash speed.
 * @return The minimum round limit in degrees.
 */
f32 PlayerActionGroundMove::getRoundLimitDegreeMin() const {
    return getConstParam()->getRoundLimitDegreeMin();
}

/**
 * @brief Gets the turn limit at walking speed.
 * @return The maximum round limit in degrees.
 */
f32 PlayerActionGroundMove::getRoundLimitDegreeMax() const {
    return getConstParam()->getRoundLimitDegreeMax();
}

#include "Player/Normal/PlayerActionStandSwimSurface.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadVector.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCheckArea.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerForwardBent.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerReaction.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/IUsePlayerWaterFlowField.hpp"
#include "Player/IUsePlayerWaterSurfaceInfo.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerTrigger.hpp"

namespace {
/// Collision trigger of a dive that ends at the water surface.
constexpr auto cCollisionTriggerDive = static_cast<PlayerTrigger::ECollisionTrigger>(5);
}  // namespace

/**
 * @brief Creates the action.
 * @param pArg The player systems.
 * @param pFigureDirector The player's power-up.
 * @param pWaterFlowField The water current pushing the player.
 * @param pWaterSurfaceInfo The water surface near the player.
 * @param pForwardBent The player bending forward.
 * @param pCheckArea Area queries at the player's position.
 * @param pCharaQuery Which character the player is.
 * @param pTrigger The one-frame events the player got.
 */
PlayerActionStandSwimSurface::PlayerActionStandSwimSurface(
    const PlayerActionArg* pArg, const PlayerFigureDirector* pFigureDirector,
    const IUsePlayerWaterFlowField* pWaterFlowField,
    const IUsePlayerWaterSurfaceInfo* pWaterSurfaceInfo, IUsePlayerForwardBent* pForwardBent,
    const IUsePlayerCheckArea* pCheckArea, const IUsePlayerCharaQuery* pCharaQuery,
    PlayerTrigger* pTrigger)
    : mArg(pArg), mFigureDirector(pFigureDirector), mWaterFlowField(pWaterFlowField),
      mWaterSurfaceInfo(pWaterSurfaceInfo), mCheckArea(pCheckArea), mCharaQuery(pCharaQuery),
      mForwardBent(pForwardBent), mTrigger(pTrigger) {}

/**
 * @brief Remembers whether a figure is a cat suit (a climbing figure or the lucky cat).
 * @param figure The figure.
 */
inline void PlayerActionStandSwimSurface::updateClimbFigure(EPlayerFigure figure) {
    bool isClimbFigure;
    switch (figure.value()) {
    case EPlayerFigure::Climb:
    case EPlayerFigure::ClimbWhite:
    case EPlayerFigure::ClimbGiga:
        isClimbFigure = true;
        break;
    default:
        isClimbFigure = figure == EPlayerFigure::Manekineko;
        break;
    }

    mIsClimbFigure = isClimbFigure;
}

/** @brief Moves the player, snapping to the ground when standing on it. */
void PlayerActionStandSwimSurface::move() {
    PlayerActionFunc::snapGroundOrSolveAir(mArg->mCollision, mArg->mProperty);
}

/** @brief Keeps the player floating at the water surface, turns and tilts it and animates it. */
void PlayerActionStandSwimSurface::update() {
    if (mFigureDirector->isFigureChangeRequested()) {
        updateClimbFigure(mFigureDirector->getNextFigure());
    }

    if (mCheckArea->isInWaterNoSink(mArg->mProperty->getTrans())) {
        if (mIsClimbFigure) {
            mArg->mSubAction->setIgnoreFloorCondition(false);
        } else {
            mArg->mSubAction->setIgnoreFloorCondition(true);
        }
    }

    sead::Vector3f velocity = mArg->mProperty->getVelocity() - mWaterFlowField->getFlowField();
    // TODO: the game uses verticalizeVelocity's return value (the removed speed) here
    f32 verticalSpeed = mArg->mProperty->getUpDir().dot(velocity);
    PlayerActionFunc::verticalizeVelocity(&velocity, mArg->mProperty->getUpDir());

    f32 rate = (static_cast<f32>(mCounter) -
                static_cast<u32>(mArg->mConstParam->getSwimSurfaceValidDamperFrame())) /
               static_cast<u32>(mArg->mConstParam->getSwimSurfaceDamperLerpFrame());
    if (rate < 0.0f) {
        rate = 0.0f;
    } else if (rate > 1.0f) {
        rate = 1.0f;
    }

    updateCounter();
    if (!mCheckArea->isInWaterNoSink(mArg->mProperty->getTrans())) {
        verticalSpeed = PlayerActionFunc::calcSwimVerticalVelocity(
            verticalSpeed, mArg->getInput(), mArg->mConstParam->getSwimSurfaceGravity(),
            mArg->mConstParam);
    }

    bool isInWaterNoSink = mCheckArea->isInWaterNoSink(mArg->mProperty->getTrans());
    if (mWaterSurfaceInfo->isWaterSurfaceExist()) {
        f32 velDamper = mArg->mConstParam->getSwimSurfaceVelDamper();
        f32 invRate = 1.0f - rate;
        f32 damper = invRate * velDamper;
        if (isInWaterNoSink) {
            f32 dampedSpeed = verticalSpeed * damper;
            f32 height = mWaterSurfaceInfo->getWaterSurfaceHeight() +
                         mArg->mConstParam->getSwimSurfaceVerticalOffset();
            f32 baseHeight = mFigureDirector->getFigure() == EPlayerFigure::Mini ?
                                 mArg->mConstParam->getSwimSurfaceBaseHeightShort() :
                                 mArg->mConstParam->getSwimSurfaceBaseHeight();
            f32 offset = height - baseHeight;
            verticalSpeed =
                dampedSpeed + offset * mArg->mConstParam->getSwimSurfaceSpringForSurfaceSwim();
        } else {
            f32 dampedSpeed = verticalSpeed * (rate + damper);
            f32 height = mWaterSurfaceInfo->getWaterSurfaceHeight();
            f32 baseHeight = mFigureDirector->getFigure() == EPlayerFigure::Mini ?
                                 mArg->mConstParam->getSwimSurfaceBaseHeightShort() :
                                 mArg->mConstParam->getSwimSurfaceBaseHeight();
            verticalSpeed = dampedSpeed + invRate * ((height - baseHeight) *
                                                     mArg->mConstParam->getSwimSurfaceSpring());
        }
    }

    PlayerActionFunc::calcSwimHorizontalVelocity(&velocity, mArg->getInput(), mArg->mProperty,
                                                 mArg->mCollision, false, mArg->mConstParam,
                                                 isInWaterNoSink);
    PlayerProperty* pProperty = mArg->mProperty;
    pProperty->mVelocity = pProperty->getUpDir() * verticalSpeed + velocity +
                           mWaterFlowField->getFlowField();

    f32 rotSpeed = mArg->mConstParam->getStandSwimRotSpeed();
    if (!isInWaterNoSink) {
        PlayerActionFunc::calcSwimFrontVec(mArg->mProperty, mArg->getInput(), rotSpeed);
        updateAnim(false);
        return;
    }

    rotSpeed = mArg->mConstParam->getStandSwimSurfaceRotSpeed();
    if (velocity.length() < mArg->mConstParam->getNoSinkSwimHorizontalHighSpeedMin()) {
        rotSpeed = mArg->mConstParam->getStandSwimSurfaceRotSpeedNoMovement();
    }

    PlayerActionFunc::calcSwimFrontVec(mArg->mProperty, mArg->getInput(), rotSpeed);
    updateAnim(true);

    f32 frontAngleMax = mArg->mConstParam->getSwimSurfaceTiltMaxFrontAngle();
    sead::Vector3f moveVec =
        mArg->getInput()->isStickOn() ? mArg->getInput()->getMoveVec() : sead::Vector3f::zero;
    f32 tilt;
    if (al::isNearZero(frontAngleMax, 0.001f)) {
        tilt = 0.0f;
    } else {
        f32 angle = al::calcAngleOnPlaneDegree(mArg->mProperty->getFront(), moveVec,
                                               mArg->mProperty->getUpDir());
        if (angle < -frontAngleMax) {
            angle = -frontAngleMax;
        } else if (angle > frontAngleMax) {
            angle = frontAngleMax;
        }

        f32 tiltDegree = angle / frontAngleMax *
                         mArg->mConstParam->getSwimSurfaceTiltDuringPivotMaxDegree();
        tilt = tiltDegree * sead::Mathf::pi() / 180.0f;
    }

    pProperty = mArg->mProperty;
    pProperty->mTilt = pProperty->mTilt * (1.0f - mArg->mConstParam->getTiltBlendRate()) +
                       tilt * mArg->mConstParam->getTiltBlendRate();
}

/** @brief Restarts the frame counter on a paddle and advances it otherwise. */
void PlayerActionStandSwimSurface::updateCounter() {
    mCounter = mArg->getInput()->isSwimPaddleTrigOn() ? 0 : mCounter + 1;
}

/**
 * @brief Picks the animation: swimming along the surface, paddling, floating or landing.
 * @param isInWaterNoSink Whether the player is in water it can't sink in.
 */
void PlayerActionStandSwimSurface::updateAnim(bool isInWaterNoSink) {
    if (isInWaterNoSink) {
        const sead::Vector3f& velocity = mArg->mProperty->getVelocity();
        f32 speed = sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        if (speed > mArg->mConstParam->getSwimSurfaceSpeedThreshold()) {
            if (!mArg->mAnimator->isAnim("SwimTop")) {
                mArg->mAnimator->startAnim("SwimTop");
                if (mIsClimbFigure) {
                    mArg->mAnimator->setAnimRate(
                        mArg->mConstParam->getSwimSurfaceClimbAnimationRate());
                }
            }
        } else if (mArg->mAnimator->isAnim("SwimTop")) {
            mArg->mAnimator->startAnim("SwimStandSurface");
        }

        return;
    }

    if (mArg->getInput()->isSwimPaddleTrigOn()) {
        if (!mArg->mAnimator->isAnim("SwimStand") ||
            mArg->mAnimator->getAnimFrame() >=
                static_cast<u32>(mArg->mConstParam->getStandSwimPaddleAnimInterval())) {
            mArg->mAnimator->startAnim("SwimStand");
        }
    }

    if (mArg->mAnimator->isAnimEnd()) {
        mArg->mAnimator->startAnim("SwimStandSurface");
    }

    if (mArg->mCollision->isOnFloor() && !mArg->getInput()->isStickOn() &&
        !mArg->mAnimator->isAnim("SwimLand")) {
        mArg->mAnimator->startAnim("SwimLand");
    }
}

/** @brief Starts the floating animation and limits the horizontal speed. */
void PlayerActionStandSwimSurface::setup() {
    if (mTrigger != nullptr && mTrigger->isOn(cCollisionTriggerDive)) {
        mArg->mReaction->notifyReaction("SwimDiveRumble");
    }

    updateClimbFigure(mFigureDirector->getFigure());

    PlayerProperty* pProperty = mArg->mProperty;
    sead::Vector3f horizontalVelocity;
    al::verticalizeVec(&horizontalVelocity, pProperty->getUpDir(), pProperty->getVelocity());
    if (horizontalVelocity.length() > mArg->mConstParam->getStandSwimHorizontalHighSpeedMax()) {
        mArg->mProperty->mVelocity -= horizontalVelocity;
        f32 speedMax = mArg->mConstParam->getStandSwimHorizontalHighSpeedMax();
        f32 speed = horizontalVelocity.length();
        if (speed > 0.0f) {
            horizontalVelocity *= speedMax / speed;
        }

        mArg->mProperty->mVelocity += horizontalVelocity;
    }

    if (mCheckArea->isInWaterNoSink(mArg->mProperty->getTrans())) {
        mArg->mAnimator->startAnim("SwimTop");
        if (mIsClimbFigure) {
            mArg->mAnimator->setAnimRate(mArg->mConstParam->getSwimSurfaceClimbAnimationRate());
        }

        IUsePlayerSubAction* pSubAction = mArg->mSubAction;
        if (pSubAction != nullptr) {
            if (mIsClimbFigure) {
                pSubAction->setIgnoreFloorCondition(false);
            } else {
                pSubAction->setIgnoreFloorCondition(true);
            }


            mArg->mSubAction->validateAll();
        }
    } else if (!mArg->mAnimator->isAnim("SwimStand") || mArg->mAnimator->isAnimEnd()) {
        pProperty = mArg->mProperty;
        if (pProperty->getVelocity().dot(pProperty->getUpDir()) > 0.0f) {
            mArg->mAnimator->startAnim("SwimStand");
        } else {
            mArg->mAnimator->startAnim("SwimStandSurface");
        }

        mArg->mSubAction->validateAll();
        mForwardBent->validateForwardBent();
    }

    mCounter = 0;
}

/** @brief Ends the forward bend, the sub actions and the tilt. */
void PlayerActionStandSwimSurface::teardown() {
    mForwardBent->invalidateForwardBent();
    mArg->mSubAction->invalidateAll();
    mArg->mProperty->mTilt = 0.0f;
}

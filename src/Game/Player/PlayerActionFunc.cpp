#include "Player/PlayerActionFunc.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/Normal/PlayerCollisionIterator.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {

/**
 * Scales a vector to a length unless it is zero.
 * @param pVec the vector
 * @param length the new length
 */
inline void setVecLength(sead::Vector3f* pVec, f32 length) {
    f32 current = pVec->length();
    if (current > 0.0f) {
        *pVec *= length / current;
    }
}

/**
 * Copies a vector as one block (the trivial copy of the underlying x/y/z struct).
 * @param pDst the destination
 * @param rSrc the source
 */
inline void copyVec(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    static_cast<sead::BaseVec3<f32>&>(*pDst) = rSrc;
}

}  // namespace

namespace PlayerActionFunc {

/**
 * Computes the player's side direction (ground up crossed with front).
 * @param pOut receives the side direction (not normalized)
 * @param pProperty the player's physical state
 */
void calcSideDir(sead::Vector3f* pOut, const PlayerProperty* pProperty) {
    pOut->setCross(pProperty->getGroundUp(), pProperty->getFront());
}

/**
 * Brings a speed towards zero by a fixed step.
 * @param speed the current speed
 * @param frame the number of frames a full stop from maxSpeed takes
 * @param maxSpeed the speed the brake is tuned for
 * @return the braked speed, never crossing zero
 */
f32 brake(f32 speed, u32 frame, f32 maxSpeed) {
    if (speed == 0.0f) {
        return speed;
    }

    f32 step = maxSpeed / frame;
    if (speed < 0.0f) {
        speed += step;
        if (speed > 0.0f) {
            speed = 0.0f;
        }
    } else {
        speed -= step;
        if (speed < 0.0f) {
            speed = 0.0f;
        }
    }

    return speed;
}

/**
 * Accelerates a speed up to a maximum.
 * @param speed the current speed
 * @param maxSpeed the speed limit
 * @param accel the acceleration added
 * @return the new speed
 */
f32 accel(f32 speed, f32 maxSpeed, f32 accel) {
    speed += accel;
    if (speed > maxSpeed) {
        speed = maxSpeed;
    }

    return speed;
}

/**
 * Pulls the velocity down, resetting downward speed while on the floor.
 * @param pProperty the player's physical state
 * @param pCollision the player's collision (may be null)
 * @param gravity the downward acceleration
 * @param fallSpeedMax the maximum falling speed
 */
void applyGravity(PlayerProperty* pProperty, const IUsePlayerCollision* pCollision, f32 gravity,
                  f32 fallSpeedMax) {
    if (pCollision != nullptr && pCollision->isOnFloor() && pProperty->mVelocity.y < 0.0f) {
        pProperty->mVelocity.y = 0.0f;
    }

    pProperty->mVelocity += sead::Vector3f(0.0f, -gravity, 0.0f);
    if (pProperty->mVelocity.y < -fallSpeedMax) {
        pProperty->mVelocity.y = -fallSpeedMax;
    }
}

/**
 * Removes the part of the velocity along the ground up direction.
 * @param pProperty the player's physical state
 */
void removeVelocityParallelToUpVec(PlayerProperty* pProperty) {
    al::verticalizeVec(&pProperty->mVelocity, pProperty->getGroundUp(), pProperty->mVelocity);
}

/**
 * Turns the player to face a direction immediately, rebuilding the up direction.
 * @param pProperty the player's physical state
 * @param rDir the direction to face
 */
void forceFaceTo(PlayerProperty* pProperty, const sead::Vector3f& rDir) {
    sead::Vector3f front = rDir;
    al::normalize(&front);
    pProperty->setFrontVec(front);

    if (front.dot(pProperty->getUpDir()) > 0.999f) {
        sead::Quatf rotate;
        al::makeQuatRotationRate(&rotate, pProperty->getFront(), front, 1.0f);
        pProperty->mGroundUp.rotate(rotate);
        al::normalize(&pProperty->mGroundUp);
    } else {
        pProperty->setUpVec(pProperty->getUpDir());
    }

    sead::Vector3f side;
    calcSideDir(&side, pProperty);
    side.normalize();
    sead::Vector3f up;
    up.setCross(pProperty->getFront(), side);
    al::normalize(&up);
    pProperty->setUpVec(up);
}

/**
 * @param pProperty the player's physical state
 * @return whether the velocity points upwards
 */
bool isUpperVelocity(const PlayerProperty* pProperty) {
    return pProperty->getGroundUp().dot(pProperty->getVelocity()) > 0.0f;
}

/**
 * Makes a vector perpendicular to an up direction and normalizes it.
 * @param pVec the vector to change
 * @param rUp the up direction
 * @param rDefault the result used when nothing is left after removing the up part
 */
void vertAndNormVec(sead::Vector3f* pVec, const sead::Vector3f& rUp,
                    const sead::Vector3f& rDefault) {
    al::verticalizeVec(pVec, rUp, *pVec);
    if (al::normalizeOrZero(pVec)) {
        copyVec(pVec, rDefault);
    }
}

/**
 * @param rA a direction
 * @param rB another direction
 * @return whether the two directions are more than 100 degrees apart
 */
bool isOppositeSide(const sead::Vector3f& rA, const sead::Vector3f& rB) {
    return rB.dot(rA) <= -0.17365f;
}

/**
 * @param pInput the player's input
 * @param pProperty the player's physical state
 * @param rDir the direction to compare with
 * @return whether the stick is pushed firmly away from a direction
 */
bool isOppositeInput(const IUsePlayerInput* pInput, const PlayerProperty* pProperty,
                     const sead::Vector3f& rDir) {
    if (pInput->getMoveVec().length() < 0.8f) {
        return false;
    }

    sead::Vector3f moveDir;
    al::verticalizeVec(&moveDir, pProperty->getGroundUp(), pInput->getMoveVec());
    if (al::normalizeOrZero(&moveDir)) {
        return false;
    }

    sead::Vector3f dir = rDir;
    if (al::normalizeOrZero(&dir)) {
        return false;
    }

    return isOppositeSide(moveDir, dir);
}

/**
 * Turns the player to the horizontal part of their velocity.
 * @param pProperty the player's physical state
 */
void faceToHorizontalVelocity(PlayerProperty* pProperty) {
    sead::Vector3f dir;
    al::verticalizeVec(&dir, pProperty->getGroundUp(), pProperty->getVelocity());
    if (!al::isNearZero(dir, 0.001f)) {
        al::normalize(&dir);
        copyVec(&pProperty->mFront, dir);
    }
}

/**
 * Turns the player to the stick direction.
 * @param pProperty the player's physical state
 * @param pInput the player's input
 */
void faceToInputDirection(PlayerProperty* pProperty, const IUsePlayerInput* pInput) {
    if (!pInput->isStickOn()) {
        return;
    }

    sead::Vector3f dir;
    al::verticalizeVec(&dir, pProperty->getGroundUp(), pInput->getMoveVec());
    if (!al::normalizeOrZero(&dir)) {
        copyVec(&pProperty->mFront, dir);
    }
}

/**
 * Starts a jump: faces the velocity and adds the jump power along the up direction.
 * @param pProperty the player's physical state
 * @param pCollision the player's collision
 * @param jumpPower the jump speed
 * @param isResetVelocity whether the current velocity is dropped first
 */
void setupJump(PlayerProperty* pProperty, IUsePlayerCollision* pCollision, f32 jumpPower,
               bool isResetVelocity) {
    faceToHorizontalVelocity(pProperty);

    if (isResetVelocity) {
        pProperty->mVelocity.set(0.0f, 0.0f, 0.0f);
    } else if (pCollision->isOnFloor()) {
        pProperty->mVelocity.y = 0.0f;
    }

    pProperty->mVelocity += pProperty->getUpDir() * jumpPower;
    pCollision->arrangeJumpFollowVel();
}

/**
 * Computes the jump power from the horizontal speed.
 * @param rVelocity the player's velocity
 * @param rUp the up direction
 * @param speedRate scales the horizontal speed
 * @param speedMin the speed at which the high jump starts
 * @param speedRange the speed range over which the high jump grows
 * @param highRate how much the high jump grows
 * @param highPower the power at speedMin
 * @param lowPower the power when standing
 * @return the jump power
 */
f32 calcJumpPow(const sead::Vector3f& rVelocity, const sead::Vector3f& rUp, f32 speedRate,
                f32 speedMin, f32 speedRange, f32 highRate, f32 highPower, f32 lowPower) {
    sead::Vector3f horizontal;
    al::verticalizeVec(&horizontal, rUp, rVelocity);

    f32 speed = horizontal.length() * speedRate;
    if (speed <= speedMin) {
        f32 rate = sead::Mathf::clamp(speed / speedMin, 0.0f, 1.0f);
        return rate * highPower + (1.0f - rate) * lowPower;
    }

    f32 rate = sead::Mathf::clamp((horizontal.length() - speedMin) / speedRange, 0.0f, 1.0f);
    rate = 1.0f - (1.0f - rate) * (1.0f - rate);
    return (rate * highRate + 1.0f) * highPower;
}

/**
 * Moves a value towards a target by a step without overshooting.
 * @param value the current value
 * @param target the target value
 * @param step the step size
 * @return the new value
 */
f32 converge(f32 value, f32 target, f32 step) {
    if (value > target) {
        value -= step;
        if (value < target) {
            return target;
        }
    } else if (value < target) {
        value += step;
        if (value > target) {
            return target;
        }
    }

    return value;
}

/**
 * Blends a flow vector towards the water flow at a position.
 * @param pAreaUser the area object holder
 * @param pFlow the flow to update
 * @param pAccess the water flow access
 * @param rPos the position
 * @param keepRate how much of the old flow is kept
 */
void calcWaterFlowField(const al::IUseAreaObj* pAreaUser, sead::Vector3f* pFlow,
                        const IUseWaterFlowAccess* pAccess, const sead::Vector3f& rPos,
                        f32 keepRate) {
    sead::Vector3f flow;
    WaterUtil::calcWaterFlowField(pAreaUser, &flow, pAccess, rPos);
    *pFlow = *pFlow * keepRate + flow * (1.0f - keepRate);
}

/**
 * @param speed the speed
 * @param threshold the smallest speed kept
 * @return zero if the speed is below the threshold, the speed otherwise
 */
f32 cutOff(f32 speed, f32 threshold) {
    return speed < threshold ? 0.0f : speed;
}

/**
 * Snaps to the ground while falling onto the floor, solves in-air collision otherwise.
 * @param pCollision the player's collision
 * @param pProperty the player's physical state
 */
void snapGroundOrSolveAir(IUsePlayerCollision* pCollision, const PlayerProperty* pProperty) {
    if (pCollision->isOnFloor() && pProperty->getVelocity().y < 0.0f) {
        pCollision->snapGround();
    } else {
        pCollision->solveAir();
    }
}

/**
 * Accelerates a velocity along a direction and limits its length.
 * @param pVelocity the velocity
 * @param rDir the acceleration direction
 * @param accel the acceleration
 * @param speedMax the maximum speed
 */
void addAndAdjustVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rDir, f32 accel,
                          f32 speedMax) {
    *pVelocity += rDir * accel;
    if (pVelocity->length() > speedMax) {
        setVecLength(pVelocity, speedMax);
    }
}

/**
 * @param speed the vertical speed
 * @param pInput the player's input
 * @param gravity the sinking acceleration
 * @param pParam the player's tuning values
 * @return the vertical swim speed: rising while paddling, sinking otherwise
 */
f32 calcSwimVerticalVelocity(f32 speed, const IUsePlayerInput* pInput, f32 gravity,
                             const PlayerConstParam* pParam) {
    if (pInput->isPrecedingSwimPaddleTrigOn()) {
        return calcSwimRiseVelocity(speed, pParam);
    }

    return calcSwimFallVelocity(speed, gravity, pParam);
}

/**
 * @param speed the vertical speed
 * @param pParam the player's tuning values
 * @return the vertical speed after one frame of rising
 */
f32 calcSwimRiseVelocity(f32 speed, const PlayerConstParam* pParam) {
    speed += pParam->getStandSwimRisePower();
    if (speed > pParam->getStandSwimRiseSpeedMax()) {
        return pParam->getStandSwimRiseSpeedMax();
    }

    return speed;
}

/**
 * @param speed the vertical speed
 * @param gravity the sinking acceleration
 * @param pParam the player's tuning values
 * @return the vertical speed after one frame of sinking
 */
f32 calcSwimFallVelocity(f32 speed, f32 gravity, const PlayerConstParam* pParam) {
    speed -= gravity;
    if (speed < -pParam->getStandSwimFallSpeedMax()) {
        return -pParam->getStandSwimFallSpeedMax();
    }

    return speed;
}

/**
 * Computes the vertical speed while bobbing in water: sink, then hold, then rise.
 * @param speed the vertical speed
 * @param gravity the sinking acceleration
 * @param pParam the player's tuning values
 * @param frame the frames spent bobbing
 * @param unused unused
 * @param chara the playable character
 * @return the new vertical speed
 */
f32 calcBobbleVelocity(f32 speed, f32 gravity, const PlayerConstParam* pParam, s32 frame,
                       s32 unused, EPlayerChara chara) {
    f32 riseStart = 0.0f;
    f32 sinkEnd = 0.0f;
    switch (chara.value()) {
    case EPlayerChara::Mario:
    case EPlayerChara::Luigi:
        sinkEnd = 90.0f;
        riseStart = 112.0f;
        break;
    case EPlayerChara::Peach:
    case EPlayerChara::Rosetta:
        sinkEnd = 100.0f;
        riseStart = 112.0f;
        break;
    default:
        if (chara == EPlayerChara::Kinopio) {
            sinkEnd = 80.0f;
            riseStart = 90.0f;
        }

        break;
    }

    if (frame <= sinkEnd) {
        return calcSwimFallVelocity(speed, gravity, pParam);
    }

    if (frame >= riseStart) {
        return calcSwimRiseVelocity(speed, pParam);
    }

    return speed;
}

/**
 * Accelerates the swim velocity while walking on the floor.
 * @param pVelocity the velocity
 * @param rDir the move direction
 * @param pInput the player's input
 * @param pParam the player's tuning values
 */
void calcSwimWalkVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rDir,
                          const IUsePlayerInput* pInput, const PlayerConstParam* pParam) {
    f32 accel;
    f32 speedMax;
    if (pInput->isDashButtonOn()) {
        accel = pParam->getStandSwimHorizontalFloorDashAccel();
        speedMax = pParam->getStandSwimHorizontalFloorDashSpeedMax();
    } else {
        accel = pParam->getStandSwimHorizontalFloorAccel();
        speedMax = pParam->getStandSwimHorizontalFloorSpeedMax();
    }

    addAndAdjustVelocity(pVelocity, rDir, accel, speedMax);
}

/**
 * Scales the part of a vector perpendicular to an up direction.
 * @param pVec the vector
 * @param rUp the up direction
 * @param rate the scale of the perpendicular part
 */
void decayVerticalVec(sead::Vector3f* pVec, const sead::Vector3f& rUp, f32 rate) {
    sead::Vector3f vertical;
    al::verticalizeVec(&vertical, rUp, *pVec);
    *pVec = (*pVec - vertical) + vertical * rate;
}

/**
 * Updates the horizontal swim velocity from the stick.
 * @param pVelocity the velocity
 * @param pInput the player's input
 * @param pProperty the player's physical state
 * @param pCollision the player's collision
 * @param isHighSpeed whether the fast swim values are used
 * @param pParam the player's tuning values
 * @param isNoSink whether the player swims without sinking
 */
void calcSwimHorizontalVelocity(sead::Vector3f* pVelocity, const IUsePlayerInput* pInput,
                                const PlayerProperty* pProperty,
                                const IUsePlayerCollision* pCollision, bool isHighSpeed,
                                const PlayerConstParam* pParam, bool isNoSink) {
    if (!pInput->isStickOn()) {
        if (al::isNearZero(pVelocity->length(), 0.001f)) {
            pVelocity->set(0.0f, 0.0f, 0.0f);
        } else {
            *pVelocity *= pParam->getStandSwimHorizontalBrakeRate();
        }

        return;
    }

    if (isNoSink &&
        pInput->getMoveVec().length() <= pParam->getNoSinkSwimHorizontalHighInputMin()) {
        return;
    }

    if (isOppositeInput(pInput, pProperty, *pVelocity)) {
        *pVelocity *= pParam->getStandSwimHorizontalBrakeRate();
    } else {
        sead::Vector3f front;
        al::verticalizeVec(&front, pProperty->getUpDir(), pProperty->getFront());
        if (!al::normalizeOrZero(&front)) {
            decayVerticalVec(pVelocity, front, pParam->getStandSwimHorizontalBrakeRate());
        }
    }

    sead::Vector3f moveDir = pInput->getMoveVec();
    al::verticalizeVec(&moveDir, pProperty->getUpDir(), moveDir);
    al::normalizeOrZero(&moveDir);

    if (pCollision->isOnFloor()) {
        calcSwimWalkVelocity(pVelocity, moveDir, pInput, pParam);
        return;
    }

    if (isNoSink) {
        addAndAdjustVelocity(pVelocity, moveDir,
                             pParam->getNoSinkSwimHorizontalHighAccel() * 2.0f,
                             pParam->getNoSinkSwimHorizontalHighSpeedMax());
    } else if (isHighSpeed) {
        addAndAdjustVelocity(pVelocity, moveDir, pParam->getStandSwimHorizontalHighAccel(),
                             pParam->getStandSwimHorizontalHighSpeedMax());
    } else {
        *pVelocity += moveDir * pParam->getStandSwimHorizontalLowAccel();
        if (pVelocity->length() > pParam->getStandSwimHorizontalLowSpeedMax()) {
            *pVelocity *= pParam->getStandSwimHorizontalBrakeRate();
        }
    }
}

/**
 * Removes the part of a velocity along a direction.
 * @param pVelocity the velocity
 * @param rUp the (normalized) direction
 */
void verticalizeVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rUp) {
    *pVelocity -= rUp * rUp.dot(*pVelocity);
}

/**
 * Turns the swimming player towards the stick, by at most a given angle per frame.
 * @param pProperty the player's physical state
 * @param pInput the player's input
 * @param degreeMax the largest turn per frame in degrees
 */
void calcSwimFrontVec(PlayerProperty* pProperty, const IUsePlayerInput* pInput, f32 degreeMax) {
    if (!pInput->isStickOn()) {
        return;
    }

    if (al::isNearDirection(pProperty->getFront(), pProperty->getUpDir(), 0.01f)) {
        sead::Quatf rotate;
        al::makeQuatRotationRate(&rotate, pProperty->getGroundUp(), pProperty->getFront(), 1.0f);
        pProperty->mGroundUp.rotate(rotate);
        pProperty->mFront.rotate(rotate);
    }

    copyVec(&pProperty->mGroundUp, pProperty->getUpDir());

    sead::Vector3f moveDir;
    al::verticalizeVec(&moveDir, pProperty->getGroundUp(), pInput->getMoveVec());
    if (al::normalizeOrZero(&moveDir)) {
        return;
    }

    sead::Vector3f side;
    calcSideDir(&side, pProperty);
    al::normalizeOrZero(&side);

    f32 cos = pProperty->getFront().dot(moveDir);
    f32 sideDot = moveDir.dot(side);
    f32 angle = sead::Mathf::acos(sead::Mathf::clamp(cos, -1.0f, 1.0f));
    f32 angleMax = degreeMax * sead::Mathf::pi() / 180.0f;
    if (angle > angleMax) {
        angle = angleMax;
    }

    if (sideDot < 0.0f) {
        angle = -angle;
    }

    sead::Quatf rotate;
    rotate.setAxisRadian(pProperty->getGroundUp(), angle);
    sead::Vector3f front = pProperty->getFront();
    front.rotate(rotate);
    al::verticalizeVec(&front, pProperty->getGroundUp(), front);
    al::normalize(&front);
    pProperty->setFrontVec(front);
}

/**
 * @param pFigureDirector the player's power-up state
 * @return whether the player has a tanooki power-up
 */
bool isRaccoonDog(const PlayerFigureDirector* pFigureDirector) {
    return pFigureDirector->getFigure() == EPlayerFigure::RaccoonDog ||
           pFigureDirector->getFigure() == EPlayerFigure::RaccoonDogWhite;
}

/**
 * @param pFigureDirector the player's power-up state
 * @return whether the player has a power-up that climbs walls
 */
bool isClimb(const PlayerFigureDirector* pFigureDirector) {
    switch (pFigureDirector->getFigure().value()) {
    case EPlayerFigure::Climb:
    case EPlayerFigure::Manekineko:
    case EPlayerFigure::ClimbWhite:
    case EPlayerFigure::ClimbGiga:
        return true;
    default:
        return false;
    }
}

/**
 * @param pFigureDirector the player's power-up state
 * @return whether the player is giga cat
 */
bool isClimbGiga(const PlayerFigureDirector* pFigureDirector) {
    return pFigureDirector->getFigure() == EPlayerFigure::ClimbGiga;
}

/**
 * Scales the part of a vector along a direction.
 * @param pVec the vector
 * @param rDir the (normalized) direction
 * @param scale the scale of the parallel part
 */
void scaleVecOfDir(sead::Vector3f* pVec, const sead::Vector3f& rDir, f32 scale) {
    f32 dot = pVec->dot(rDir);
    *pVec -= rDir * dot;
    *pVec += rDir * (dot * scale);
}

/**
 * Turns the player towards the stick, sidestepping a turn of more than 90 degrees.
 * @param pProperty the player's physical state
 * @param pInput the player's input
 * @param rate how much of the target direction is taken
 */
void controlDirection(PlayerProperty* pProperty, const IUsePlayerInput* pInput, f32 rate) {
    sead::Vector3f moveDir = pInput->getMoveVec();
    al::normalize(&moveDir);

    sead::Vector3f front = pProperty->getFront();
    al::verticalizeVec(&front, pProperty->getUpDir(), front);
    if (al::normalizeOrZero(&front)) {
        copyVec(&front, moveDir);
    }

    pProperty->mFront = moveDir;
    if (front.dot(moveDir) < 6.1232e-17f) {
        sead::Vector3f side;
        side.setCross(pProperty->getUpDir(), front);
        al::verticalizeVec(&side, pProperty->getUpDir(), side);
        al::normalizeOrZero(&side);
        if (side.dot(moveDir) < 0.0f) {
            side = -side;
        }

        copyVec(&pProperty->mFront, side);
    }

    pProperty->mFront = pProperty->getFront() * rate + front * (1.0f - rate);
    al::normalize(&pProperty->mFront);
}

/**
 * Starts the squat start animation unless a rolling, squat or back jump animation plays.
 * @param pAnimator the player's animator
 * @param pAnimName the animation to start
 * @return whether the animation was started
 */
bool setupSquatStartAnim(IUsePlayerAnimator* pAnimator, const char* pAnimName) {
    if (pAnimator->isAnim("Rolling") || pAnimator->isAnim("RollingAir") ||
        pAnimator->isAnim("RollingAirShort") || pAnimator->isAnim("RollingShort") ||
        pAnimator->isAnim("RollingPropeller") || pAnimator->isAnim("RollingShortPropeller") ||
        pAnimator->isAnim("JumpBack") || pAnimator->isAnim("JumpBackHigh") ||
        pAnimator->isAnim("SquatLand") || pAnimator->isAnim("SquatWait") ||
        pAnimator->isAnim("SquatStart") || pAnimator->isAnim("GigaSquatStart") ||
        pAnimator->isAnim("JumpBroad") || pAnimator->isAnim("TailAttackSquatAir") ||
        pAnimator->isAnim("TailAttackSquatGround") || pAnimator->isAnim("JumpBackStart") ||
        pAnimator->isAnim("SquatWalk") || pAnimator->isAnim("TrampleJumpBack") ||
        pAnimator->isAnim("WallHitLandSquat")) {
        return false;
    }

    pAnimator->startAnim(pAnimName);
    return true;
}

/**
 * Brakes the forward velocity while the stick points backwards, then steers sideways.
 * @param pVelocity the velocity
 * @param pProperty the player's physical state
 * @param pInput the player's input
 * @param brakeRate scales the backward stick into a speed change
 * @param speedMin the lowest forward speed
 * @param sideRate the sideways steering rate
 */
void controlDirectionalVelocity(sead::Vector3f* pVelocity, const PlayerProperty* pProperty,
                                const IUsePlayerInput* pInput, f32 brakeRate, f32 speedMin,
                                f32 sideRate) {
    const sead::Vector3f& rFront = pProperty->getFront();
    f32 input = rFront.dot(pInput->getMoveVec());
    if (input < 0.0f) {
        f32 speed = rFront.dot(*pVelocity);
        *pVelocity -= rFront * speed;
        speed += input * brakeRate;
        if (speed < speedMin) {
            speed = speedMin;
        }

        *pVelocity += pProperty->getFront() * speed;
    }

    controlSideVelocity(pVelocity, pProperty, pInput, sideRate);
}

/**
 * Steers the velocity sideways with the stick.
 * @param pVelocity the velocity
 * @param pProperty the player's physical state
 * @param pInput the player's input
 * @param rate the steering rate
 */
void controlSideVelocity(sead::Vector3f* pVelocity, const PlayerProperty* pProperty,
                         const IUsePlayerInput* pInput, f32 rate) {
    sead::Vector3f moveDir = pInput->getMoveVec();
    if (al::normalizeOrZero(&moveDir)) {
        return;
    }

    sead::Vector3f side;
    calcSideDir(&side, pProperty);
    al::normalize(&side);
    *pVelocity += side * (side.dot(moveDir) * rate);
}

/**
 * Updates the front and the horizontal velocity while squat walking.
 * @param pProperty the player's physical state
 * @param pInput the player's input
 * @param speed the walking speed
 * @param turnRate how much of the old front is kept
 * @param decayRate the speed decay without input
 */
void updateSquatVelocity(PlayerProperty* pProperty, const IUsePlayerInput* pInput, f32 speed,
                         f32 turnRate, f32 decayRate) {
    sead::Vector3f horizontal;
    al::verticalizeVec(&horizontal, pProperty->getUpDir(), pProperty->getVelocity());
    sead::Vector3f vertical;
    al::parallelizeVec(&vertical, pProperty->getUpDir(), pProperty->getVelocity());

    if (pInput->isStickOn()) {
        sead::Vector3f moveDir = pInput->getMoveVec();
        al::normalize(&moveDir);
        if (pProperty->getFront().dot(moveDir) < 0.0f) {
            sead::Vector3f side;
            calcSideDir(&side, pProperty);
            al::normalize(&side);
            if (side.dot(moveDir) >= 0.0f) {
                copyVec(&moveDir, side);
            } else {
                moveDir = -side;
            }
        }

        moveDir = moveDir * (1.0f - turnRate) + pProperty->getFront() * turnRate;
        al::normalize(&moveDir);
        pProperty->setFrontVec(moveDir);
        horizontal += (moveDir * speed - horizontal) * 0.5f;
    } else {
        horizontal *= decayRate;
    }

    pProperty->mVelocity = horizontal + vertical;
}

/**
 * Applies a dead zone to a stick axis.
 * @param stick the stick axis value
 * @return the axis value rescaled past the dead zone
 */
f32 calcStickPow(f32 stick) {
    f32 sign = stick < 0.0f ? -1.0f : 1.0f;
    f32 pow = sead::Mathf::clamp((sign * stick - 0.1f) / 0.9f, 0.0f, 1.0f);
    return sign * pow;
}

/**
 * @param pCollision the player's collision
 * @param pCode the map code
 * @return whether any touched side has a map code
 */
bool checkMapCode(const IUsePlayerCollision* pCollision, const char* pCode) {
    IUsePlayerCollision::Info info;
    for (PlayerCollisionIterator it(pCollision); !it.isEnd(); ++it) {
        if (!it.isOn()) {
            continue;
        }

        it.getInfo(&info);
        if (al::isEqualString(info.mMapCode, pCode)) {
            return true;
        }
    }

    return false;
}

/**
 * @param pCollision the player's collision
 * @param pCode the material code
 * @return whether the floor has a material code
 */
bool checkMaterialCode(const IUsePlayerCollision* pCollision, const char* pCode) {
    if (!pCollision->isOnFloor()) {
        return false;
    }

    IUsePlayerCollision::Info info;
    pCollision->getFloorInfo(&info);
    return al::isEqualString(info.mMaterialCode, pCode);
}

/**
 * @param pCollision the player's collision
 * @return whether the player stands on ice
 */
bool isMapCodeSkate(const IUsePlayerCollision* pCollision) {
    if (!pCollision->isOnFloor()) {
        return false;
    }

    IUsePlayerCollision::Info info;
    pCollision->getFloorInfo(&info);
    return al::isEqualString(info.mMapCode, "Skate");
}

/**
 * @param pFigureDirector the player's power-up state
 * @return the height of the player's center relative to their size
 */
f32 calcCenterRate(const PlayerFigureDirector* pFigureDirector) {
    return pFigureDirector->getFigure() == EPlayerFigure::Mini ? 0.25f : 0.5f;
}

/**
 * Turns the ground up and front towards the up direction, flipping them first when upside down.
 * @param pProperty the player's physical state
 */
void turnHeadUpForce(PlayerProperty* pProperty) {
    if (pProperty->getGroundUp().dot(pProperty->getUpDir()) < -0.999f) {
        pProperty->mGroundUp.negate();
        pProperty->mFront.negate();
    }

    sead::Quatf rotate;
    al::makeQuatRotationRate(&rotate, pProperty->getGroundUp(), pProperty->getUpDir(), 1.0f);
    pProperty->mGroundUp.rotate(rotate);
    al::normalize(&pProperty->mGroundUp);
    pProperty->mFront.rotate(rotate);
    al::normalize(&pProperty->mFront);
}

/**
 * Computes the direction down a slope.
 * @param pOut receives the downward direction
 * @param pProperty the player's physical state
 * @param rNormal the slope's normal
 */
void calcDownward(sead::Vector3f* pOut, const PlayerProperty* pProperty,
                  const sead::Vector3f& rNormal) {
    sead::Vector3f side;
    side.setCross(pProperty->getUpDir(), rNormal);
    if (al::normalizeOrZero(&side)) {
        calcSideDir(&side, pProperty);
        al::verticalizeVec(&side, rNormal, side);
        al::normalize(&side);
    }

    pOut->setCross(side, rNormal);
}

}  // namespace PlayerActionFunc

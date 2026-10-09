#include "Player/Giga/WaitRollingParam.hpp"

#include <math/seadVector.h>

#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerAttack.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerCollisionSize.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerTrigger.hpp"


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 WaitRollingParam::getMinSpeed() const {
    return mConstParam->getWaitRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 WaitRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getWaitRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 WaitRollingParam::getBrakeRate() const {
    return mConstParam->getWaitRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 WaitRollingParam::getSideBrakeRate() const {
    return mConstParam->getWaitRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 WaitRollingParam::getSideAccel() const {
    return mConstParam->getWaitRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 WaitRollingParam::getSideMaxSpeed() const {
    return mConstParam->getWaitRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* WaitRollingParam::getAnimName() const {
    return "RollingShort";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* WaitRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* WaitRollingParam::getAnimNameWithEquipment() const {
    return "RollingShortPropeller";
}


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 NormalRollingParam::getMinSpeed() const {
    return mConstParam->getNormalRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 NormalRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getNormalRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 NormalRollingParam::getBrakeRate() const {
    return mConstParam->getNormalRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 NormalRollingParam::getSideBrakeRate() const {
    return mConstParam->getNormalRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 NormalRollingParam::getSideAccel() const {
    return mConstParam->getNormalRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 NormalRollingParam::getSideMaxSpeed() const {
    return mConstParam->getNormalRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* NormalRollingParam::getAnimName() const {
    return "Rolling";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* NormalRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* NormalRollingParam::getAnimNameWithEquipment() const {
    return "RollingPropeller";
}


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 AirRollingParam::getMinSpeed() const {
    return mConstParam->getAirRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 AirRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getAirRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 AirRollingParam::getBrakeRate() const {
    return mConstParam->getAirRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 AirRollingParam::getSideBrakeRate() const {
    return mConstParam->getAirRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 AirRollingParam::getSideAccel() const {
    return mConstParam->getAirRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 AirRollingParam::getSideMaxSpeed() const {
    return mConstParam->getAirRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* AirRollingParam::getAnimName() const {
    return "Rolling";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* AirRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* AirRollingParam::getAnimNameWithEquipment() const {
    return "RollingPropeller";
}

/**
 * @brief Gets the upward speed the roll starts with.
 * @return The jump speed.
 */
f32 AirRollingParam::getJumpPow() const {
    return mConstParam->getAirRollingJumpPow();
}

/**
 * @brief Gets the extra gravity applied during the roll.
 * @return The gravity addition.
 */
f32 AirRollingParam::getGravityAddition() const {
    return mConstParam->getAirRollingGravity();
}


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 DashRollingParam::getMinSpeed() const {
    return mConstParam->getRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 DashRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 DashRollingParam::getBrakeRate() const {
    return mConstParam->getRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 DashRollingParam::getSideBrakeRate() const {
    return mConstParam->getRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 DashRollingParam::getSideAccel() const {
    return mConstParam->getRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 DashRollingParam::getSideMaxSpeed() const {
    return mConstParam->getRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* DashRollingParam::getAnimName() const {
    return "Rolling";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* DashRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* DashRollingParam::getAnimNameWithEquipment() const {
    return "RollingPropeller";
}


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 RaccoonDogWaitRollingParam::getMinSpeed() const {
    return mConstParam->getRaccoonDogWaitRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 RaccoonDogWaitRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getRaccoonDogWaitRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 RaccoonDogWaitRollingParam::getBrakeRate() const {
    return mConstParam->getRaccoonDogWaitRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 RaccoonDogWaitRollingParam::getSideBrakeRate() const {
    return mConstParam->getRaccoonDogWaitRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 RaccoonDogWaitRollingParam::getSideAccel() const {
    return mConstParam->getRaccoonDogWaitRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 RaccoonDogWaitRollingParam::getSideMaxSpeed() const {
    return mConstParam->getRaccoonDogWaitRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingParam::getAnimName() const {
    return "RollingShort";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingParam::getAnimNameWithEquipment() const {
    return "RollingShortPropeller";
}


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 RaccoonDogNormalRollingParam::getMinSpeed() const {
    return mConstParam->getRaccoonDogNormalRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 RaccoonDogNormalRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getRaccoonDogNormalRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 RaccoonDogNormalRollingParam::getBrakeRate() const {
    return mConstParam->getRaccoonDogNormalRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 RaccoonDogNormalRollingParam::getSideBrakeRate() const {
    return mConstParam->getRaccoonDogNormalRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 RaccoonDogNormalRollingParam::getSideAccel() const {
    return mConstParam->getRaccoonDogNormalRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 RaccoonDogNormalRollingParam::getSideMaxSpeed() const {
    return mConstParam->getRaccoonDogNormalRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* RaccoonDogNormalRollingParam::getAnimName() const {
    return "RollingShort";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* RaccoonDogNormalRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* RaccoonDogNormalRollingParam::getAnimNameWithEquipment() const {
    return "RollingShortPropeller";
}


/**
 * @brief Gets the speed under which the roll ends.
 * @return The minimum speed.
 */
f32 RaccoonDogDashRollingParam::getMinSpeed() const {
    return mConstParam->getRaccoonDogDashRollingMinSpeed();
}

/**
 * @brief Gets how many frames the roll keeps its speed before braking.
 * @return The frame count.
 */
f32 RaccoonDogDashRollingParam::getNoBrakeFrame() const {
    return static_cast<u32>(mConstParam->getRaccoonDogDashRollingNoBrakeFrame());
}

/**
 * @brief Gets how fast the roll slows down.
 * @return The brake rate.
 */
f32 RaccoonDogDashRollingParam::getBrakeRate() const {
    return mConstParam->getRaccoonDogDashRollingBrakeRate();
}

/**
 * @brief Gets how fast sideways movement slows down.
 * @return The side brake rate.
 */
f32 RaccoonDogDashRollingParam::getSideBrakeRate() const {
    return mConstParam->getRaccoonDogDashRollingSideBrakeRate();
}

/**
 * @brief Gets the sideways acceleration from the stick.
 * @return The side acceleration.
 */
f32 RaccoonDogDashRollingParam::getSideAccel() const {
    return mConstParam->getRaccoonDogDashRollingSideAccel();
}

/**
 * @brief Gets the maximum sideways speed.
 * @return The maximum side speed.
 */
f32 RaccoonDogDashRollingParam::getSideMaxSpeed() const {
    return mConstParam->getRaccoonDogDashRollingSideMaxSpeed();
}

/**
 * @brief Gets the roll animation.
 * @return The animation name.
 */
const char* RaccoonDogDashRollingParam::getAnimName() const {
    return "RollingShort";
}

/**
 * @brief Gets the roll animation used while climbing.
 * @return The animation name.
 */
const char* RaccoonDogDashRollingParam::getClimbAnimName() const {
    return "ClimbSlidingAttack";
}

/**
 * @brief Gets the roll animation used with the propeller equipped.
 * @return The animation name.
 */
const char* RaccoonDogDashRollingParam::getAnimNameWithEquipment() const {
    return "RollingShortPropeller";
}

/**
 * @brief Constructs the rolling attack action.
 * @param pArg The player systems.
 * @param pCollisionSize The collision size, squatted while the attack runs.
 * @param pTrigger The sensor triggers (for trampling something).
 * @param pParam The tuning values of the attack.
 * @param pFigureDirector The figure director (for the raccoon dog and climb forms).
 * @param pAttack The attack that runs the raccoon dog tail attack.
 */
PlayerActionRollingAttack::PlayerActionRollingAttack(const PlayerActionArg* pArg,
                                                     IUsePlayerCollisionSize* pCollisionSize,
                                                     const PlayerTrigger* pTrigger,
                                                     const IUseRollingAttackParam* pParam,
                                                     const PlayerFigureDirector* pFigureDirector,
                                                     IUsePlayerAttack* pAttack)
    : mArg(pArg), mCollisionSize(pCollisionSize), mParam(pParam), mTrigger(pTrigger),
      mFigureDirector(pFigureDirector), mAttack(pAttack), mIsTailAttack(false) {}

/**
 * @brief Moves the player through the air.
 */
void PlayerActionRollingAttack::move() {
    mArg->mCollision->solveAir();
}

/**
 * @brief Steers the horizontal velocity, applies gravity and bounces off trampled objects.
 */
void PlayerActionRollingAttack::update() {
    PlayerProperty* pProperty = mArg->mProperty;
    const sead::Vector3f& rUp = pProperty->mUpDir;
    sead::Vector3f hVelocity;
    al::verticalizeVec(&hVelocity, rUp, pProperty->mVelocity);
    controlHVelocity(hVelocity);

    sead::Vector3f vVelocity;
    if (mTrigger->isOn(PlayerTrigger::cTrample)) {
        mArg->mAnimator->startAnim("TrampleRollingAir");
        vVelocity = rUp * mParam->getJumpPow();
    } else {
        al::parallelizeVec(&vVelocity, rUp, mArg->mProperty->mVelocity);
        vVelocity -= rUp * mParam->getGravity();

        if (vVelocity.dot(rUp) < -mParam->getFallSpeedMax()) {
            f32 fallSpeedMax = mParam->getFallSpeedMax();
            f32 length = vVelocity.length();
            if (length > 0.0f) {
                vVelocity *= fallSpeedMax / length;
            }
        }
    }

    mArg->mProperty->mVelocity = vVelocity + hVelocity;

    if (mIsTailAttack && !PlayerActionFunc::isRaccoonDog(mFigureDirector)) {
        mAttack->endTailAttack();
    }
}

/**
 * @brief Steers the horizontal velocity with the stick.
 * @param rVelocity The horizontal velocity to steer.
 */
void PlayerActionRollingAttack::controlHVelocity(sead::Vector3f& rVelocity) {
    PlayerActionFunc::controlDirectionalVelocity(
        &rVelocity, mArg->mProperty, mArg->mInput, mArg->mConstParam->getCommonRollingAttackBrake(),
        mArg->mConstParam->getCommonRollingAttackSpeedMin(),
        mArg->mConstParam->getCommonRollingAttackSideAccel());
}

/**
 * @brief Starts the attack: launches the player forward and up and starts the animation.
 */
void PlayerActionRollingAttack::setup() {
    PlayerProperty* pProperty = mArg->mProperty;
    const sead::Vector3f& rUp = pProperty->mUpDir;
    sead::Vector3f hVelocity;
    al::verticalizeVec(&hVelocity, rUp, pProperty->mVelocity);  // overwritten right away
    hVelocity = mArg->mProperty->mFront * mParam->getAttackVel();

    sead::Vector3f vVelocity;
    al::parallelizeVec(&vVelocity, rUp, mArg->mProperty->mVelocity);
    f32 jumpPow = mParam->getJumpPow();
    f32 vSpeed = vVelocity.dot(rUp);
    if (vSpeed < 0.0f) {
        jumpPow += vSpeed;
    }

    mArg->mProperty->mVelocity = hVelocity + rUp * jumpPow;

    if (PlayerActionFunc::isRaccoonDog(mFigureDirector)) {
        if (!mArg->mAnimator->isAnim("TailAttackSquatAir")) {
            mArg->mAnimator->startAnim("TailAttackSquatAir");
        }

        mAttack->startTailAttack();
        mIsTailAttack = true;
    } else {
        if (PlayerActionFunc::isClimb(mFigureDirector)) {
            mArg->mAnimator->startAnim(mParam->getClimbAnimName());
        } else {
            mArg->mAnimator->startAnim(mParam->getAnimName());
        }

        mIsTailAttack = false;
    }

    mArg->mAnimator->setAnimRate(mParam->getAnimRate());
    mCollisionSize->squat();
}

/**
 * @brief Ends the tail attack if it still runs and stands the collision back up.
 */
void PlayerActionRollingAttack::teardown() {
    if (mIsTailAttack) {
        mAttack->endTailAttack();
    }

    mCollisionSize->standUp();
}

/**
 * @brief Gets the horizontal speed of the attack.
 * @return The attack speed.
 */
f32 DashRollingAttackParam::getAttackVel() const {
    return mConstParam->getRollingAttackVelH();
}

/**
 * @brief Gets the upward speed of the attack.
 * @return The jump speed.
 */
f32 DashRollingAttackParam::getJumpPow() const {
    return mConstParam->getRollingAttackJumpPow();
}

/**
 * @brief Gets the gravity during the attack.
 * @return The gravity.
 */
f32 DashRollingAttackParam::getGravity() const {
    return mConstParam->getRollingAttackJumpGravity();
}

/**
 * @brief Gets the maximum fall speed during the attack.
 * @return The maximum fall speed.
 */
f32 DashRollingAttackParam::getFallSpeedMax() const {
    return mConstParam->getFallSpeedMax();
}

/**
 * @brief Gets the attack animation.
 * @return The animation name.
 */
const char* DashRollingAttackParam::getAnimName() const {
    return "RollingAir";
}

/**
 * @brief Gets the attack animation used while climbing.
 * @return The animation name.
 */
const char* DashRollingAttackParam::getClimbAnimName() const {
    return "ClimbRollingAttack";
}

/**
 * @brief Gets the horizontal speed of the attack.
 * @return The attack speed.
 */
f32 NormalRollingAttackParam::getAttackVel() const {
    return mConstParam->getNormalRollingAttackVelH();
}

/**
 * @brief Gets the upward speed of the attack.
 * @return The jump speed.
 */
f32 NormalRollingAttackParam::getJumpPow() const {
    return mConstParam->getNormalRollingAttackJumpPow();
}

/**
 * @brief Gets the gravity during the attack.
 * @return The gravity.
 */
f32 NormalRollingAttackParam::getGravity() const {
    return mConstParam->getNormalRollingAttackJumpGravity();
}

/**
 * @brief Gets the maximum fall speed during the attack.
 * @return The maximum fall speed.
 */
f32 NormalRollingAttackParam::getFallSpeedMax() const {
    return mConstParam->getFallSpeedMax();
}

/**
 * @brief Gets the attack animation.
 * @return The animation name.
 */
const char* NormalRollingAttackParam::getAnimName() const {
    return "RollingAirShort";
}

/**
 * @brief Gets the attack animation used while climbing.
 * @return The animation name.
 */
const char* NormalRollingAttackParam::getClimbAnimName() const {
    return "ClimbRollingAttack";
}

/**
 * @brief Gets the horizontal speed of the attack.
 * @return The attack speed.
 */
f32 WaitRollingAttackParam::getAttackVel() const {
    return mConstParam->getWaitRollingAttackVelH();
}

/**
 * @brief Gets the upward speed of the attack.
 * @return The jump speed.
 */
f32 WaitRollingAttackParam::getJumpPow() const {
    return mConstParam->getWaitRollingAttackJumpPow();
}

/**
 * @brief Gets the gravity during the attack.
 * @return The gravity.
 */
f32 WaitRollingAttackParam::getGravity() const {
    return mConstParam->getWaitRollingAttackJumpGravity();
}

/**
 * @brief Gets the maximum fall speed during the attack.
 * @return The maximum fall speed.
 */
f32 WaitRollingAttackParam::getFallSpeedMax() const {
    return mConstParam->getFallSpeedMax();
}

/**
 * @brief Gets the attack animation.
 * @return The animation name.
 */
const char* WaitRollingAttackParam::getAnimName() const {
    return "RollingAirShort";
}

/**
 * @brief Gets the attack animation used while climbing.
 * @return The animation name.
 */
const char* WaitRollingAttackParam::getClimbAnimName() const {
    return "ClimbRollingAttack";
}

/**
 * @brief Gets the horizontal speed of the attack.
 * @return The attack speed.
 */
f32 RaccoonDogWaitRollingAttackParam::getAttackVel() const {
    return mConstParam->getRaccoonDogWaitRollingAttackVelH();
}

/**
 * @brief Gets the upward speed of the attack.
 * @return The jump speed.
 */
f32 RaccoonDogWaitRollingAttackParam::getJumpPow() const {
    return mConstParam->getRaccoonDogWaitRollingAttackJumpPow();
}

/**
 * @brief Gets the gravity during the attack.
 * @return The gravity.
 */
f32 RaccoonDogWaitRollingAttackParam::getGravity() const {
    return mConstParam->getRaccoonDogWaitRollingAttackJumpGravity();
}

/**
 * @brief Gets the maximum fall speed during the attack.
 * @return The maximum fall speed.
 */
f32 RaccoonDogWaitRollingAttackParam::getFallSpeedMax() const {
    return mConstParam->getFallSpeedMax();
}

/**
 * @brief Gets the attack animation.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingAttackParam::getAnimName() const {
    return "RollingAirShort";
}

/**
 * @brief Gets the attack animation used while climbing.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingAttackParam::getClimbAnimName() const {
    return "ClimbRollingAttack";
}

/**
 * @brief Gets the horizontal speed of the attack.
 * @return The attack speed.
 */
f32 RaccoonDogWaitRollingAttackHighParam::getAttackVel() const {
    return mConstParam->getRaccoonDogWaitRollingAttackHighVelH();
}

/**
 * @brief Gets the upward speed of the attack.
 * @return The jump speed.
 */
f32 RaccoonDogWaitRollingAttackHighParam::getJumpPow() const {
    return mConstParam->getRaccoonDogWaitRollingAttackHighJumpPow();
}

/**
 * @brief Gets the gravity during the attack.
 * @return The gravity.
 */
f32 RaccoonDogWaitRollingAttackHighParam::getGravity() const {
    return mConstParam->getRaccoonDogWaitRollingAttackHighJumpGravity();
}

/**
 * @brief Gets the maximum fall speed during the attack.
 * @return The maximum fall speed.
 */
f32 RaccoonDogWaitRollingAttackHighParam::getFallSpeedMax() const {
    return mConstParam->getFallSpeedMax();
}

/**
 * @brief Gets the attack animation.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingAttackHighParam::getAnimName() const {
    return "RollingAirShort";
}

/**
 * @brief Gets the attack animation used while climbing.
 * @return The animation name.
 */
const char* RaccoonDogWaitRollingAttackHighParam::getClimbAnimName() const {
    return "ClimbRollingAttack";
}

#include "MapObj/BallStateThrow.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
    NERVE_DECL(BallStateThrow, Throw);
    NERVE_DECL(BallStateThrow, ThrowAuto);
    NERVES_MAKE_NOSTRUCT(BallStateThrow, Throw, ThrowAuto)
}

BallStateThrowParam::BallStateThrowParam()
    : mLaunchSpeed(30.0f), mLaunchSpeedUp(8.0f), mThrowGravity(0.45f), mGravity(1.5f),
      mWaterGravity(0.5f), mVelocityScale(0.996f), mWaterVelocityScale(0.65f),
      mAirRotationSpeed(8.0f), mThrowFrames(45), mRotateOnAir(true), mIsAutoThrow(false) {}

BallStateThrowParam::BallStateThrowParam(float launchSpeed, float launchSpeedUp,
    float throwGravity, float gravity, float waterGravity, float velocityScale,
    float waterVelocityScale, float airRotationSpeed, int throwFrames, bool rotateOnAir,
    bool isAutoThrow)
    : mLaunchSpeed(launchSpeed), mLaunchSpeedUp(launchSpeedUp), mThrowGravity(throwGravity),
      mGravity(gravity), mWaterGravity(waterGravity), mVelocityScale(velocityScale),
      mWaterVelocityScale(waterVelocityScale), mAirRotationSpeed(airRotationSpeed),
      mThrowFrames(throwFrames), mRotateOnAir(rotateOnAir), mIsAutoThrow(isAutoThrow) {}

BallStateThrow::BallStateThrow(al::LiveActor* pActor, const BallStateThrowParam* pParam)
    : al::ActorStateBase("ボール投げ状態", pActor), mParam(pParam) {
    initNerve(&NrvBallStateThrowThrow, 0);
}

void BallStateThrow::appear() {
    al::ActorStateBase::appear();
    if (mParam->mIsAutoThrow)
        al::setNerve(this, &NrvBallStateThrowThrowAuto);
    else
        al::setNerve(this, &NrvBallStateThrowThrow);
}

void BallStateThrow::setThrowParam(const al::HitSensor* pSensor,
    const BallStateThrowParam* pParam, const al::LiveActor* pThrower) {
    mThrower = pThrower;
    mThrowSensor = pSensor;
    mParam = pParam;
}

void BallStateThrow::exeThrow() {
    al::LiveActor* actor = mHostActor;
    bool inWater = rc::isInWaterArea(actor);
    if (al::isFirstStep(this)) {
        if (mThrowSensor) {
            sead::Vector3f direction;
            if (al::isSensorPlayer(mThrowSensor)) {
                direction.set(rc::getPlayerFront(mThrowSensor));
            } else {
                direction.set(sead::Vector3f::ez);
                al::calcDirBetweenSensorsH(&direction, mThrowSensor, al::getHitSensor(actor, 0));
            }
            BallStateFunction::calcLaunchSpeed(actor, direction, mParam);
        }
        BallStateFunction::setPositionOnRelease(actor, mThrowSensor, mThrower);
    }
    float gravity = al::isGreaterEqualStep(this, mParam->mThrowFrames) ?
        mParam->mGravity : mParam->mThrowGravity;
    if (inWater)
        gravity = mParam->mWaterGravity;
    al::addVelocityToGravity(actor, gravity);
    al::scaleVelocity(actor, inWater ? mParam->mWaterVelocityScale : mParam->mVelocityScale);
    BallStateFunction::rotateOnAir(actor, mParam->mAirRotationSpeed, mParam->mRotateOnAir);
    BallStateFunction::sendMsgToCollision(actor, false);
    BallStateFunction::reboundCollisionWallOrCeiling(actor);
    if (al::isOnGround(actor, 0, 0.0f))
        kill();
}

void BallStateThrow::exeThrowAuto() {
    al::LiveActor* actor = mHostActor;
    bool inWater = rc::isInWaterArea(actor);
    al::addVelocityToGravity(actor, inWater ? mParam->mWaterGravity : mParam->mGravity);
    al::scaleVelocity(actor, inWater ? mParam->mWaterVelocityScale : mParam->mVelocityScale);
    BallStateFunction::rotateOnAir(actor, mParam->mAirRotationSpeed, mParam->mRotateOnAir);
    BallStateFunction::sendMsgToCollision(actor, false);
    BallStateFunction::reboundCollisionWallOrCeiling(actor);
    if (al::isOnGround(actor, 0, 0.0f))
        kill();
}

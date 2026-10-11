#include "MapObj/BallStateFallParam.hpp"
#include "MapObj/BallStateFall.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
    NERVE_DECL(BallStateFall, Fall);
    NERVES_MAKE_NOSTRUCT(BallStateFall, Fall)
}

BallStateFallParam::BallStateFallParam()
    : _00(28.0f), _04(11.0f), _08(0.6f), mReboundRate(0.7f), mWaterReboundRate(0.1f),
      mReboundMinSpeed(10.0f), mAirRotationSpeed(8.0f), mGravity(1.3f), mWaterGravity(0.5f),
      mVelocityScale(0.996f), mWaterVelocityScale(0.65f), mReboundFriction(0.7f) {}

BallStateFallParam::BallStateFallParam(float a, float b, float c, float reboundRate,
    float waterReboundRate, float reboundMinSpeed, float airRotationSpeed, float gravity,
    float waterGravity, float velocityScale, float waterVelocityScale, float reboundFriction)
    : _00(a), _04(b), _08(c), mReboundRate(reboundRate), mWaterReboundRate(waterReboundRate),
      mReboundMinSpeed(reboundMinSpeed), mAirRotationSpeed(airRotationSpeed), mGravity(gravity),
      mWaterGravity(waterGravity), mVelocityScale(velocityScale),
      mWaterVelocityScale(waterVelocityScale), mReboundFriction(reboundFriction) {}

BallStateFall::BallStateFall(al::LiveActor* pActor, const BallStateFallParam* pParam)
    : al::ActorStateBase("ボール投げ状態", pActor), mParam(pParam) {
    initNerve(&NrvBallStateFallFall, 0);
}

void BallStateFall::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvBallStateFallFall);
}

void BallStateFall::exeFall() {
    al::LiveActor* actor = mHostActor;
    bool isInWater = rc::isInWaterArea(actor);
    al::addVelocityToGravity(actor, isInWater ? mParam->mWaterGravity : mParam->mGravity);
    al::scaleVelocity(actor, isInWater ? mParam->mWaterVelocityScale : mParam->mVelocityScale);
    BallStateFunction::rotateOnAir(actor, mParam->mAirRotationSpeed, mIsRotate);
    BallStateFunction::sendMsgToCollision(actor, false);
    BallStateFunction::reboundCollisionWallOrCeiling(actor);
    if (!BallStateFunction::reboundCollisionGround(actor, mParam, isInWater))
        kill();
}

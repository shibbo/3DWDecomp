#include "MapObj/BallStateRolling.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(BallStateRolling, Rolling);
    NERVES_MAKE_NOSTRUCT(BallStateRolling, Rolling)
}

BallStateRollingParam::BallStateRollingParam()
    : mGravity(1.2f), mVelocityScale(0.95f), mStopSpeed(0.5f), mStopFrames(10) {}

BallStateRollingParam::BallStateRollingParam(float gravity, float velocityScale,
    float stopSpeed, int stopFrames)
    : mGravity(gravity), mVelocityScale(velocityScale), mStopSpeed(stopSpeed),
      mStopFrames(stopFrames) {}

BallStateRolling::BallStateRolling(al::LiveActor* pActor, const BallStateRollingParam* pParam)
    : al::ActorStateBase("ボール投げ状態", pActor), mParam(pParam) {
    initNerve(&NrvBallStateRollingRolling, 0);
}

void BallStateRolling::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvBallStateRollingRolling);
    mSlowFrames = 0;
    mAirFrames = 0;
    mPreviousPos.set(al::getTrans(mHostActor));
}

void BallStateRolling::exeRolling() {
    al::LiveActor* actor = mHostActor;
    al::addVelocityToGravity(actor, mParam->mGravity);
    al::scaleVelocity(actor, mParam->mVelocityScale);
    BallStateFunction::sendMsgToCollision(actor, false);
    if (al::isOnGround(actor, 0, 0.0f))
        mAirFrames = 0;
    else if (++mAirFrames >= 5) {
        kill();
        return;
    }
    if (sead::Mathf::abs(BallStateFunction::calcSpeedFront(actor)) <= mParam->mStopSpeed &&
        al::isOnGround(actor, 0, 0.0f)) {
        if (++mSlowFrames >= mParam->mStopFrames) {
            mSlowFrames = 0;
            al::setVelocityZero(actor);
            kill();
            return;
        }
    } else {
        BallStateFunction::rotateOnGround(actor, mPreviousPos);
        if (mSlowFrames - 1 >= 0)
            --mSlowFrames;
    }
    mPreviousPos.set(al::getTrans(actor));
}

bool BallStateRolling::isOnAir() {
    return mAirFrames > 4;
}

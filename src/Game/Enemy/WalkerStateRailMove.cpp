#include "Enemy/WalkerStateRailMove.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(WalkerStateRailMove, ReturnToRail)
NERVE_DECL(WalkerStateRailMove, RailMove)
NERVE_DECL(WalkerStateRailMove, Turn)
NERVES_MAKE_NOSTRUCT(WalkerStateRailMove, ReturnToRail, RailMove, Turn)
const WalkerStateRailMoveParam cDefaultParam;
}

/** @brief Initializes the default rail walking settings. */
WalkerStateRailMoveParam::WalkerStateRailMoveParam()
    : mSpeed(3.0f), mReturnDistance(50.0f), mRailTurnDegree(4.0f), mTurnDegree(4.0f),
      mReturnAccel(0.6f), mReturnTurnDegree(8.0f), mTurnAlongGround(false),
      mWalkAction("Walk"), mTurnAction("Walk") {}

/** @brief Initializes rail walking with a custom speed.
 * @param speed Distance traveled per frame along the rail.
 */
WalkerStateRailMoveParam::WalkerStateRailMoveParam(f32 speed)
    : mSpeed(speed), mReturnDistance(50.0f), mRailTurnDegree(4.0f), mTurnDegree(4.0f),
      mReturnAccel(0.6f), mReturnTurnDegree(8.0f), mTurnAlongGround(false),
      mWalkAction("Walk"), mTurnAction("Walk") {}

/** @brief Initializes all rail walking settings.
 * @param speed Rail movement speed.
 * @param returnDistance Distance at which to align with the rail.
 * @param railTurnDegree Turning angle while following the rail.
 * @param turnDegree Turning angle when reversing direction.
 * @param returnAccel Acceleration toward the rail.
 * @param returnTurnDegree Turning angle while returning to the rail.
 * @param pWalkAction Walking animation.
 * @param pTurnAction Turning animation.
 * @param turnAlongGround Whether return movement follows the ground.
 */
WalkerStateRailMoveParam::WalkerStateRailMoveParam(f32 speed, f32 returnDistance,
        f32 railTurnDegree, f32 turnDegree, f32 returnAccel, f32 returnTurnDegree,
        const char* pWalkAction, const char* pTurnAction, bool turnAlongGround)
    : mSpeed(speed), mReturnDistance(returnDistance), mRailTurnDegree(railTurnDegree),
      mTurnDegree(turnDegree), mReturnAccel(returnAccel), mReturnTurnDegree(returnTurnDegree),
      mTurnAlongGround(turnAlongGround), mWalkAction(pWalkAction), mTurnAction(pTurnAction) {}

/** @brief Initializes rail movement and records the actor and rail clipping bounds.
 * @param pHost Actor moved along the rail.
 * @param pParam Passive movement settings.
 * @param pRailMoveParam Rail settings, or nullptr for defaults.
 */
WalkerStateRailMove::WalkerStateRailMove(al::LiveActor* pHost, const WalkerStateParam* pParam,
        const WalkerStateRailMoveParam* pRailMoveParam)
    : al::ActorStateBase("歩行型敵レール移動状態", pHost), mParam(pParam), mRailMoveParam(pRailMoveParam) {
    initNerve(&NrvWalkerStateRailMoveReturnToRail, 0);
    if (!mRailMoveParam) { mRailMoveParam = &cDefaultParam; }
    mOriginalClippingRadius = al::getClippingRadius(mHostActor);
    al::setRailPosToStart(mHostActor);
    al::setRailClippingInfo(&mRailCenter, mHostActor, 100.0f, 100.0f);
    mRailClippingRadius = al::getClippingRadius(mHostActor);
    setClippingInfoOrigin();
}

/** @brief Restores clipping centered on the actor. */
void WalkerStateRailMove::setClippingInfoOrigin() {
    al::setClippingInfo(mHostActor, mOriginalClippingRadius, al::getTransPtr(mHostActor));
}

/** @brief Starts returning to the nearest point on the rail. */
void WalkerStateRailMove::appear() {
    al::ActorStateBase::appear();
    al::setRailPosToNearestPos(mHostActor, al::getTrans(mHostActor));
    al::setNerve(this, &NrvWalkerStateRailMoveReturnToRail);
}

/** @brief Ends rail movement and restores actor clipping. */
void WalkerStateRailMove::kill() {
    al::ActorStateBase::kill();
    setClippingInfoOrigin();
}

/** @brief Places the actor at the start of its rail. */
void WalkerStateRailMove::setPositionToStart() {
    al::setRailPosToStart(mHostActor);
    al::syncRailTrans(mHostActor);
    al::resetPosition(mHostActor, false);
}

/** @brief Uses clipping bounds covering the rail. */
void WalkerStateRailMove::setClippingInfoRail() {
    al::setClippingInfo(mHostActor, mRailClippingRadius, &mRailCenter);
}

/** @brief Walks toward the rail and aligns with its direction. */
void WalkerStateRailMove::exeReturnToRail() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(mHostActor, mRailMoveParam->mWalkAction.cstr(), 0)) {
            al::startAction(mHostActor, mRailMoveParam->mWalkAction.cstr());
        }
        al::setRailPosToNearestPos(mHostActor, al::getTrans(mHostActor));
    }
    if ((al::getRailPos(mHostActor) - al::getTrans(mHostActor)).squaredLength() <
            mRailMoveParam->mReturnDistance * mRailMoveParam->mReturnDistance) {
        al::addVelocityToTarget(mHostActor, al::getRailPos(mHostActor), mRailMoveParam->mReturnAccel);
        al::LiveActor* pHost = mHostActor;
        const f32 gravity = mParam->mGravity;
        al::scaleVelocityHV(pHost, mParam->mGroundFriction, mParam->mAirFriction);
        if (!al::isCollidedGround(pHost)) {
            al::addVelocityToGravity(pHost, gravity);
        }
        if (al::turnToRailDir(mHostActor, mRailMoveParam->mRailTurnDegree)) {
            al::setNerve(this, &NrvWalkerStateRailMoveRailMove);
        }
    } else {
        al::walkAndTurnToTarget(mHostActor, al::getRailPos(mHostActor), mRailMoveParam->mReturnAccel,
            mParam->mGravity, mParam->mGroundFriction, mRailMoveParam->mReturnTurnDegree,
            mRailMoveParam->mTurnAlongGround);
    }
}

/** @brief Follows the rail until reaching an endpoint. */
void WalkerStateRailMove::exeRailMove() {
    if (al::isFirstStep(this)) {
        setClippingInfoRail();
        al::setVelocityZero(mHostActor);
        if (!al::isSklAnimPlaying(mHostActor, mRailMoveParam->mWalkAction.cstr(), 0)) {
            al::startAction(mHostActor, mRailMoveParam->mWalkAction.cstr());
        }
        sead::Vector3f railDir(0.0f, 0.0f, 0.0f);
        al::calcRailDirAtCoord(&railDir, mHostActor, al::getRailCoord(mHostActor));
        if (!al::isRailGoingToEnd(mHostActor)) { railDir = -railDir; }
        al::turnDirectionDegree(mHostActor, al::getFrontPtr(mHostActor), railDir, 180.0f);
    }
    if (al::turnToRailDir(mHostActor, mRailMoveParam->mRailTurnDegree)) {
        al::moveSyncRail(mHostActor, mRailMoveParam->mSpeed);
    }
    if (al::isRailReachedGoal(mHostActor)) { al::setNerve(this, &NrvWalkerStateRailMoveTurn); }
}

/** @brief Reverses the rail direction and turns before resuming movement. */
void WalkerStateRailMove::exeTurn() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(mHostActor, mRailMoveParam->mTurnAction.cstr(), 0)) {
            al::startAction(mHostActor, mRailMoveParam->mTurnAction.cstr());
        }
        al::reverseRail(mHostActor);
        al::setVelocityZero(mHostActor);
        if (al::isNearZero(mTurnDir, 0.001f)) { mTurnDir = -al::getFront(mHostActor); }
    }
    if (al::turnDirectionDegree(mHostActor, al::getFrontPtr(mHostActor), mTurnDir, mRailMoveParam->mTurnDegree)) {
        al::setNerve(this, &NrvWalkerStateRailMoveRailMove);
        mTurnDir = sead::Vector3f::zero;
    }
}

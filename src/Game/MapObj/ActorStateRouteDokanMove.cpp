#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/RouteDokanInOutEffect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    NERVE_DECL(ActorStateRouteDokanMove, Start);
    NERVE_DECL(ActorStateRouteDokanMove, Move);
    NERVES_MAKE_NOSTRUCT(ActorStateRouteDokanMove, Start, Move)
}

ActorStateRouteDokanMove::ActorStateRouteDokanMove(al::LiveActor* pActor, const al::ActorInitInfo& rInfo)
    : al::ActorStateBase("ルート土管移動状態", pActor) {
    mEffect = new RouteDokanInOutEffect("ルート土管出入りエフェクト");
    al::initCreateActorNoPlacementInfoNoViewId(mEffect, rInfo);
    mRailRider = new al::BlockRailRider();
    initNerve(&NrvActorStateRouteDokanMoveStart, 0);
}
ActorStateRouteDokanMove::~ActorStateRouteDokanMove() {}

void ActorStateRouteDokanMove::appear() {
    al::setNerve(this, &NrvActorStateRouteDokanMoveStart);
    mIsDead = false;
}

void ActorStateRouteDokanMove::exeStart() {
    if (al::isFirstStep(this)) {
        sead::Vector3f position;
        sead::Vector3f direction;
        mRailRider->calcPosAndDir(&position, &direction);
        mEffect->startIn(position, direction);
        al::setVelocityZero(mHostActor);
        al::tryStartSe(mHostActor, "StateRouteDokanIn", nullptr);
        al::tryStartSe(mHostActor, "StateRouteDokanMove", nullptr);
    }
    al::setNerve(this, &NrvActorStateRouteDokanMoveMove);
}

void ActorStateRouteDokanMove::exeMove() {
    mRailRider->move(mMoveSpeed, al::getTransPtr(mHostActor), &mMoveDirection);
    sead::Vector3f front(mMoveDirection);
    front.y = 0.0f;
    if (!al::normalizeOrZero(&front))
        mFrontDirection.set(front);
    if (mRailRider->isReachEnd()) {
        al::setVelocity(mHostActor, mEndSpeed * mMoveDirection);
        mEffect->startOut(al::getTrans(mHostActor), mMoveDirection);
        al::tryStopSe(mHostActor, "StateRouteDokanMove");
        al::tryStartSe(mHostActor, "StateRouteDokanOut", nullptr);
        kill();
    }
}

bool ActorStateRouteDokanMove::tryStart(al::HitSensor* pSelf, al::HitSensor* pOther) {
    sead::Vector3f direction;
    al::calcDirBetweenSensors(&direction, pSelf, pOther);
    if (!(al::getVelocity(mHostActor).dot(direction) > 0.0f))
        return false;
    if (!rc::sendMsgBlockRailRide(pOther, pSelf, mRailRider))
        return false;
    direction.y = 0.0f;
    if (al::normalizeOrZero(&mFrontDirection, direction)) {
        al::calcFrontDir(&mFrontDirection, mHostActor);
        mFrontDirection.y = 0.0f;
        if (al::normalizeOrZero(&mFrontDirection))
            mFrontDirection.set(sead::Vector3f::ez);
    }
    return true;
}

void ActorStateRouteDokanMove::setMoveSpeed(float speed) { mMoveSpeed = speed < 0.0f ? 0.0f : speed; }
void ActorStateRouteDokanMove::setEndSpeed(float speed) { mEndSpeed = speed < 0.0f ? 0.0f : speed; }
void ActorStateRouteDokanMove::setRouteSelecter(al::BlockRailRouteSelecter* pSelecter) {
    mRailRider->setRouteSelecter(pSelecter);
}
void ActorStateRouteDokanMove::forceCalcMoveDirection() {
    mRailRider->calcDir(&mMoveDirection);
    mMoveDirection.x = -mMoveDirection.x;
    mMoveDirection.y = -mMoveDirection.y;
    mMoveDirection.z = -mMoveDirection.z;
}

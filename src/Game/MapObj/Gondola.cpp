#include "Game/MapObj/Gondola.hpp"
#include "Game/MapObj/RideUserInfoKeeper.hpp"
#include "Game/Util/ControlUserUtil.hpp"
#include "Game/Util/ProjectMsgUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Physics/RotateHangDynamics.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
NERVE_DECL(Gondola, Appear)
NERVE_DECL(Gondola, RailRunBeforeWait)
NERVE_DECL(Gondola, RailRun)
NERVE_DECL(Gondola, RailReachGoal)
NERVE_DECL(Gondola, Disappear)
NERVE_DECL(Gondola, DisappearSound)
NERVES_MAKE_NOSTRUCT(Gondola, Appear, RailRunBeforeWait, RailRun, RailReachGoal, Disappear,
                     DisappearSound)
} // namespace
class GondolaBody : public al::LiveActor {
  public:
    GondolaBody(const char* name, Gondola* host) : LiveActor(name), mHost(host) {}
    bool receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                    al::HitSensor* receiver) override {
        return mHost->receiveMsg(msg, sender, receiver);
    }

  private:
    Gondola* mHost;
};
Gondola::Gondola(const char* name) : LiveActor(name) {}
void Gondola::init(const al::ActorInitInfo& info) {
    mRideInfo = new RideUserInfoKeeper();
    mDynamics = new al::RotateHangDynamics();
    mBody = new GondolaBody("ゴンドラ本体部分", this);
    al::initActorWithArchiveName(mBody, info, "GondolaBody", nullptr);
    al::invalidateClipping(mBody);
    mBody->makeActorAppeared();
    al::initActorWithArchiveName(this, info, "GondolaGear", nullptr);
    al::initSubActorKeeperNoFile(this, info, 1);
    al::registerSubActorSyncClipping(this, mBody, false);
    mFrame =
        al::createCollisionObj(mBody, info, "GondolaFrame",
                               al::getHitSensor(mBody, "FrameCollisionParts"), "Center", nullptr);
    mFrame->appear();
    al::trySyncStageSwitchAppear(this);
    al::listenStageSwitchOnKill(
        this, al::FunctorV0M<Gondola*, void (Gondola::*)()>(this, &Gondola::killBySwitch));
    mRadius = al::calcModelBoundingSphereRadius(this);
    if (al::isExistRail(this)) {
        al::tryGetArg(&mSpeedMin, info, "RailRunSpeedMin");
        al::tryGetArg(&mSpeedMax, info, "RailRunSpeedMax");
        al::tryGetArg(&mGravity, info, "RailRunGravity");
        al::tryGetArg(&mDamping, info, "RailRunDamp");
        al::initNerve(this, &NrvGondolaAppear, 0);
        al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 100.0f);
        al::setSyncRailToStart(this);
        al::resetPosition(this, false);
        mDynamics->initPosLength(al::getTrans(this), 600.0f, sead::Vector3f::ey);
        mDynamics->_3c = 0.75f;
        mDynamics->_44 = 5.0f;
        mDynamics->_48 = 2000.0f;
        mDynamics->mDamping = 0.99f;
        mDynamics->mMaxAngle = 360.0f;
        al::setTrans(mBody, mDynamics->mTipPos);
        al::resetPosition(mBody, false);
    } else {
        makeActorDead();
    }
}
void Gondola::killBySwitch() {
    al::LiveActor::kill();
    mBody->kill();
    mFrame->kill();
}
void Gondola::updateDynamics() {
    sead::Vector3f gravity = -sead::Vector3f::ey;
    mDynamics->moveRootPos(al::getTrans(this));
    mDynamics->applyGravity(gravity);
    mDynamics->update(1.0f / 60.0f);
    sead::Vector3f up = mDynamics->mRootPos - mDynamics->mTipPos;
    al::normalizeOrZero(&up);
    const sead::Vector3f& front = al::getRailDir(this);
    al::setTrans(mBody, mDynamics->mTipPos);
    al::calcQuatUp(&up, mDynamics->mQuat);
    sead::Quatf quat;
    if (al::isParallelDirection(front, up, 0.01f))
        al::makeQuatUpNoSupport(&quat, up);
    else
        al::makeQuatUpFront(&quat, up, front);
    al::setQuat(mBody, quat);
}
void Gondola::exeAppear() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::showModelIfHide(mBody);
        al::validateCollisionParts(mBody);
        al::validateCollisionParts(mFrame);
        mSpeed = 0.0f;
        al::setSyncRailToStart(this);
        sead::Vector3f pos = al::getTrans(this);
        al::resetPosition(this, pos, false);
        al::validateClipping(this);
        mDynamics->mMaxAngle = 360.0f;
        mDynamics->mOmega = sead::Vector3f::zero;
        mDynamics->moveRootPosAndFix(pos);
    }
    updateDynamics();
    if (al::isGreaterStep(this, 2) && mRideInfo->isRideAtLeastOne()) {
        al::startAction(mBody, "Ride");
        al::setNerve(this, &NrvGondolaRailRunBeforeWait);
    }
}
void Gondola::exeRailRunBeforeWait() {
    if (al::isFirstStep(this)) {
        mSpeed = 0.0f;
        al::invalidateClipping(this);
    }
    updateDynamics();
    if (mRideInfo->isIncreasedRideUser())
        al::startAction(mBody, "Ride");
    if (mRideInfo->isRideAtLeastOne()) {
        if (al::isGreaterStep(this, 80))
            al::setNerve(this, &NrvGondolaRailRun);
    } else {
        al::setNerve(this, &NrvGondolaRailRunBeforeWait);
    }
}
void Gondola::exeRailRun() {
    if (al::isFirstStep(this))
        al::startAction(mBody, "RailRun");
    al::invalidateClipping(this);
    const sead::Vector3f& dir = al::getRailDir(this);
    sead::Vector3f velocity = mSpeed * dir;
    sead::Vector3f gravity = sead::Vector3f::ey;
    gravity *= mGravity;
    velocity -= gravity;
    velocity = mDamping * velocity;
    al::calcRailToGoalLength(this);
    mSpeed = sead::Mathf::clamp(dir.dot(velocity), mSpeedMin, mSpeedMax);
    al::moveSyncRail(this, mSpeed);
    updateDynamics();
    if (al::isRailReachedNearGoal(this, 5.0f)) {
        al::setNerve(this, &NrvGondolaRailReachGoal);
        return;
    }
    al::holdSeWithParam(this, "PgRailMove", mSpeed, nullptr);
}
void Gondola::exeRailReachGoal() {
    if (al::isFirstStep(this))
        al::startAction(mBody, "Arrival");
    mDynamics->mMaxAngle = 0.0f;
    mDynamics->mOmega = sead::Vector3f::zero;
    al::moveSyncRail(this, mSpeed);
    updateDynamics();
    if (al::isActionEnd(mBody)) {
        al::setVelocityZero(this);
        mSpeed = 0.0f;
        al::setNerve(this, &NrvGondolaDisappear);
    }
}
void Gondola::exeDisappear() {
    if (al::isFirstStep(this))
        al::startAction(mBody, "Disappear");
    if (al::isActionEnd(mBody)) {
        al::invalidateCollisionParts(mBody);
        al::invalidateCollisionParts(mFrame);
        al::setNerve(this, &NrvGondolaDisappearSound);
    }
}
void Gondola::exeDisappearSound() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::hideModelIfShow(mBody);
    }
    if (al::isGreaterStep(this, 120)) {
        al::startAction(mBody, "Appear");
        al::setNerve(this, &NrvGondolaAppear);
    }
}
void Gondola::control() {
    if (al::isNerve(this, &NrvGondolaRailRun) || al::isNerve(this, &NrvGondolaRailReachGoal) ||
        mRideInfo->isRideAtLeastOne()) {
        al::tryStartMclAnimIfNotPlaying(this, "On");
        al::tryStartMclAnimIfNotPlaying(mBody, "On");
    } else {
        al::tryStartMclAnimIfNotPlaying(this, "Off");
        al::tryStartMclAnimIfNotPlaying(mBody, "Off");
    }
    mRideInfo->resetInfo(this);
}
bool Gondola::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerFloorTouch(msg)) {
        rc::sendMsgRequestTouchFromHoldedPlayer(sender, receiver);
        if (receiver != al::getHitSensor(mBody, "FrameCollisionParts"))
            mRideInfo->setUserRide(rc::findControlUserId(sender));
        sead::Vector3f gravity = sead::Vector3f::ey * 5.0f;
        sead::Vector3f force;
        force = al::getActorVelocity(sender);
        force *= 0.0f;
        force -= gravity;
        force = force * 10.0f;
        mDynamics->applyForcePos(force, al::getSensorPos(sender));
        return false;
    }
    if (al::isMsgPlayerHipDropAll(msg)) {
        sead::Vector3f force = sead::Vector3f::ey * -1000.0f;
        force = force * 10.0f;
        mDynamics->applyForcePos(force, al::getSensorPos(sender));
        if (!al::isNerve(this, &NrvGondolaDisappear) && !al::isExistAction(mBody, "Hipdrop"))
            al::startAction(mBody, "Hipdrop");
        return false;
    }
    if (al::isMsgPlayerUpperPunch(msg)) {
        sead::Vector3f force = sead::Vector3f::ey * 500.0f;
        force = force * 10.0f;
        mDynamics->applyForcePos(force, al::getSensorPos(sender));
        return true;
    }
    return false;
}

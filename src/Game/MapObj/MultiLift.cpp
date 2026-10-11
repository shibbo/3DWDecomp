#include "MapObj/MultiLift.hpp"
#include "MapObj/RideUserInfoKeeper.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
    NERVE_DECL(MultiLift, WaitForRide);
    NERVE_DECL(MultiLift, RideCheck);
    NERVE_DECL(MultiLift, PreMove);
    NERVE_DECL(MultiLift, Move);
    NERVE_DECL(MultiLift, Stop);
    NERVE_DECL(MultiLift, Wait);
    NERVES_MAKE_NOSTRUCT(MultiLift, WaitForRide, RideCheck, PreMove, Move, Stop, Wait)
}
MultiLift::MultiLift(const char* name) : al::LiveActor(name) {}
MultiLift::~MultiLift() {}
void MultiLift::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvMultiLiftWaitForRide, 0);
    al::tryGetArg(&mRequiredRiders, info, "RideNum");
    al::tryGetArg(&mAntiLightStrong, info, "IsAntiLightStrong");
    mRiders = new RideUserInfoKeeper;
    mKeyPoses = al::createKeyPoseKeeper(info);
    float radius;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoses, 0.0f);
    radius = al::getClippingRadius(this) + radius;
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::killPrePassLightAll(this, 0);
    makeActorAppeared();
}
void MultiLift::exeWaitForRide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitForRide");
        if (mAntiLightStrong) al::startMclAnim(this, "WaitForRideLight");
    }
    if (mRiders->isRideGreaterEqual(mRequiredRiders)) al::setNerve(this, &NrvMultiLiftRideCheck);
}
void MultiLift::exeRideCheck() {
    if (mRiders->isRideGreaterEqual(mRequiredRiders)) {
        if (al::isGreaterStep(this, 3)) al::setNerve(this, &NrvMultiLiftPreMove);
    } else al::setNerve(this, &NrvMultiLiftWaitForRide);
}
void MultiLift::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mMoveTime = al::calcKeyMoveWaitTime(mKeyPoses);
    }
    if (al::isGreaterEqualStep(this, mMoveTime)) al::setNerve(this, &NrvMultiLiftMove);
}
void MultiLift::exePreMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PreMove");
        if (mAntiLightStrong) al::startMclAnim(this, "PreMoveLight");
        al::appearPrePassLightAll(this, -1);
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvMultiLiftMove);
}
void MultiLift::exeMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Move");
        mMoveTime = al::calcKeyMoveMoveTime(mKeyPoses);
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoses, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoses);
        if (al::isStop(mKeyPoses)) al::setNerve(this, &NrvMultiLiftStop);
        else al::setNerve(this, &NrvMultiLiftWait);
    }
}
void MultiLift::exeStop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Stop");
        if (mAntiLightStrong) al::startMclAnim(this, "StopLight");
        al::killPrePassLightAll(this, -1);
    }
}
void MultiLift::control() {
    bool stop = al::isNerve(this, &NrvMultiLiftStop);
    int frame = 0;
    if (mRequiredRiders == 2) frame = stop ? 3 : 0;
    else if (mRequiredRiders == 3) frame = stop ? 4 : 1;
    else if (mRequiredRiders == 4) frame = stop ? 5 : 2;
    al::startMtpAnimAndSetFrameAndStop(this, "MultiLift", frame);
    mRiders->resetInfo(this);
}
bool MultiLift::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerFloorTouch(msg)) {
        rc::sendMsgRequestTouchFromHoldedPlayer(sender, receiver);
        mRiders->setUserRide(rc::findControlUserId(sender));
    }
    return false;
}

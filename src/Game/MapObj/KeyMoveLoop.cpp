#include "MapObj/KeyMoveLoopLift.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
    NERVE_DECL(KeyMoveLoopLift, StandBy);
    NERVE_DECL(KeyMoveLoopLift, StartSign);
    NERVE_DECL(KeyMoveLoopLift, Appear);
    NERVE_DECL(KeyMoveLoopLift, Wait);
    NERVE_DECL(KeyMoveLoopLift, MoveSign);
    NERVE_DECL(KeyMoveLoopLift, Move);
    NERVE_DECL(KeyMoveLoopLift, StopSign);
    NERVE_DECL(KeyMoveLoopLift, Stop);
    NERVES_MAKE_NOSTRUCT(KeyMoveLoopLift, StandBy, StartSign, Appear, Wait, MoveSign, Move, StopSign, Stop)
}
KeyMoveLoopLift::KeyMoveLoopLift(const char* pName) : al::LiveActor(pName) {}
KeyMoveLoopLift::~KeyMoveLoopLift() {}
void KeyMoveLoopLift::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    mKeyPoseKeeper->setMoveTypeStop();
    al::setKeyMoveClippingInfo(this, &mClippingCenter, mKeyPoseKeeper);
    al::initNerve(this, &NrvKeyMoveLoopLiftStandBy, 0);
    makeActorDead();
}
bool KeyMoveLoopLift::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgFloorTouch(pMsg)) {
        if (al::isNerve(this, &NrvKeyMoveLoopLiftStandBy)) {
            al::setNerve(this, &NrvKeyMoveLoopLiftStartSign);
            return true;
        }
        return false;
    }
    return false;
}
void KeyMoveLoopLift::startStandBy() { al::setNerve(this, &NrvKeyMoveLoopLiftStandBy); start(); }
void KeyMoveLoopLift::start() {
    al::resetKeyPose(mKeyPoseKeeper);
    al::setQuat(this, al::getCurrentKeyQuat(mKeyPoseKeeper));
    al::setTrans(this, al::getCurrentKeyTrans(mKeyPoseKeeper));
    al::resetPosition(this, false);
    makeActorAppeared();
}
void KeyMoveLoopLift::startAppearAndStandBy() {
    al::startAction(this, "Appear");
    al::setNerve(this, &NrvKeyMoveLoopLiftStandBy);
    start();
}
void KeyMoveLoopLift::startAppear() { al::setNerve(this, &NrvKeyMoveLoopLiftAppear); start(); }
bool KeyMoveLoopLift::isStandByEnd() const { return !al::isNerve(this, &NrvKeyMoveLoopLiftStandBy); }
void KeyMoveLoopLift::exeStandBy() {}
void KeyMoveLoopLift::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKeyMoveLoopLiftWait);
}
void KeyMoveLoopLift::exeStartSign() {
    if (al::isFirstStep(this)) al::startAction(this, "StartSign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKeyMoveLoopLiftWait);
}
void KeyMoveLoopLift::exeMoveSign() {
    if (al::isFirstStep(this)) al::startAction(this, "MoveSign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKeyMoveLoopLiftMove);
}
void KeyMoveLoopLift::exeStopSign() {
    if (al::isFirstStep(this)) al::startAction(this, "StopSign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKeyMoveLoopLiftStop);
}
void KeyMoveLoopLift::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        int waitTime = al::calcKeyMoveWaitTime(mKeyPoseKeeper);
        if (waitTime >= 0) mWaitTime = waitTime;
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) {
        if (al::isMoveSignKey(mKeyPoseKeeper)) al::setNerve(this, &NrvKeyMoveLoopLiftMoveSign);
        else al::setNerve(this, &NrvKeyMoveLoopLiftMove);
    }
}
void KeyMoveLoopLift::exeMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Move");
        mMoveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper);
    }
    sead::Vector3f previous = al::getTrans(this);
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    float distance = al::calcDistance(this, previous);
    al::tryHoldSeWithParam(this, "PgMoveWithParam", distance, nullptr);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoseKeeper);
        if (al::isStop(mKeyPoseKeeper)) al::setNerve(this, &NrvKeyMoveLoopLiftStopSign);
        else al::setNerve(this, &NrvKeyMoveLoopLiftWait);
    }
}
void KeyMoveLoopLift::exeStop() {
    if (al::isFirstStep(this)) {
        al::startHitReactionDisappear(this);
        kill();
    }
}

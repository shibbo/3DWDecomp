#include "MapObj/GoalPoleStateRunaway.hpp"
#include "MapObj/GoalPole.hpp"
#include "MapObj/GoalPoleWing.hpp"
#include "MapObj/GoalPoleRunawayStep.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/PlayerUtil.hpp"
#include <cmath>
namespace {
NERVE_DECL(GoalPoleStateRunaway, StandBy)
NERVE_DECL(GoalPoleStateRunaway, Wait)
NERVE_DECL(GoalPoleStateRunaway, Move)
NERVE_DECL(GoalPoleStateRunaway, WaitNearPlayer)
NERVE_DECL(GoalPoleStateRunaway, Stop)
NERVES_MAKE_NOSTRUCT(GoalPoleStateRunaway, StandBy, Wait, Move, WaitNearPlayer, Stop)
}
GoalPoleStateRunaway::GoalPoleStateRunaway(GoalPole* pole, const al::ActorInitInfo& info,
    const sead::Matrix34f* baseMtx) : al::NerveStateBase("ゴールポール逃げるステート"), mPole(pole) {
    mKeyPose = al::createKeyPoseKeeper(info);
    al::tryGetArg(&mMoveStopDistance, info, "MoveStopDistance");
    al::tryGetArg(&mMoveRestartDistance, info, "MoveRestartDistance");
    if (al::listenStageSwitchOnStart(mPole, al::Functor(this, &GoalPoleStateRunaway::start)))
        initNerve(&NrvGoalPoleStateRunawayStandBy, 0);
    else initNerve(&NrvGoalPoleStateRunawayWait, 0);
    sead::Vector3f first, last;
    al::getKeyPoseTrans(&first, mKeyPose, 0);
    al::getKeyPoseTrans(&last, mKeyPose, al::getKeyPoseCount(mKeyPose) - 1);
    mForward.setSub(last, first);
    if (al::normalizeOrZero(&mForward)) mForward.set(sead::Vector3f::ez);
    mWing = new GoalPoleWing(mPole);
    al::initCreateActorNoPlacementInfo(mWing, info);
    mStep = new GoalPoleRunawayStep(baseMtx);
    al::initCreateActorNoPlacementInfo(mStep, info);
}
void GoalPoleStateRunaway::start() {
    mWing->appear();
    mPole->startLerpFlag();
    al::startAction(mPole, "RunawayStart");
    al::setNerve(this, &NrvGoalPoleStateRunawayWait);
}
void GoalPoleStateRunaway::kill() {
    al::NerveStateBase::kill();
    if (al::isAlive(mWing)) mWing->kill();
}
void GoalPoleStateRunaway::appearStep() { mStep->appear(); }
void GoalPoleStateRunaway::startMove() {
    mMoveTime = al::calcKeyMoveMoveTime(mKeyPose);
    mMoveStep = 0;
    al::setNerve(this, &NrvGoalPoleStateRunawayMove);
}
void GoalPoleStateRunaway::updateWaitTrans() {
    al::setTrans(mPole, al::getCurrentKeyTrans(mKeyPose));
    addFloatTrans();
}
void GoalPoleStateRunaway::addFloatTrans() {
    al::getTransPtr(mPole)->y += sinf(sead::Mathf::deg2rad(mFloatAngle)) * 30.0f;
    mFloatAngle = al::wrapValue(mFloatAngle + 3.0f, 360.0f);
}
void GoalPoleStateRunaway::updateMoveTransAndRotate() {
    float rate = float(mMoveStep) / float(mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(mPole), mKeyPose, rate);
    addFloatTrans();
    updateRotate();
}
void GoalPoleStateRunaway::updateRotate() {
    sead::Vector3f dir;
    al::calcDirToNextKey(&dir, mKeyPose);
    al::turnQuatFrontToDirDegreeH(mPole, -dir, 4.0f);
}
bool GoalPoleStateRunaway::isMoveStop(float distance) const {
    if (al::calcDistanceNextKeyTrans(mKeyPose) < 1100.0f) return false;
    sead::Vector3f dir;
    al::calcDirToNextKey(&dir, mKeyPose);
    if (dir.dot(mForward) < 0.0f) return false;
    int playerNum = al::getPlayerNumMax(mPole);
    for (int i = 0; i < playerNum; ++i) {
        auto* player = al::getPlayerActor(mPole, i);
        if (rc::isPlayerDeadOrBubble(player)) continue;
        sead::Vector3f toPlayer;
        al::calcDirToActor(&toPlayer, mPole, player);
        if (dir.dot(toPlayer) > sead::Mathf::cos(sead::Mathf::deg2rad(120.0f))) return false;
    }
    return rc::tryFindNearestActivePlayerActorInSphere(mPole, distance) == nullptr;
}
void GoalPoleStateRunaway::exeStandBy() {}
void GoalPoleStateRunaway::exeWait() {
    if (al::isFirstStep(this)) mWaitTime = al::calcKeyMoveWaitTime(mKeyPose);
    updateWaitTrans();
    updateRotate();
    if (al::isGreaterEqualStep(this, mWaitTime)) startMove();
}
void GoalPoleStateRunaway::exeMove() {
    updateMoveTransAndRotate();
    if (!(mMoveStopDistance <= 0.0f) && isMoveStop(mMoveStopDistance)) {
        al::setNerve(this, &NrvGoalPoleStateRunawayWaitNearPlayer);
        return;
    }
    if (mMoveStep == mMoveTime) {
        al::nextKeyPose(mKeyPose);
        _3c = 0;
        if (al::isStop(mKeyPose)) al::setNerve(this, &NrvGoalPoleStateRunawayStop);
        else if (al::calcKeyMoveWaitTime(mKeyPose) >= 1) al::setNerve(this, &NrvGoalPoleStateRunawayWait);
        else startMove();
        return;
    }
    ++mMoveStep;
}
void GoalPoleStateRunaway::exeWaitNearPlayer() {
    updateMoveTransAndRotate();
    if (!isMoveStop(mMoveRestartDistance)) al::setNerve(this, &NrvGoalPoleStateRunawayMove);
}
void GoalPoleStateRunaway::exeStop() { updateWaitTrans(); }

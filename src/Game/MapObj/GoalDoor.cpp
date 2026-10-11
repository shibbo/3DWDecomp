#include "MapObj/GoalDoor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
namespace {
    NERVE_DECL(GoalDoor, Wait);
    NERVE_DECL(GoalDoor, Open);
    NERVE_DECL(GoalDoor, Cancel);
    NERVE_DECL(GoalDoor, End);
    NERVE_DECL(GoalDoor, OpenWait);
    NERVE_DECL(GoalDoor, Close);
    NERVES_MAKE_NOSTRUCT(GoalDoor, Wait, Open, Cancel, End, OpenWait, Close)
}
GoalDoor::GoalDoor(const char* name) : al::LiveActor(name) {}
GoalDoor::~GoalDoor() {}
void GoalDoor::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "WarpDoor", nullptr);
    al::initNerve(this, &NrvGoalDoorWait, 0);
    makeActorAppeared();
}
void GoalDoor::control() { if (mTouchTimer - 1 >= 0) --mTouchTimer; }
bool GoalDoor::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerTouch(msg)) { mTouchTimer = 3; return true; }
    if (al::isMsgBindStart(msg)) {
        if (al::isNerve(this, &NrvGoalDoorWait) && mTouchTimer > 0 && rc::isPlayerOnGround(sender) && !mPuppet) return true;
        return false;
    }
    if (al::isMsgBindInit(msg)) {
        mPuppet = rc::startPuppet(receiver, sender);
        al::sendMsgWarpStart(sender, receiver);
        al::invalidateClipping(this);
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        rc::setPuppetFrontVec(mPuppet, -front);
        rc::startPuppetAction(mPuppet, "DoorIn");
        rc::hidePuppetSilhouette(mPuppet);
        rc::forceEndSubActionPuppet(mPuppet);
        al::setNerve(this, &NrvGoalDoorOpen);
        return true;
    }
    if (al::isMsgBindCancel(msg)) {
        if (isGoal()) return false;
        al::sendMsgWarpEnd(rc::getPuppetSensor(mPuppet), al::getHitSensor(this, nullptr));
        rc::showPuppet(mPuppet);
        rc::showPuppetSilhouette(mPuppet);
        mPuppet = nullptr;
        al::validateClipping(this);
        al::setNerve(this, &NrvGoalDoorCancel);
        return true;
    }
    return false;
}
bool GoalDoor::isGoal() const { return al::isNerve(this, &NrvGoalDoorEnd); }
bool GoalDoor::isEndGoalDemo() const { return al::isNerve(this, &NrvGoalDoorEnd); }
void GoalDoor::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "CloseWait"); }
void GoalDoor::exeOpen() {
    if (al::isFirstStep(this)) al::startAction(this, "Open");
    sead::Vector3f target = al::getTrans(this);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    sead::Vector3f current = rc::getPuppetTrans(mPuppet);
    sead::Vector3f offset = target - current;
    if (offset.length() < 10.0f) rc::setPuppetTrans(mPuppet, target);
    else {
        offset.normalize();
        rc::setPuppetTrans(mPuppet, current + offset * 10.0f);
    }
    rc::setPuppetFrontVec(mPuppet, -front);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGoalDoorOpenWait);
}
void GoalDoor::exeOpenWait() {
    if (al::isFirstStep(this)) al::startAction(this, "OpenWait");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGoalDoorClose);
}
void GoalDoor::exeClose() {
    if (al::isFirstStep(this)) al::startAction(this, "Close");
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGoalDoorEnd);
        rc::hidePuppet(mPuppet);
    }
}
void GoalDoor::exeEnd() {}
void GoalDoor::exeCancel() {
    if (al::isFirstStep(this)) al::startAction(this, "CloseWait");
    if (al::isStep(this, 60)) al::setNerve(this, &NrvGoalDoorWait);
}

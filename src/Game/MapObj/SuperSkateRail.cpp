#include "MapObj/SuperSkateRail.hpp"
#include "MapObj/SuperSkateShoes.hpp"
#include "Library/ActorUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
SuperSkateRail::SuperSkateRail(const char* name) : al::LiveActor(name) {}
void SuperSkateRail::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "SuperSkateRail", nullptr);
    if (al::calcLinkChildNum(info, "Next")) {
        auto* next = new SuperSkateRail("SuperSkateRail");
        next->mPrevious = this;
        mNext = next;
        al::initLinksActor(next, info, "Next", 0);
    }
    al::invalidateClipping(this);
    makeActorAppeared();
}
void SuperSkateRail::attackSensor(al::HitSensor*, al::HitSensor*) {}
bool SuperSkateRail::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (rc::isMsgSkateShoesAttack(msg)) {
        auto* shoes = static_cast<SuperSkateShoes*>(al::getSensorHost(sender));
        if (shoes) {
            shoes->startGrind(this);
            return true;
        }
        return false;
    }
    return false;
}
void SuperSkateRail::control() {
    if (mNext) {
        al::getTrans(mNext);
        al::getTrans(this);
    }
}
sead::Vector3f SuperSkateRail::getDirection() {
    if (mNext) {
        sead::Vector3f direction = al::getTrans(mNext) - al::getTrans(this);
        direction.normalize();
        return direction;
    }
    return sead::Vector3f::zero;
}
sead::Vector3f SuperSkateRail::getReverseDirection() {
    if (mPrevious) return -mPrevious->getDirection();
    return sead::Vector3f::zero;
}
SuperSkateRail* SuperSkateRail::getNextRail() { return mNext; }
SuperSkateRail* SuperSkateRail::getPreviousRail() { return mPrevious; }
sead::Vector3f SuperSkateRail::lerp(float rate) {
    if (mNext) return al::getTrans(this) + rate * (al::getTrans(mNext) - al::getTrans(this));
    return sead::Vector3f::zero;
}
float SuperSkateRail::getDistance() {
    if (mDistance == 0.0f && mNext)
        mDistance = (al::getTrans(this) - al::getTrans(mNext)).length();
    return mDistance;
}
float SuperSkateRail::getReverseDistance() {
    if (mPrevious) return mPrevious->getDistance();
    return 0.0f;
}

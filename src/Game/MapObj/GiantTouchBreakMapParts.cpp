#include "MapObj/GiantTouchBreakMapParts.hpp"
#include "Library/ActorUtil.hpp"

GiantTouchBreakMapParts::GiantTouchBreakMapParts(const char* pName) : al::LiveActor(pName) {}

void GiantTouchBreakMapParts::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    makeActorAppeared();
}

void GiantTouchBreakMapParts::kill() {
    al::LiveActor::kill();
    al::tryOnSwitchDeadOn(this);
}

bool GiantTouchBreakMapParts::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                       al::HitSensor* pSelf) {
    if (al::isMsgPlayerGiantTouch(pMsg)) {
        kill();
        return true;
    }
    return false;
}

GiantTouchBreakMapParts::~GiantTouchBreakMapParts() {}

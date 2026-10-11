#include "MapObj/ZigzagBuildingCover.hpp"
#include "Library/ActorUtil.hpp"

ZigzagBuildingCover::ZigzagBuildingCover(const char* pName) : al::LiveActor(pName) {
}

void ZigzagBuildingCover::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    makeActorAppeared();
}

bool ZigzagBuildingCover::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    return al::isMsgScreenPointInvalidCollisionParts(pMsg);
}

ZigzagBuildingCover::~ZigzagBuildingCover() {
}

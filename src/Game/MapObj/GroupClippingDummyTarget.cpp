#include "MapObj/GroupClippingDummyTarget.hpp"
#include "Library/ActorUtil.hpp"

GroupClippingDummyTarget::GroupClippingDummyTarget(const char* pName) : al::LiveActor(pName) {}

void GroupClippingDummyTarget::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 64);
    al::setClippingInfo(this, al::getScale(this).x * 100.0f * 0.5f, nullptr);
    makeActorAppeared();
}

GroupClippingDummyTarget::~GroupClippingDummyTarget() {}

#include "MapObj/Fish.hpp"
#include "Library/ActorUtil.hpp"

Fish::Fish(const char* pName) : al::LiveActor(pName) {}

void Fish::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    makeActorAppeared();
    al::startAction(this, "Wait");
}

Fish::~Fish() {}

#include "MapObj/EchoBlockMapParts.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Library/ActorUtil.hpp"

EchoBlockMapParts::EchoBlockMapParts(const char* pName) : al::LiveActor(pName) {}

void EchoBlockMapParts::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    rc::initEchoEmitterHolder(this, rInfo);
    al::trySyncStageSwitchAppear(this);
    al::tryListenStageSwitchKill(this);
}

void EchoBlockMapParts::exeWait() {}

EchoBlockMapParts::~EchoBlockMapParts() {}

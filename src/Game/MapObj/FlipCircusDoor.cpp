#include "MapObj/FlipCircusDoor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(FlipCircusDoorA, Wait);
    NERVES_MAKE_NOSTRUCT(FlipCircusDoorA, Wait)
}

FlipCircusDoorA::FlipCircusDoorA(const char* pName) : al::LiveActor(pName) {}

void FlipCircusDoorA::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvFlipCircusDoorAWait, 0);
    makeActorAppeared();
}

void FlipCircusDoorA::kill() {
    al::LiveActor::kill();
    al::startHitReactionDisappear(this);
}

void FlipCircusDoorA::exeWait() {
    if (al::isOnStageSwitch(this, "SwitchKill"))
        kill();
}

FlipCircusDoorA::~FlipCircusDoorA() {}

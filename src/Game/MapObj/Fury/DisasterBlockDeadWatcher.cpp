#include "MapObj/Fury/DisasterBlockDeadWatcher.hpp"
#include "MapObj/Fury/BlockHardLaserOnly.hpp"
#include "Library/ActorUtil.hpp"
DisasterBlockDeadWatcher::DisasterBlockDeadWatcher(const char* pName) : AllDeadWatcher(pName) {}
DisasterBlockDeadWatcher::~DisasterBlockDeadWatcher() {}
void DisasterBlockDeadWatcher::init(const al::ActorInitInfo& rInfo) { AllDeadWatcher::init(rInfo); }
void DisasterBlockDeadWatcher::exeWatch() {
    for (int i = 0; i < mActorCount; ++i) {
        auto* block = static_cast<BlockHardLaserOnly*>(mActors[i]);
        if ((al::isAlive(block) || al::isDeadAlive(mActors[i])) && !block->isBreaking(true))
            return;
    }
    setWaitNerve();
}
void DisasterBlockDeadWatcher::exeWatchPartial() {
    for (int i = 0; i < mActorCount; ++i) {
        auto* block = static_cast<BlockHardLaserOnly*>(mActors[i]);
        if (al::isDead(block) || block->isBreaking(true)) {
            al::tryOnStageSwitch(this, "SwitchPartialDeadOn");
            setWatchNerve();
        }
    }
}

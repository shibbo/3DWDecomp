#include "MapObj/SwitchAnd.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Thread/Functor.hpp"

SwitchAnd::SwitchAnd(const char* pName) : al::LiveActor(pName) {
}

void SwitchAnd::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::tryGetArg(&mAllowInstantSwitch, rInfo, "AllowInstantSwitch");

    if (al::listenStageSwitchOn(this, "InputSwitchA", al::Functor(this, &SwitchAnd::notifyInputSwitchOn))) {
        ++mRemainingSwitches;
    }
    if (al::listenStageSwitchOn(this, "InputSwitchB", al::Functor(this, &SwitchAnd::notifyInputSwitchOn))) {
        ++mRemainingSwitches;
    }
    if (al::listenStageSwitchOn(this, "InputSwitchC", al::Functor(this, &SwitchAnd::notifyInputSwitchOn))) {
        ++mRemainingSwitches;
    }
    if (al::listenStageSwitchOn(this, "InputSwitchD", al::Functor(this, &SwitchAnd::notifyInputSwitchOn))) {
        ++mRemainingSwitches;
    }
    makeActorDead();
}

// Only connected inputs count toward the AND condition.
void SwitchAnd::notifyInputSwitchOn() {
    if (--mRemainingSwitches == 0) {
        if (mAllowInstantSwitch) {
            al::tryOnStageSwitchInstant(this, "OutputSwitchOn");
        } else {
            al::tryOnStageSwitch(this, "OutputSwitchOn");
        }
    }
}

SwitchAnd::~SwitchAnd() {
}

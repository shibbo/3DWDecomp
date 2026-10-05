#include "MapObj/TimerStageSwitch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

TimerStageSwitch::TimerStageSwitch(const char* pName) : al::LiveActor(pName) {
}

void TimerStageSwitch::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::tryGetArg(&mTimer, rInfo, "Timer");
    al::listenStageSwitchOnStart(this, al::Functor(this, &TimerStageSwitch::start));
    makeActorDead();
}

void TimerStageSwitch::start() {
    mFrame = 0;
    appear();
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        al::tryOffStageSwitch(this, "SwitchTimerOn");
    }
}

void TimerStageSwitch::control() {
    if (++mFrame >= mTimer) {
        al::tryOnStageSwitch(this, "SwitchTimerOn");
        kill();
    }
}

TimerStageSwitch::~TimerStageSwitch() {
}

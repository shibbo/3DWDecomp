#include "MapObj/GoalItemWatcher.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

namespace {
    NERVE_DECL(GoalItemWatcher, Watch);
    NERVES_MAKE_NOSTRUCT(GoalItemWatcher, Watch)
}

GoalItemWatcher::GoalItemWatcher(const char* pName) : al::LiveActor(pName) {}
GoalItemWatcher::~GoalItemWatcher() {}

void GoalItemWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvGoalItemWatcherWatch, 0);
    al::tryGetArg(&mNumGoalItemsToTrigger, rInfo, "NumGoalItemsToTrigger");
    al::tryGetArg(&mIsDisasterTrigger, rInfo, "IsDisasterTrigger");
    al::tryGetArg(&mNumWaitFrames, rInfo, "NumWaitFrames");
    mGoalItemsCollected = SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(this));
    makeActorAppeared();
}

void GoalItemWatcher::exeWatch() {
    int collected = SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(this));
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (collected < mNumGoalItemsToTrigger) {
        if (collected > mGoalItemsCollected) {
            mGoalItemsCollected = collected;
            if (controller && controller->isDisasterMode())
                controller->endImmediate();
        }
        return;
    }
    mGoalItemsCollected = collected;
    al::tryOnStageSwitch(this, "SwitchGoalItemOn");
    if (mIsDisasterTrigger) {
        if (controller->isDisasterMode()) {
            controller->pause(false);
        } else {
            controller->setGoalItemDisasterTrigger();
            controller->beginAndNeverEndDebug();
        }
    }
    kill();
}

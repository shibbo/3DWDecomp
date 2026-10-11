#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class GoalItemWatcher : public al::LiveActor {
public:
    GoalItemWatcher(const char* pName);
    ~GoalItemWatcher() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWatch();

private:
    int mNumGoalItemsToTrigger = 0;
    bool mIsDisasterTrigger;
    int mNumWaitFrames;
    int mGoalItemsCollected = 0;
};

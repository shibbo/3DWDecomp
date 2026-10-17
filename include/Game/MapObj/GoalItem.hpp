#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GoalItem : public al::LiveActor {
public:
    int getIslandId() const { return mIslandId; }
    int getScenarioId() const { return mScenarioId; }
private:
    u8 mUnknown144[0x24];
    int mIslandId;
    int mScenarioId;
};

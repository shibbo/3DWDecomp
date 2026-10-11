#include "MapObj/GoalObjHolder.hpp"
#include "MapObj/IGoalObj.hpp"

GoalObjHolder::GoalObjHolder(int capacity) {
    mGoals.allocBuffer(capacity, nullptr);
}

void GoalObjHolder::pushBack(const IGoalObj* pGoal) {
    mGoals.pushBack(pGoal);
}

bool GoalObjHolder::isGoal() const {
    for (int i = 0; i < mGoals.size(); ++i)
        if (mGoals[i]->isGoal())
            return true;
    return false;
}

bool GoalObjHolder::isEndGoalDemo() const {
    for (int i = 0; i < mGoals.size(); ++i)
        if (mGoals[i]->isGoal() && mGoals[i]->isEndGoalDemo())
            return true;
    return false;
}

bool GoalObjHolder::isUseResult() const {
    for (int i = 0; i < mGoals.size(); ++i)
        if (mGoals[i]->isGoal() && !mGoals[i]->isUseResult())
            return false;
    return true;
}

bool GoalObjHolder::isRetireGoal() const {
    for (int i = 0; i < mGoals.size(); ++i)
        if (mGoals[i]->isGoal())
            return mGoals[i]->isRetireGoal();
    return false;
}

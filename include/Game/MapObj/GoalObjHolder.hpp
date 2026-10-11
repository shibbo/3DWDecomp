#pragma once

#include <container/seadPtrArray.h>

class IGoalObj;

class GoalObjHolder {
public:
    explicit GoalObjHolder(int capacity);
    void pushBack(const IGoalObj* pGoal);
    bool isGoal() const;
    bool isEndGoalDemo() const;
    bool isUseResult() const;
    bool isRetireGoal() const;

private:
    sead::PtrArray<const IGoalObj> mGoals;
};

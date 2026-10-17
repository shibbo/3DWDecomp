#pragma once

class IGoalObj {
public:
    virtual bool isGoal() const = 0;
    virtual bool isEndGoalDemo() const = 0;
    virtual bool isUseResult() const { return true; }
    virtual bool isRetireGoal() const { return false; }
};

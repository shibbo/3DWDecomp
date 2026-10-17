#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalObj.hpp"
class GreenStar;
class GoalObjStateGoalDemo;
class GoalObjStateGoalDemoParam;
class GateKeeperChecker : public al::LiveActor, public IGoalObj {
public:
    explicit GateKeeperChecker(const char* name);
    void init(const al::ActorInitInfo& info) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isGoal() const override;
    bool isEndGoalDemo() const override;
    void exeChecking();
    void exeGoalDemo();
    void exeEnd();
private:
    GreenStar* mStar = nullptr;
    GoalObjStateGoalDemo* mGoalDemo = nullptr;
    GoalObjStateGoalDemoParam* mGoalParam = nullptr;
};
static_assert(sizeof(GateKeeperChecker) == 0x168);

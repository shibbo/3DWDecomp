#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalObj.hpp"
class IUsePlayerPuppet;
class GoalDoor : public al::LiveActor, public IGoalObj {
public:
    explicit GoalDoor(const char*);
    ~GoalDoor() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isGoal() const override;
    bool isEndGoalDemo() const override;
    bool isUseResult() const override { return false; }
    bool isRetireGoal() const override { return true; }
    void exeWait();
    void exeOpen();
    void exeOpenWait();
    void exeClose();
    void exeEnd();
    void exeCancel();
private:
    IUsePlayerPuppet* mPuppet = nullptr;
    int mTouchTimer = 0;
};
static_assert(sizeof(GoalDoor) == 0x160);

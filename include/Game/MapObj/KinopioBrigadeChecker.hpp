#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalObj.hpp"
#include <container/seadPtrArray.h>
#include <prim/seadBitFlag.h>
class GreenStar;
class DemoStartPosition;
class GoalObjStateGoalDemo;
class GoalObjStateGoalDemoParam;
class KinopioBrigadeChecker : public al::LiveActor, public IGoalObj {
public:
    explicit KinopioBrigadeChecker(const char*);
    ~KinopioBrigadeChecker() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isGoal() const override;
    bool isEndGoalDemo() const override;
    DemoStartPosition* findNearestDemoStartPosition() const;
    void exeChecking();
    void exeGoalDemo();
    void exeEnd();
private:
    sead::FixedPtrArray<GreenStar, 5> mStars;
    sead::PtrArray<DemoStartPosition> mPositions;
    sead::BitFlag32 mAcquired;
    al::LiveActor* mDemoStar = nullptr;
    al::LiveActor* mDemoStarEmpty = nullptr;
    al::LiveActor* mGoalPlayer = nullptr;
    GoalObjStateGoalDemo* mGoalDemo = nullptr;
    GoalObjStateGoalDemoParam* mGoalParam = nullptr;
};
static_assert(sizeof(KinopioBrigadeChecker) == 0x1c8);

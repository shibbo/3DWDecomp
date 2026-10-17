#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include "MapObj/IGoalObj.hpp"
#include <container/seadPtrArray.h>
class GreenStar;
class MysteryBox;
class GoalObjStateGoalDemo;
class GoalObjStateGoalDemoParam;
struct MysteryHouseStarInfo {
    explicit MysteryHouseStarInfo(const GreenStar* value) : star(value), acquired(false) {}
    const GreenStar* star;
    bool acquired;
};
class MysteryHouseChecker : public al::LiveActor, public IGoalObj, public al::ISceneObj {
public:
    explicit MysteryHouseChecker(const char*);
    ~MysteryHouseChecker() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isGoal() const override;
    bool isEndGoalDemo() const override;
    bool isMiss() const;
    void registerGreenStar(const GreenStar*);
    void registerMysteryBox(al::LiveActor*);
    int tryCalcLastAcquirerUserId() const;
    void exeChecking();
    void exeGoalDemo();
    void exeEnd();
    void exeMiss();
private:
    sead::FixedPtrArray<MysteryHouseStarInfo, 50> mStars;
    sead::FixedPtrArray<MysteryBox, 100> mBoxes;
    int mRequiredStars = 1;
    int mAcquiredCount = 0;
    GoalObjStateGoalDemo* mGoalDemo = nullptr;
    al::HitSensor* mLastAcquirer = nullptr;
    GoalObjStateGoalDemoParam* mGoalParam = nullptr;
};
static_assert(sizeof(MysteryHouseChecker) == 0x648);
namespace MysteryHouseCheckerFunction {
void tryRegisterGreenStar(const GreenStar*);
void tryRegisterMysteryBox(al::LiveActor*);
}

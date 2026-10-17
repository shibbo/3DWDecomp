#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalObj.hpp"
class BindPuppeteerGroup;
class DokanBindPuppeteer;
class DokanGuideBalloon;
class DokanWorldWarp : public al::LiveActor, public IGoalObj {
public:
    explicit DokanWorldWarp(const char*);
    ~DokanWorldWarp() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isGoal() const override;
    bool isEndGoalDemo() const override;
    bool isUseResult() const override { return false; }
    void setLayout(const al::HitSensor*);
    DokanBindPuppeteer* getPuppeteer(const al::HitSensor*) const;
    void updatePuppeteer();
    bool isEnableStartWorldWarp() const;
    void exeWait();
    void exePlayerIn();
    void exeWaitStartWorldWarp();
private:
    BindPuppeteerGroup* mPuppeteers = nullptr;
    bool mBindAll = false;
    DokanGuideBalloon** mBalloons = nullptr;
    bool mHipDrop = false;
    al::HitSensor* mBinderSensor = nullptr;
};
static_assert(sizeof(DokanWorldWarp) == 0x178);

#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CourseSelectMiniature;
class CourseSelectSensor;
class CourseSelectLockController;
class CourseSelectDirector;
class CourseSelectLock : public al::LiveActor {
public:
    CourseSelectLock(const char* pName, CourseSelectMiniature* pMiniature, int requiredStars);
    ~CourseSelectLock() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    void startAppear();
    void startUnlock();
    void startShow();
    void startDisable();
    bool tryDecide(const CourseSelectDirector* pDirector);
    bool isUnlock() const;
    void exeAppear();
    void exeWait();
    void exeUnlock();
    void exeDisappear();
    void exeEnd();
private:
    int mRequiredStars;
    CourseSelectSensor* mpSensor = nullptr;
    CourseSelectSensor* mpTouchSensor = nullptr;
    CourseSelectMiniature* mpMiniature;
    CourseSelectLockController* mpController = nullptr;
};
static_assert(sizeof(CourseSelectLock) == 0x168);

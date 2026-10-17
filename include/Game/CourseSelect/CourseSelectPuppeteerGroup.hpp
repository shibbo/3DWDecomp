#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectPuppeteer;
class CourseSelectScene;
class ICourseSelectActorController;

/**
 * @brief Actor owning the course-select puppeteers, one per control user.
 */
class CourseSelectPuppeteerGroup : public al::LiveActor {
public:
    ~CourseSelectPuppeteerGroup() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void control() override;
    void startEventGateKeeperDemo(ICourseSelectActorController*);
    void startWorldStartDemo();
    void startWorldStartDemoW8();
    bool isPlayDemo() const;
    bool isPlayEntryDemo() const;
    void exeWait();
    void exeSave();
    void startWaitDemo(int userId);
    void startDokanDemo(ICourseSelectActorController* pController);
    void startUnLockDemo(ICourseSelectActorController* pController);
    void startWorldWarpDokanDemo(ICourseSelectActorController* pController);
    bool isDemoEnd() const;
    void startRouteDokan(ICourseSelectActorController* pController);
    int getPuppeteerCount() const { return mPuppeteerNum; }
    explicit CourseSelectPuppeteerGroup(CourseSelectScene* pScene);

    void startExitDemo(ICourseSelectActorController* pController);
    void startEnterDemo(ICourseSelectActorController* pController);
    void startLockAppearDemo(ICourseSelectActorController* pController);
    void startOpenGateKeeperDemo(ICourseSelectActorController* pController);
    void startWarpToCourse(ICourseSelectActorController* pController);
    void startHidePlayerDemo();
    void startOpenRoadDemo(ICourseSelectActorController* pController);
    void endDemo();
    bool isPlayDemo(s32 userId) const;
    bool isPlayDemoAll() const;
    bool isPlayDemoAny() const;
    void reviveUser(s32 userId, ICourseSelectActorController* pController, bool isAppearDemo);
    void enterUser(s32 userId);
    void leaveUser(s32 userId);
    bool isPlayEntryDemo(s32 userId) const;
    bool isPlayLeaveDemo(s32 userId) const;
    void startRocketDemo(ICourseSelectActorController* pController);
    void startRocketBreakDemo(ICourseSelectActorController* pController);
    void startSaveDataWriteInDemo(ICourseSelectActorController* pController);
    bool isEndSaveDataWriteInDemo(ICourseSelectActorController* pController);

    /** @brief Gets the puppeteer of a control user. @param userId User id. @return The puppeteer. */
    CourseSelectPuppeteer* getPuppeteer(s32 userId) const { return mPuppeteers[userId]; }
    /** @brief Gets the number of puppeteers. @return The puppeteer count. */
    s32 getPuppeteerNum() const { return mPuppeteerNum; }

private:
    CourseSelectScene* mScene;
    ICourseSelectActorController* mController = nullptr;
    s32 mPuppeteerNum = 0;  // 0x158
    u8 _15c[0x160 - 0x15c];
    CourseSelectPuppeteer** mPuppeteers = nullptr;  // 0x160
};

static_assert(sizeof(CourseSelectPuppeteerGroup) == 0x168);

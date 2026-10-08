#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectPuppeteer;
class CourseSelectScene;
class ICourseSelectActorController;

/**
 * @brief Actor owning the course-select puppeteers, one per control user.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectPuppeteerGroup : public al::LiveActor {
public:
    explicit CourseSelectPuppeteerGroup(CourseSelectScene* pScene);

    void startEnterDemo(ICourseSelectActorController* pController);
    void startLockAppearDemo(ICourseSelectActorController* pController);
    void startOpenGateKeeperDemo(ICourseSelectActorController* pController);
    void startWarpToCourse(ICourseSelectActorController* pController);
    void startHidePlayerDemo();
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
    u8 _144[0x158 - 0x144];  // starts in the tail padding of al::LiveActor
    s32 mPuppeteerNum;  // 0x158
    u8 _15c[0x160 - 0x15c];
    CourseSelectPuppeteer** mPuppeteers;  // 0x160
};

static_assert(sizeof(CourseSelectPuppeteerGroup) == 0x168);

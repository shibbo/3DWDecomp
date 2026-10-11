#pragma once

#include <basis/seadTypes.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class ActorInitInfo;
class BlockRailRider;
class HitSensor;
class LiveActor;
class Nerve;
class SensorMsg;
}  // namespace al

class CourseSelectPuppeteerGroup;
class ICourseSelectActorController;
class PlayerActor;
class RouteDokanInOutEffect;

/**
 * @brief Drives one control user's player through the scripted moves of the course-select map:
 * entering and leaving courses, dokans and rockets, route dokans, unlock demos and the
 * appearance of users that join or revive.
 */
class CourseSelectPuppeteer : public BindPuppeteer {
public:
    CourseSelectPuppeteer(CourseSelectPuppeteerGroup* pGroup, const al::ActorInitInfo& rInfo,
                          s32 userId);
    ~CourseSelectPuppeteer() override;

    void setActor(PlayerActor* pActor);
    void startDemoWithNextNerve(ICourseSelectActorController* pController,
                                const al::Nerve* pNextNerve);
    void startLandingWithNextNerve(const al::Nerve* pNextNerve);
    void startEnterDemo(ICourseSelectActorController* pController);
    void startExitDemo(ICourseSelectActorController* pController);
    void startEventGateKeeperDemo(ICourseSelectActorController* pController);
    void startOpenRoadDemo(ICourseSelectActorController* pController);
    void startUnLockDemo(ICourseSelectActorController* pController);
    void startLockAppearDemo(ICourseSelectActorController* pController);
    void startOpenGateKeeperDemo(ICourseSelectActorController* pController);
    void startDokanDemo(ICourseSelectActorController* pController);
    void startWorldWarpDokanDemo(ICourseSelectActorController* pController);
    void startRocketDemo(ICourseSelectActorController* pController);
    void startRocketBreakDemo(ICourseSelectActorController* pController);
    void startRouteDokan(ICourseSelectActorController* pController);
    void startWarpToCourse(ICourseSelectActorController* pController);
    void startWorldStartDemo();
    void startWorldStartDemoW8();
    void startHidePlayerDemo();
    void startWaitDemo();
    void endDemo();
    void update();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver);
    bool isDemoEnd() const;
    void activateUser(const al::Nerve* pNerve, bool isHidePlayer, const sead::Vector3f& rTrans);
    void cancelLeave();
    void reviveUser(ICourseSelectActorController* pController, bool isLineUpBehindLeader);
    void enterUser();
    void leaveUser();
    bool isPlayEntryDemo() const;
    bool isPlayLeaveDemo() const;
    void startBindWarpActor(const al::LiveActor* pWarpStart, const al::LiveActor* pWarpEnd);
    void startDokanWarp();
    void startDokanEnd();
    bool isDokanDemoWarpWait() const;
    bool isDokanDemoEndWait() const;
    void startRocketDemoWarp();
    void startRocketDemoOut();
    void startRocketDemoEnd();
    bool isRocketDemoWarpWait() const;
    bool isRocketDemoEndWait() const;

    void exeWait() {}
    void exeBindWait();
    void exeLanding();
    void exeEnterPrepare();
    void setupJumpPrepare(const al::LiveActor* pTarget);
    void exeEnter();
    void setupJumpEnter(const al::LiveActor* pTarget, f32 height);
    void updateJumpParamEnter();
    void exeEnterGateKeeper();
    void exeEnterHide();
    void exeEventGateKeeper();
    void exePrepareExit();
    void exeExit();
    void setupJumpExit(const al::LiveActor* pTarget, f32 height);
    sead::Vector3f updateJumpParamExit();
    void exeExitKinopioBrigade();
    void exeOpenRoad();
    void exeUnLock();
    void exeLockAppear();
    void exeOpenGateKeeper();
    void exeDokanPrepareIn();
    void exeDokanIn();
    void exeDokanWarpWait();
    void exeDokanPrepareOut();
    void exeDokanOut();
    void exeDokanEndWait();
    void exeWorldWarpDokanStart();
    void exeWorldWarpDokanMove();
    void exeWorldWarpDokanOut();
    void exeRocketBreakDemo();
    void exeRocketPrepareIn();
    void exeRocketIn();
    void exeRocketLaunchWait();
    void exeRocketPrepareOut();
    void exeRocketOut();
    void exeRocketEndWait();
    void exeRouteDokanMoveStart();
    void exeRouteDokanMove();
    void exeRouteDokanMoveEnd();
    void exeWarpToCourse();
    void exeWorldStartDemo();
    void exeWorldStartDemoW8();
    void exeHidePlayerDemo();
    void exeWaitDemo();
    void exeReviveUserDelay();
    void exeReviveUser();
    void exeEnterUser();
    void exeLeaveUser();
    void exeBindEndLanding();
    void exeBindEnd();

private:
    CourseSelectPuppeteerGroup* mGroup;  // 0x20
    s32 mUserId;  // 0x28
    PlayerActor* mActor = nullptr;  // 0x30
    ICourseSelectActorController* mController = nullptr;  // 0x38
    al::BlockRailRider* mRailRider = nullptr;  // 0x40
    RouteDokanInOutEffect* mDokanEffect = nullptr;  // 0x48
    sead::Vector3f mRouteStartTrans;  // 0x50
    sead::Quatf mRouteStartQuat;  // 0x5c
    sead::Vector3f mRouteEndTrans;  // 0x6c
    sead::Quatf mRouteEndQuat;  // 0x78
    const al::Nerve* mNextNerve = nullptr;  // 0x88
    s32 _90 = 0;
    f32 mJumpEndY = 0.0f;  // 0x94
    s32 mReviveDelay = 0;  // 0x98
    const al::LiveActor* mWarpStartActor = nullptr;  // 0xa0
    const al::LiveActor* mWarpEndActor = nullptr;  // 0xa8
    bool mIsRequestLeave = false;  // 0xb0
};

static_assert(sizeof(CourseSelectPuppeteer) == 0xb8);

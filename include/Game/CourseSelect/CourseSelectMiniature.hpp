#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "CourseSelect/CourseSelectActorInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class RumbleCalculator;
}  // namespace al

class CourseSelectDirector;
class CourseSelectFairy;
class CourseSelectFlag;
class CourseSelectLock;
class CourseSelectNode;
class CourseSelectPuppeteerGroup;
class CourseSelectRouteDokan;
class CourseSelectSensor;
class CourseSelectWorldWarpDokan;
class DemoWorldClear;
class ICourseSelectActorController;
class MiniatureController;
class StageDatabaseInfo;

/**
 * @brief Course miniature of the course-select map: the small model of a course the players walk
 * onto to enter it. It owns the objects shown around the course (clear flag, green star lock,
 * fairy of Bowser's castles, route and world warp dokans) and plays the open, clear and enter
 * demos.
 */
class CourseSelectMiniature : public al::LiveActor {
public:
    /** @brief Flags read from the stage database and the placement of the miniature. */
    enum Flag : u32 {
        cFlag_NoRotate = 1 << 0,
        cFlag_NoEnter = 1 << 1,
        cFlag_HideModel = 1 << 2,
        cFlag_AlwaysOpen = 1 << 3,
        cFlag_AlwaysShow = 1 << 4,
        cFlag_ClearHide = 1 << 5,
        cFlag_House = 1 << 6,
        cFlag_KoopaCastle = 1 << 7,
        cFlag_BackInverse = 1 << 8,
        cFlag_HasAppearAction = 1 << 9,
    };

    explicit CourseSelectMiniature(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    StageDatabaseInfo* getStageInfo() const;
    s32 getCourseId() const;
    s32 getWorldId() const;
    void initAfterPlacement() override;
    s32 getStageId() const;
    bool isGreenStarLock() const;
    bool isCourseOpen() const;
    void control() override;
    void appear() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void startEnter();
    void startClearDemo();
    bool isGateKeeper() const;
    void startOpenRouteDokan();
    bool isNextCourse(s32 courseId) const;
    CourseSelectNode* tryFindNextNodeEnd() const;
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup);
    void endPuppetDemo();
    bool isNeedClearDemo() const;
    bool isPlayingClearDemo() const;
    bool isKinopioHouse() const;
    s32 tryFindNextNode(ICourseSelectActorController** pControllers, s32 maxNum);
    void startAppear();
    void startGreenStarLockAppear();
    void startClearFlagDemo();
    bool isAppeared() const;
    void startWorldClearDemo();
    bool isEndWorldClearDemo() const;
    void returnCastleFairyPrincess();
    void changeCurrentWorldId(s32 worldId);
    bool tryDecide(const CourseSelectDirector* pDirector);

    void exeClear();
    void exeClearGateKeeper();
    void exeClearKinopioBrigade();
    void exeHide();
    void exeAppear();
    void exeWait();
    void updatePosture();
    void exeEnter();
    void exeEnterEnd();
    void exeKinopioBrigadeWaitEnter();
    void exeKinopioBrigadeEnter();
    void exeClearHide();
    void exeLockWait();

    /** @brief Gets the fairy shown next to this miniature. @return The fairy, or nullptr. */
    CourseSelectFairy* getFairy() const { return mFairy; }
    /** @brief Gets the clear flag of the miniature. @return The flag, or nullptr. */
    CourseSelectFlag* getFlag() const { return mFlag; }
    /** @brief Gets the route dokan opened by clearing the course. @return The dokan, or nullptr. */
    CourseSelectRouteDokan* getRouteDokan() const { return mRouteDokan; }
    /** @brief Gets the world clear demo of the miniature. @return The demo, or nullptr. */
    DemoWorldClear* getDemoWorldClear() const { return mDemoWorldClear; }
    /** @brief Gets the branch node placed on the miniature. @return The node. */
    CourseSelectNode* getNode() const { return mNode; }
    /** @brief Gets the controller the puppeteers use. @return The controller. */
    MiniatureController* getController() const { return mController; }
    /** @brief Gets the number of courses opened by clearing this one. @return The count. */
    s32 getNextCourseNum() const { return mNextCourseNum; }
    /** @brief Gets the ids of the courses opened by clearing this one. @return The id array. */
    const s32* getNextCourseIds() const { return mNextCourseIds; }
    /** @brief Gets whether a dokan is linked to the miniature. @return true if linked. */
    bool hasDokanLink() const { return mHasDokanLink; }
    /** @brief Gets the position of the linked dokan. @return The dokan position. */
    const sead::Vector3f& getDokanTrans() const { return mDokanTrans; }

private:
    inline void showCloseModel();
    inline void hideCloseModel();
    inline void setNerveAfterClear();
    inline void calcGateKeeperMoveDir(f32* pDirX, f32* pDirZ) const;

    u32 mFlags = 0;
    f32 mRotateDegree = 0.0f;
    al::LiveActor* mCloseModel = nullptr;
    CourseSelectFairy* mFairy = nullptr;
    CourseSelectFlag* mFlag = nullptr;
    CourseSelectLock* mLock = nullptr;
    CourseSelectRouteDokan* mRouteDokan = nullptr;
    void* _178 = nullptr;
    CourseSelectWorldWarpDokan* mWorldWarpDokan = nullptr;
    DemoWorldClear* mDemoWorldClear = nullptr;
    s32 mNextCourseNum = 0;
    s32* mNextCourseIds = nullptr;
    CourseSelectDirector* mDirector = nullptr;
    CourseSelectNode* mNode = nullptr;
    CourseSelectSensor* mSensor = nullptr;
    CourseSelectSensor* mDrcSensor = nullptr;
    CourseSelectActorInfo* mActorInfo = nullptr;
    CourseSelectPuppeteerGroup* mPuppeteerGroup = nullptr;
    MiniatureController* mController = nullptr;
    al::RumbleCalculator* mRumble;
    sead::Vector3f mAppearTrans;
    bool mHasDokanLink = false;
    bool _1ed = false;
    sead::Vector3f mDokanTrans = {0.0f, 0.0f, 0.0f};
    bool mHasNextNodeEnd = false;
    sead::Vector3f mNextNodeEndTrans = {0.0f, 0.0f, 0.0f};
    bool mIsNearSe = false;
    s32 mColorFrame = -1;
    s32 mGateKeeperMoveDir = -1;
};

static_assert(sizeof(CourseSelectMiniature) == 0x218);

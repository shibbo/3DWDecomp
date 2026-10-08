#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalObj.hpp"

namespace al {
class CameraInfo;
class SimpleLayoutAppearWait;
template <class T>
class DeriveActorGroup;
}  // namespace al

class ActorStateDemoCamera;
class BindPuppeteerGroup;
class FairyPrincess;
class GoalPoleBindPuppeteer;
class GoalPoleFlag;
class GoalPoleStateRunaway;
class RosettaNpc;
class StageTimer;

/**
 * @brief The goal pole at the end of a course: players jump on it, slide down and the course is
 * cleared.
 */
class GoalPole : public al::LiveActor, public IGoalObj {
public:
    /// Kind of goal pole, decided by the placement object name.
    enum class Type : s32 {
        Normal = 0,
        Super = 1,
        Last = 2,
    };

    using FairyGroup = al::DeriveActorGroup<FairyPrincess>;

    explicit GoalPole(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void tryCreateNpc(const al::ActorInitInfo& rInfo);
    bool isSuper() const;
    void startDemoFairyFocus();
    bool isLast() const;
    void startDemoEndingPrev();
    void initAfterPlacement() override;
    void control() override;
    GoalPoleBindPuppeteer* getPuppeteer(s32 index) const;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    const sead::Vector3f& getPoleTrans() const;
    void fixCamera(const sead::Vector3f& rLookAt);
    bool isGoalDemoPlaying() const;

    /**
     * Sets the stage timer stopped by the goal.
     * @param pStageTimer The stage timer.
     */
    void setStageTimer(StageTimer* pStageTimer) { mStageTimer = pStageTimer; }

    /**
     * Prevents the players from catching the pole, e.g. once the time is up.
     */
    void invalidateBind() { mIsInvalidBind = true; }
    bool isSuperWithFairyBottle() const;
    bool isCatchSuccess(s32 index) const;
    f32 calcCatchHeightRate(s32 index) const;
    s32 calcHighestUserId() const;
    s32 calcHeightOrder(s32 index) const;
    s32 getHeightOrder(s32 index) const;
    void prepareDemoW7KoopaCastleClear();
    void startDemoW7KoopaCastleClear();
    void startGoalPoseNpcAction();
    bool tryReleaseFairy();
    void startLerpFlag();
    void updateBindSensor();
    void tryKillFlag();
    void tryAppearFlagBottom();
    GoalPoleBindPuppeteer* findUnderHeightPuppeteer(s32 heightOrder) const;
    GoalPoleBindPuppeteer* findFirstCatchPuppeteer() const;
    void addDemoActorNpc();
    void exeWait();
    void exeRunaway();
    void exeFairyFocusDemo();
    void exeEndingPrevDemoRequest();
    void exeEndingPrevDemo();
    void exeWaitGoalDemo();
    void exeGoalDemoCatch();
    void exeGoalDemoFall();
    void exeGoalDemoJump();
    void exeGoalDemoFireworks();

    bool isGoal() const override { return mIsGoal; }
    bool isEndGoalDemo() const override { return mIsEndGoalDemo; }

    bool isRunaway() const { return mRunawayState != nullptr; }
    const sead::Matrix34f& getRunawayBaseMtx() const { return mBaseMtx; }

    /**
     * @brief Height of the pole top, where the players reach the top of the pole.
     * @return The world Y coordinate of the pole top.
     */
    f32 getPoleTopHeight() const { return mPoleTopTrans.y; }

    /**
     * @brief Whether the players show their fur when jumping off the pole.
     * @return True if the fur is enabled.
     */
    bool isEnableFur() const { return mIsEnableFur; }

private:
    bool mIsGoal = false;  // 0x150
    bool mIsEndGoalDemo = false;  // 0x151
    sead::Vector3f* mBindPos = nullptr;  // 0x158
    BindPuppeteerGroup* mPuppeteerGroup = nullptr;  // 0x160
    GoalPoleFlag* mFlag = nullptr;  // 0x168
    FairyGroup* mFairyGroup = nullptr;  // 0x170
    RosettaNpc* mRosetta = nullptr;  // 0x178
    s32 mCatchNum = 0;  // 0x180
    Type mType = Type::Normal;  // 0x184
    al::CameraInfo* mCatchCamera = nullptr;  // 0x188
    al::CameraInfo* mAnimCamera = nullptr;  // 0x190
    s32 mGoalSe = 0;  // 0x198
    al::SimpleLayoutAppearWait* mClearLayout = nullptr;  // 0x1a0
    GoalPoleStateRunaway* mRunawayState = nullptr;  // 0x1a8
    ActorStateDemoCamera* mFairyFocusState = nullptr;  // 0x1b0
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;  // 0x1b8
    sead::Vector3f mPoleTopTrans = {0.0f, 0.0f, 0.0f};  // 0x1e8
    StageTimer* mStageTimer = nullptr;  // 0x1f8
    s32 mTimerDisplayCount = 0;  // 0x200
    sead::Vector3f mFairyFocusLookAt = {0.0f, 0.0f, 0.0f};  // 0x204
    sead::Quatf mEndingPrevPlayerQuat = sead::Quatf::unit;  // 0x210
    sead::Vector3f mEndingPrevPlayerTrans = {0.0f, 0.0f, 0.0f};  // 0x220
    bool mIsInvalidBind = false;  // 0x22c
    bool mIsEnableFur = true;  // 0x22d
    sead::FixedSafeString<32>* mEndingPrevDemoStartId = nullptr;  // 0x230
    sead::FixedSafeString<32>* mEndingPrevDemoEndId = nullptr;  // 0x238
};

static_assert(sizeof(GoalPole) == 0x240);

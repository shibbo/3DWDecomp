#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
class LiveActor;
}  // namespace al

class GoalPole;

/**
 * @brief Drives one player bound to the goal pole: catch, slide down, jump off and pose.
 */
class GoalPoleBindPuppeteer : public BindPuppeteer {
public:
    GoalPoleBindPuppeteer(const char* pName, GoalPole* pPole, const al::ActorInitInfo& rInfo,
                          const sead::Matrix34f* pPoleMtx);

    void startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor, s32 catchIndex);
    void update();
    void onCatchFailure();
    bool isCatchSuccess() const;
    bool isCatchJust() const;
    bool isEndCatchMove() const;
    bool isEnableStartFall() const;
    bool isEndFalling() const;
    void startFall(s32 heightOrder, s32 catchNum, const GoalPoleBindPuppeteer* pUnder);
    bool isEnableStartJump() const;
    void startJump();
    bool isHeightLevelMax() const;
    bool isEnableAppearFlag() const;
    f32 calcCatchHeightRate() const;
    bool isEnableFallUpperPlayer(const sead::Vector3f& rTrans) const;
    static s32 getJumpStartDelayFrame();
    static s32 getFallFrameMax();
    void startCatch(const char* pActionName);
    void setPuppetTransSyncHostAnim();
    void updateBubbleVerticalMovement(const sead::Vector3f& rBaseTrans);

    void exeDeactive();
    void exeCatchMove();
    void exeCatch();
    void exeClimbWallRun();
    void exeClimbWallFailure();
    void exeCatchTop();
    void exeCatchTopWait();
    void exeWaitFall();
    void exeFall();
    void exeFallEnd();
    void exeTurnWait();
    void exeTurn();
    void exeTurnEnd();
    void exeWaitJump();
    void exeJump();
    void exeLand();
    void exePose();
    void exeCatchFailureStart();
    void exeCatchFailureWait();
    void exeCatchFailure();
    void exeCatchFailureHidden();

    /**
     * @brief Height on the pole at which the player caught it.
     * @return The catch height.
     */
    f32 getCatchHeight() const { return mCatchHeight; }

    /**
     * @brief Order in which the player caught the pole (0 for the first).
     * @return The catch index.
     */
    s32 getCatchIndex() const { return mCatchIndex; }

    /**
     * @brief Rank of the catch height among all players (0 for the highest).
     * @return The height order.
     */
    s32 getHeightOrder() const { return mHeightOrder; }

private:
    GoalPole* mPole;  // 0x20
    const sead::Matrix34f* mPoleMtx;  // 0x28
    bool mIsCatchFailure = false;  // 0x30
    bool mIsClimbStarted = false;  // 0x31
    sead::Vector3f mCatchBaseTrans = {0.0f, 0.0f, 0.0f};  // 0x34
    sead::Vector3f mCatchTrans = {0.0f, 0.0f, 0.0f};  // 0x40
    sead::Vector3f mPoseFront = sead::Vector3f::ez;  // 0x4c
    sead::Vector3f mJumpTargetTrans = {0.0f, 0.0f, 0.0f};  // 0x58
    s32 mJumpStartStep = 0;  // 0x64
    s32 mPoseStartStep = 0;  // 0x68
    f32 mTurnDegree = 12.0f;  // 0x6c
    s32 mTurnStep = 0;  // 0x70
    const GoalPoleBindPuppeteer* mUnderPuppeteer = nullptr;  // 0x78
    f32 mCatchHeight = 0.0f;  // 0x80
    s32 mHeightLevel = 0;  // 0x84
    s32 mCatchIndex = 0;  // 0x88
    s32 mHeightOrder = 0;  // 0x8c
    sead::Vector3f mFrontDir = sead::Vector3f::ez;  // 0x90
    sead::Vector3f mCatchMoveVelocity = {0.0f, 0.0f, 0.0f};  // 0x9c
    s32 mCatchMoveStep = 0;  // 0xa8
    sead::Vector3f mBubbleBaseTrans = {0.0f, 0.0f, 0.0f};  // 0xac
    al::LiveActor* mBubble;  // 0xb8
    s32 mBubbleStep = 0;  // 0xc0
    sead::Vector3f mBubbleFront = sead::Vector3f::ez;  // 0xc4
};

static_assert(sizeof(GoalPoleBindPuppeteer) == 0xd0);

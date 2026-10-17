#pragma once

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class ActorStateGiantBlow;
class BgmRhythmAnimeController;
class BindPuppeteerGroup;
class CoinBlowGenerator;
class TreeBindPuppeteer;

/**
 * @brief A tree that players can climb, shake with attacks or the microphone, and that a giant
 * player blows away.
 */
class Tree : public al::LiveActor {
public:
    explicit Tree(const char* pName);
    ~Tree() override;

    void init(const al::ActorInitInfo& rInfo) override;
    TreeBindPuppeteer* getPuppeteer(s32 index) const;
    void respawn() override;
    void control() override;
    void updateSensorPos();
    void startClipped() override;
    void endClipped() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void appearItem();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void controlEnableHandStandFlag();
    bool isExistClimbPlayer() const;
    void forceRespawn();
    void exeWait();
    void exeReactionStart();
    void exeReaction();
    void exeReactionEnd();
    void playAnimForPalm();
    void exeHoldStart();
    void exeHold();
    void exeRelease();
    void exeGiantBlow();
    void exeAwaitRespawn();

    /** @brief Leaves respawning to a TreeFarLodWatcher instead of the clipping logic. */
    void setRespawnByWatcher() { mRespawnByWatcher = true; }

    /** @return Whether a player can currently do a hand stand on the tree top. */
    bool isEnableHandStand() const { return mIsEnableHandStand; }

private:
    void tryOffSnow();
    void updateRhythmAnim();
    void setJointRotate(f32 rotateZ, f32 rotateY);

    al::LiveActor* mSnowModel = nullptr;                    // 0x148
    al::LiveActor* mSinkedItem = nullptr;                   // 0x150
    bool mIsSnowOn = true;                                  // 0x158
    sead::Matrix34f mTopJointMtx = sead::Matrix34f::ident;  // 0x15c
    sead::Vector3f mTouchPos = sead::Vector3f::zero;        // 0x18c
    sead::Vector3f mTopJointPos = sead::Vector3f::zero;     // 0x198
    f32 mRotateZ = 0.0f;                                    // 0x1a4
    f32 mJointRotateZ[4] = {};                              // 0x1a8
    f32 mRotateY = 0.0f;                                    // 0x1b8
    f32 mJointRotateY[4] = {};                              // 0x1bc
    f32 mJointRotateRate = 0.04f;                           // 0x1cc
    f32 mJointRotateRateAtClimbTree = 0.005f;               // 0x1d0
    BindPuppeteerGroup* mPuppeteerGroup = nullptr;          // 0x1d8
    sead::Vector3f* mBindSensorPos = nullptr;               // 0x1e0
    f32 mSensorBottomOffsetY = 250.0f;                      // 0x1e8
    f32 mSensorTopOffsetY = 500.0f;                         // 0x1ec
    sead::Vector3f* mPuppetSensorPos = nullptr;             // 0x1f0
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;  // 0x1f8
    ActorStateGiantBlow* mGiantBlowState = nullptr;         // 0x208
    al::LiveActor* mBreakModel = nullptr;                   // 0x210
    al::LiveActor* mTraceModel = nullptr;                   // 0x218
    s32 mItemType = -1;                                     // 0x220
    f32 mItemAppearOffsetY = 600.0f;                        // 0x224
    sead::Vector3f mFrontDir = sead::Vector3f::ez;          // 0x228
    bool mIsEnableHandStand = true;                         // 0x234
    CoinBlowGenerator* mCoinBlow = nullptr;                 // 0x238
    BgmRhythmAnimeController* mRhythmAnimCtrl = nullptr;    // 0x240
    BgmRhythmAnimeController* mSnowRhythmAnimCtrl = nullptr;  // 0x248
    bool mIsPalm = false;                                   // 0x250
    al::HitSensor* mItemSensor = nullptr;                   // 0x258
    sead::Vector3f mInitTrans = sead::Vector3f::zero;       // 0x260
    sead::Quatf mInitQuat = sead::Quatf(1.0f, 0.0f, 0.0f, 0.0f);  // 0x26c
    bool mIsSpecialReappear = false;                        // 0x27c
    bool mRespawnByWatcher = false;                         // 0x27d
};

static_assert(sizeof(Tree) == 0x280);

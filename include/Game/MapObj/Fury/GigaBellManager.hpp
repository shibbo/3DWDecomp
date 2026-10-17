#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include "MapObj/Fury/GigaBell.hpp"

namespace al {
class ByamlIter;
class HitSensor;
class WipeSimple;
}  // namespace al
class DemoCutscene;
class DemoObjBase;
class DummyCameraTarget;
class GigaBell3in1;
class SuperBowser;

/**
 * @brief Scene object owning the Giga Bells of Bowser's Fury: it keeps them locked until enough
 * Cat Shines are collected, plays their unlock and return cutscenes and moves them around Fury
 * Bowser during the final Plessie chase.
 */
class GigaBellManager : public al::LiveActor, public al::ISceneObj {
public:
    /// How the bells move around Fury Bowser during the Plessie chase.
    enum class MoveType : s32 {
        Default = 0,
        Swing = 1,
        FigureEight = 2,
        BowTie = 3,
        Rose = 4,
        Circle = 5,
        Atom = 6,
    };

    /// Knockback applied to Fury Bowser when a bell is rung during the Plessie chase.
    struct PlessieChaseKnockbackParam {
        void initFromYaml(al::ByamlIter* pIter, const char* pKey);

        f32 height;
        f32 distance;
        s32 frames;
        f32 slideDistance;
        s32 slideFrames;
        s32 delayFrames;
        f32 rotationDeg;
    };

    /// Movement pattern of the bells around Fury Bowser.
    struct MovePatternParam {
        f32 moveTime;
        f32 moveDistance;
        f32 moveHeight;
        f32 offsetY;
    };

    /// Rose curve movement pattern of the bells.
    struct MovePatternRoseParam {
        f32 moveTime;
        f32 moveDistance;
        f32 moveHeight;
        f32 offsetY;
        f32 coefficient;
        f32 rotationOffset;
        f32 smoothness;
    };

    /// Atom-like movement pattern of the bells.
    struct MovePatternAtomParam {
        f32 moveTime;
        f32 moveDistance;
        f32 moveHeight;
        f32 offsetY;
        f32 rotateSpeed;
    };

    /// Camera of the final hit of the Plessie chase.
    struct FinalHitCameraParam {
        s32 camera1PlayTime;
        s32 camera1FreezeTime;
        s32 camera2PlayTime;
        s32 camera2FreezeTime;
        s32 camera3PlayTime;
        s32 cameraInterpBackFrames;
        f32 camera2AngleV;
        f32 camera2Distance;
        sead::Vector3f camera2Offset;
        f32 camera2AngleHOffset;
        f32 camera3AngleV;
        f32 camera3Distance;
        sead::Vector3f camera3Offset;
        f32 camera3AngleHOffset;
    };

    explicit GigaBellManager(const char* pName);
    static GigaBellManager* tryGetManager(const al::IUseSceneObjHolder* pHolder);
    void init(const al::ActorInitInfo& rInfo) override;
    void initFromYaml();
    void setLockCount(s32 lockCount);
    void exeWatch();
    bool shouldPlayUnlockCutscene();
    s32 getIndexOfGigaBellClosestToPlayer();
    void exeWaitDemo();
    void exeWaitOpenBell();
    void exeWaitResetBell();
    void exePlessieChase();
    void exePlessieChaseReset();
    GigaBell* getGigaBell(s32 index);
    bool tryQueueCutscene();
    void disasterModeStart();
    bool tryQueueReturnCutscene(bool isCheckOnly, bool isSkipDisasterDemo, bool isStartDemo);
    void gigaBellCollected(GigaBell* pBell);
    void incrementLockCount();
    GigaBell* getGigaBellClosestTo(sead::Vector3f pos);
    void endCutscene(bool isSelf);
    void endReturnCutscene();
    s32 getGigaBellCount();
    void shineCollected();
    void forceUnlockBells();
    al::WipeSimple* getWipeFadeBlack();
    al::WipeSimple* getWipeFadeWhite();
    void startCloseFadeWhite();
    bool hasLockCountBeenIncremented();
    bool isCutsceneDone();
    bool isInUnlockCutscene();
    void resetGigaBells();
    DemoCutscene* getNextExplainationCutscene(s32 layerId, s32* pCutsceneId);
    DemoCutscene* getExplainationCutscene(s32 index);
    void startPlessieChase(SuperBowser* pBowser);
    void hitBellPlessieChase();
    void tryHitNextBellPlessieChase(al::HitSensor* pOther, al::HitSensor* pSelf);
    void updateMoveType();
    sead::Vector3f calcMoveOffset(s32 index);
    s32 getPlessieChaseHitCount();
    bool isPlessieChaseLv(s32 level);
    sead::Vector3f getPlessieChaseBellBaseOffset();
    void plessieChaseReset(s32 hitCount, s32 tier, bool isFinal);
    bool canHitBellPlessieChase();
    void doPlessieChaseHitRepeat(s32 index);
    void setLastHitBell(GigaBell* pBell);
    GigaBell3in1* getGigaBell3in1();
    void toggleLoopCollect();
    PlessieChaseKnockbackParam getCurrentKnockbackParam();

    /** @brief Number of goal items needed to unlock the bells. @return The count. */
    s32 getGoalItemsRequired() const { return mLockCount; }

    /** @brief Remember the goal item count shown by the last unlock cutscene. */
    void updateUnlockedGoalItems() { mUnlockedGoalItems = mLockCount; }

    /** @brief Demo played when a bell is collected. @return The demo, or nullptr. */
    DemoObjBase* getCollectDemo() const { return mCollectDemo; }

    /** @brief Whether the camera should return to the stored pose. @return The flag. */
    bool isCameraReturn() const { return mIsCameraReturn; }

    /** @brief Clear the camera return request. */
    void clearCameraReturn() { mIsCameraReturn = false; }

    /** @brief Smallest number of goal items needed to unlock the bells. @return The count. */
    s32 getLockCountMin() const { return mLockCountMin; }

private:
    s32 calcIndexOfGigaBellClosestTo(const sead::Vector3f& rPos) const;

    s32 mLockCount = -1;
    s32 mGigaBellNum = 0;
    GigaBell** mGigaBells = nullptr;
    s32 mUnlockedGoalItems = -1;
    DummyCameraTarget* mCameraTarget = nullptr;
    DemoObjBase* mCollectDemo = nullptr;
    bool mIsUnlocked = false;
    bool mIsSkipDisasterDemo;
    bool mIsReturnDemoStarted = false;
    GigaBell::LockState mLockState = GigaBell::LockState::Locked;
    al::WipeSimple* mWipeFadeBlack = nullptr;
    al::WipeSimple* mWipeFadeWhite;
    bool mIsCameraReturn = false;
    SuperBowser* mSuperBowser;
    bool mIsLoopCollect = false;
    bool mIsPlessieChaseFinal = false;
    bool mIsDebugMoveType = false;
    s32 mPlessieChaseHitCount = 0;
    MoveType mMoveType = MoveType::Default;
    MoveType mDebugMoveType = MoveType::Default;
    sead::Vector3f mBaseSocketOffset{0.0f, 1000.0f, 7000.0f};
    sead::Vector3f mBaseSocketOffsetFirst{0.0f, 750.0f, 6000.0f};
    s32 mMoveFrame = 0;
    GigaBell* mLastHitBell = nullptr;
    GigaBell3in1* mGigaBell3in1 = nullptr;
    MovePatternParam mMoveDefault = {240.0f, 1400.0f, 0.25f, 0.0f};
    MovePatternParam mMoveSwing = {240.0f, 1400.0f, 0.25f, 900.0f};
    MovePatternParam mMoveFigureEight = {200.0f, 2000.0f, 0.3f, 0.0f};
    MovePatternParam mMoveBowTie = {130.0f, 1600.0f, 0.4f, 0.0f};
    MovePatternRoseParam mMoveRose = {270.0f, 1200.0f, 0.6f, -100.0f, 3.0f, 180.0f, 2.0f};
    MovePatternParam mMoveCircle = {240.0f, 800.0f, 1.0f, 0.0f};
    MovePatternAtomParam mMoveAtom = {240.0f, 800.0f, 1.0f, 0.0f, 0.5f};
    s32 mLockCountMin = -1;
    s32 mLockCountMax = -1;
    s32 mLockCountIncrement = 0;
    sead::PtrArray<DemoCutscene> mExplainCutscenes;
    u8 _280[0x2a0 - 0x280];
    FinalHitCameraParam mFinalHitCamera = {35,     40,      35,     40,
                                           150,    180,     -20.0f, 3000.0f,
                                           {1600.0f, 1200.0f, -2000.0f}, 340.0f,
                                           -10.0f, 1500.0f, {-800.0f, 400.0f, -100.0f}, 15.0f};
    PlessieChaseKnockbackParam mKnockback = {1600.0f, 14000.0f, 50, 4000.0f, 30, 3, -35.0f};
    PlessieChaseKnockbackParam mKnockbackFinalHit = {2500.0f, 18000.0f, 50, 4000.0f, 30, 3, 0.0f};
    PlessieChaseKnockbackParam mKnockbackP10 = {1800.0f, 11200.0f, 50, 3200.0f, 30, 3, -35.0f};
    PlessieChaseKnockbackParam mKnockbackFinalHitP10 = {900.0f, 6000.0f, 25, 2400.0f, 35, 3, 0.0f};
};

static_assert(sizeof(GigaBellManager) == 0x358);

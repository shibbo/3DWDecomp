#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class SuperBowserGiantFireballState;
class SuperBowserLaserState;

/**
 * @brief Fury Bowser, the giant Bowser of Bowser's Fury.
 * @note Only the members used by reconstructed code are declared.
 */
class SuperBowser : public al::LiveActor {
  public:
    /// Placement Fury Bowser currently appears from.
    struct SpawnInfo {
        sead::Vector3f mTrans;     // 0x0
        sead::Vector3f mFrontDir;  // 0xc
    };

    /// State of the Plessie chase of the last battle.
    struct PlessieChaseState {
        u8 _0[0x40];
        s32 mTier;  // 0x40
    };

    /// Target and joint limits Fury Bowser's head and spine follow while aiming.
    struct BowserJointState {
        s32 _0;
        f32 mRate;                 // 0x4
        sead::Vector3f mTarget;    // 0x8
        f32 mConstraintBeam;       // 0x14
        f32 mConstraintFace;       // 0x18
        f32 mConstraintNeck;       // 0x1c
        f32 mConstraintSpine;      // 0x20
        f32 mConstraintHip;        // 0x24
        f32 mPitchBeam;            // 0x28
        f32 mPitchFace;            // 0x2c
        f32 mPitchNeck;            // 0x30
        f32 mPitchSpine;           // 0x34
        f32 mPitchHip;             // 0x38
    };

    bool isLastPhase3Bowser();
    const char* getActionName(const char* pBaseName);
    void startJointAim(BowserJointState state, s32 frames, s32 jointNum, s32 blendFrames);
    void updateJointAim(sead::Vector3f target);
    void updateTurning(const char* pActionName, s32 step);
    bool tryCancelNextLaser();
    void setGuideBalloonVisible(bool isVisible);
    s32 getBaseOffset() const;
    bool isPlessieChaseLv4() const;
    void disappear(bool isInstant);
    void tryDamageDarkBowser();
    void setFacingDirection(sead::Vector3f& rDir);
    void startBowserExitCamera(bool, bool, bool);
    void endBowserExitCamera(bool isSkip);
    bool isHidden();
    f32 getJumpRate();
    SuperBowserLaserState* getLaserState();
    SuperBowserGiantFireballState* getGiantFireballState();
    s32 getGiantFireballShootCount();
    void forceKillFireballs();
    bool isDoingEndingPreparations();
    s32 getLastPhase3CurrentIndex() const;
    bool isPlessieChaseBigRamp() const;
    SpawnInfo* getCurrentSpawnInfo();
    void notifyPlayerHit();
    void setLaunchSpikeQueueCount(s32 count);
    void clearLaunchSpikeAmbientFrameCount();
    void tryLaunchSpike();
    bool hasLanded();
    void doPlessieChaseHit(bool isReset);
    void resetPlessieChase(s32 tier);
    bool canPlessieChaseHit();
    bool isPlessieChaseTierJump() const;
    explicit SuperBowser(const char* pName);
    void switchToV2(bool isV2);
    bool isRepelling();
    void pauseForDemo();
    void resumeFromDemo();
    void setLastPhase3Bowser(bool isLast);
    void killHealthBar();
    void activateCamera();
    void deactivateCamera();
    void requestDisappear(void (*pStartCallback)(void*), void (*pEndCallback)(void*), void* pArg,
                          bool isInstant);
    void end();
    void endByTime();
    void repel();
    bool isLeaving();
    void cancelFlow();
    void setUseSecondaryFlow(bool isUse);
    void enableLaserLightEffects(bool isEnable);

    /**
     * @brief Check whether Fury Bowser is out in the world.
     * @return The flag.
     */
    bool isActive() const { return mIsActive; }

    /** @brief Stop Fury Bowser's attack timers. */
    void stopAttackTimer() { mIsAttackTimerActive = false; }

    /** @brief Toggle Fury Bowser's laser attack. */
    void toggleLaserAttack() { mIsLaserAttack = !mIsLaserAttack; }

    /** @brief Remember that the first appearance cutscene was shown. */
    void setFirstAppearDone() { mIsFirstAppearDone = true; }

    /**
     * @brief Check whether Fury Bowser should leave as soon as possible.
     * @return The flag.
     */
    bool isLeaveRequested() const { return mIsLeaveRequested; }

    /**
     * @brief Set whether Fury Bowser should leave as soon as possible.
     * @param isRequested The flag.
     */
    void setLeaveRequested(bool isRequested) { mIsLeaveRequested = isRequested; }

    /**
     * @brief Set whether Fury Bowser may shoot fireballs.
     * @param isEnable The flag.
     */
    void setFireballEnable(bool isEnable) { mIsFireballEnable = isEnable; }

    /**
     * @brief Check whether the Plessie chase is in its first tier.
     * @return True when a chase is running and it is in its first tier.
     */
    bool isPlessieChaseFirstTier() const {
        return mPlessieChaseState != nullptr && mPlessieChaseState->mTier == 0;
    }

    /**
     * @brief Check whether the Giga Bells ignore their pattern's vertical offset.
     * @return The flag.
     */
    bool isIgnoreBellOffsetY() const { return mIsIgnoreBellOffsetY; }

    /**
     * @brief Get the suffix appended to the attack init file names, if any.
     * @return The suffix, or nullptr.
     */
    const char* getInitFileSuffix() const { return mInitFileSuffix; }

    /**
     * @brief Get the kind of Fury Bowser (selects his charge effects and sounds).
     * @return The kind.
     */
    s32 getBowserType() const { return mBowserType; }

private:
    u8 _144[0x150 - 0x144];
    bool mIsActive;  // 0x150
    u8 _151[0x1a8 - 0x151];
    bool mIsAttackTimerActive;  // 0x1a8
    u8 _1a9[0x1f0 - 0x1a9];
    bool mIsLaserAttack;  // 0x1f0
    bool mIsFirstAppearDone;  // 0x1f1
    bool mIsLeaveRequested;  // 0x1f2
    bool mIsFireballEnable;  // 0x1f3
    u8 _1f4[0x1f8 - 0x1f4];
    const char* mInitFileSuffix;  // 0x1f8
    u8 _200[0x208 - 0x200];
    s32 mBowserType;  // 0x208
    u8 _20c[0x210 - 0x20c];
    PlessieChaseState* mPlessieChaseState;  // 0x210
    u8 _218[0x50f - 0x218];
    bool mIsIgnoreBellOffsetY;  // 0x50f
};

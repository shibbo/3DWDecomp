#pragma once

#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObj;
class CollisionObj;
}  // namespace al

class CoinBlow;
class CoinBlowConcentric;
class DisasterSpikeDirector;
class DisasterSpikeDirt;

/// Spike that falls from the sky during Bowser's Fury disasters.
class DisasterSpike : public al::LiveActor {
public:
    explicit DisasterSpike(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void setActive(bool isActive, bool isCrumble);
    void setHot(bool isHot);
    void updateHotCollisionMtx();
    void initAfterPlacement() override;
    void appear() override;
    void appearChildren();
    void hideChildren();
    void invalidateClipping();
    void start();
    void kill() override;
    void startNerveAction(const char* pActionName);
    bool isReplacedByGold() const;
    bool tryAppear(bool isForce);
    bool canFall();
    void clipInAtAllFallTime();
    void setDisasterSpikeDirector(DisasterSpikeDirector* pDirector);
    void setOceanSpike();
    void attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    bool canCrumble();
    void dieByLaser();
    bool canCrumbleOceanSpike();
    void control() override;
    void fadeShadow(f32 rate);
    bool hasLanded();
    bool isPositionInCameraView(sead::Vector3f pos);
    void validateClipping();
    void startClipped() override;
    void endClipped() override;
    bool isDisaster() const;
    void hide();
    void updateEchoPulse();
    void exeDeactivate();
    void exeShadowFadeInDelay();
    void exeShadowFadeIn();
    void setShadowDropLengthPercent(f32 percent);
    void exeMoveDelay();
    void exeMove();
    sead::Vector3f getCurrentPosition();
    sead::Vector3f getNextPosition();
    sead::Quatf getCurrentQuat();
    sead::Quatf getNextQuat();
    void setTransform(sead::Vector3f pos, sead::Quatf quat);
    void updateShadowDirection();
    sead::Vector3f getEndPosition();
    sead::Vector3f getStartPosition();
    void startLand();
    void exeLand();
    void land();
    void exeLandWater();
    void exeLandWait();
    void updateCollisionValidation();
    void exeLandOcean();
    void exeShakeDelay();
    void shake(s32 frames);
    void exeShake();
    void exeShakeWait();
    void exeCrumble();
    void updateCrumble();
    void exeCrumbleLaser();
    void exeLaserDeath();
    void exeSinkDelay();
    void exeSink();
    void exeClipInAfterDemo();
    void setPosition(sead::Vector3f pos);
    void resetTransform();
    bool isWithinSpikeNoFallRadius();
    bool canBeReplacedByGold() const;
    void showChildren();
    void crumbleChildren();
    void killChildren();
    sead::Quatf getStartQuatWithOffset(f32 offsetDegree);
    DisasterSpike* getLastMove();
    bool isHot() const;
    void turnIntoCoins();
    void setKillOutOfView(bool isKill);
    bool isTriggered();
    void setTriggered(bool isTriggered);
    void setReplacedByGold(bool isReplaced);
    void clearGoldSpikeOriginalSpike();
    bool isGoldSpikeInUse() const;
    s32 getChildCount();
    al::LiveActor* getChild(s32 index);
    void crumble();
    bool checkGround(sead::Vector3f& rPos, sead::Quatf& rQuat);
    bool isAppearPositionInCameraView();
    bool isPositionInSpikeNoFallCylinder(sead::Vector3f pos, f32 radius);
    bool isWithinCollideDistance();

    DisasterSpikeDirector* getDisasterSpikeDirector() const { return mDisasterSpikeDirector; }

    /**
     * @brief Check whether the spike falls straight from the sky instead of along MoveNext links.
     * @return True if the spike has no MoveNext chain and is its own current move point.
     */
    bool isSingleSpike() const { return mCurrentMove == this && mMoveNext == nullptr; }

    /**
     * @brief Get the spike that is actually shown: the gold spike replacing this one, if any.
     * @return The gold spike in use, or this spike.
     */
    DisasterSpike* getActiveSpike() {
        if (isReplacedByGold() && mGoldSpike != nullptr) {
            return mGoldSpike;
        }

        return this;
    }

    /**
     * @brief Get the spike owning the children: the original spike for a gold spike in use.
     * @return The spike whose children this spike drives.
     */
    DisasterSpike* getChildOwner() {
        if (mIsGold && mOriginalSpike != nullptr) {
            return mOriginalSpike;
        }

        return this;
    }

protected:
    DisasterSpikeDirector* mDisasterSpikeDirector = nullptr;  // 0x148

private:
    s32 mShadowFadeInDelay = 0;                                // 0x150
    s32 mMoveTime = 45;                                        // 0x154
    s32 mInterpolateType = 0;                                  // 0x158
    s32 mLandHeight = 0;                                       // 0x15c
    s32 _160 = 0;                                              // 0x160
    bool mIsHideTip = true;                                    // 0x164
    s32 mHideTipStepFrames = -1;                               // 0x168
    bool mIsShortSpike = false;                                // 0x16c
    bool mIsHideDirtBottom = false;                            // 0x16d
    bool mIsActive = true;                                     // 0x16e
    DisasterSpike* mMoveNext = nullptr;                        // 0x170
    DisasterSpike* mCurrentMove = this;                        // 0x178
    sead::Vector3f mPosition = sead::Vector3f::zero;           // 0x180
    sead::Quatf mQuat = sead::Quatf::unit;                     // 0x18c
    f32 mRotateOffsetDegree = 0.0f;                            // 0x19c
    al::LiveActor** mChildren = nullptr;                       // 0x1a0
    sead::Vector3f* mChildOffsets;                             // 0x1a8
    s32 mChildCount = 0;                                       // 0x1b0
    CoinBlowConcentric* mCoinBlowConcentric = nullptr;         // 0x1b8
    CoinBlow* mCoinBlow = nullptr;                             // 0x1c0
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;       // 0x1c8
    al::CollisionObj* mHotCollision = nullptr;                 // 0x1f8
    s32 mStep = 0;                                             // 0x200
    s32 mShakeFrames = 30;                                     // 0x204
    sead::Vector3f mShakeBasePos = sead::Vector3f::zero;       // 0x208
    f32 mShadowMaskSize = 100.0f;                              // 0x214
    s32 mShadowFadeStep = 0;                                   // 0x218
    bool mIsShadowFading = false;                              // 0x21c
    bool mIsOcean = false;                                     // 0x21d
    s32 mSinkDelay = 100;                                      // 0x220
    sead::Matrix34f mHotCollisionMtx = sead::Matrix34f::ident; // 0x224
    al::LiveActor* mPlayer = nullptr;                          // 0x258
    bool mIsKillOutOfView = false;                             // 0x260
    bool mIsForeverSpike = false;                              // 0x261
    bool mIsLaserDead = false;                                 // 0x262
    DisasterSpikeDirt* mDirt = nullptr;                        // 0x268
    bool mIsTriggered = false;                                 // 0x270
    bool mHasLanded = false;                                   // 0x271
    bool mIsEchoBlock = false;                                 // 0x272
    s32 mAlphaFadeInFrames = 15;                               // 0x274
    s32 mAlphaFadeInStep = 0;                                  // 0x278
    s32 mEchoTimer = 0;                                        // 0x27c
    bool mIsGold = false;                                      // 0x280
    bool mCanBeGold = false;                                   // 0x281
    bool mIsAlwaysGold = false;                                // 0x282
    bool mIsNeverGold = false;                                 // 0x283
    bool mIsReplacedByGold = false;                            // 0x284
    DisasterSpike* mGoldSpike = nullptr;                       // 0x288
    DisasterSpike* mOriginalSpike = nullptr;                   // 0x290
    f32 mNpcAvoidRadius = 1.0f;                                // 0x298
    bool mIsFadingOut = false;                                 // 0x29c
    f32 mFadeOutAlpha = 1.0f;                                  // 0x2a0
    sead::Vector3f mPlayerPushPos = sead::Vector3f::zero;      // 0x2a4
    al::AreaObj* mNoFallArea = nullptr;                        // 0x2b0
    sead::Vector3f mNoFallAreaCorners[8];                      // 0x2b8
};

static_assert(sizeof(DisasterSpike) == 0x318);

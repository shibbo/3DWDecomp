#pragma once

#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class BallStateThrow;
class ItemStateGigaPlayerHold;
class SePlayObj;

/** @brief Flight tuning of a spike thrown by (Fury) Bowser. */
struct BowserSpikeParam {
    s32 mLaunchStep;        ///< Steps spent rising before the spike starts its flight.
    f32 mLaunchSpeed;       ///< Upward speed added every step while rising.
    f32 mGravity;           ///< Downward speed added every step while falling.
    s32 mFallStartStep;     ///< Step of the flight after which the spike starts falling.
    f32 mVelocityScale;     ///< Velocity damping applied every step.
};
static_assert(sizeof(BowserSpikeParam) == 0x14);

/**
 * @brief Spike shot up by Bowser that falls down onto a target position, sticks in the ground and
 * pulses until it explodes. The giga player can pick it up and throw it back.
 */
class BowserSpike : public al::LiveActor {
public:
    BowserSpike(const char* pName, const BowserSpikeParam* pParam);
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void turnOnCollider();
    void control() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void setupMaterialSettings();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableHold(al::HitSensor* pSensor) const;
    void updateCollider() override;
    void exePrepare();
    void exeLaunch();
    void exeFly();
    void updateShadowEffects();
    void exeLand();
    void turnOnCollision();
    void exeWait();
    void exePulse();
    void exeDie();
    void exeThrow();
    void exeHold();
    void prepareLaunch(const sead::Vector3f& rPos, const sead::Quatf& rQuat, bool isGolden);
    void quickEnd();
    void launch(const sead::Vector3f* pTarget, f32 degree, f32 distance,
                const BowserSpikeParam* pParam);
    void delaunch();

private:
    const BowserSpikeParam* mParam;                    // 0x148
    const sead::Vector3f* mLaunchTarget = nullptr;     // 0x150
    sead::Vector3f mLandPos = sead::Vector3f::zero;    // 0x158
    f32 mLaunchDegree = 0.0f;                          // 0x164
    f32 mLaunchDistance = 0.0f;                        // 0x168
    f32 mShadowMaskSize = 100.0f;                      // 0x16c
    bool mIsGolden = false;                            // 0x170
    bool mIsInWater = false;                           // 0x171
    bool mIsExplode = true;                            // 0x172
    s32 mPulseCount = 0;                               // 0x174
    u8 _178[0x188 - 0x178];
    al::HitSensor* mHolderSensor = nullptr;            // 0x188
    BallStateThrow* mStateThrow = nullptr;             // 0x190
    ItemStateGigaPlayerHold* mStateHold = nullptr;     // 0x198
    SePlayObj* mLandWarningSe = nullptr;               // 0x1a0
    bool mIsShowGuide = false;                         // 0x1a8
    bool mIsPlayerCanCarry = false;                    // 0x1a9
};
static_assert(sizeof(BowserSpike) == 0x1b0);

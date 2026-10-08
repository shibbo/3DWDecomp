#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadColor.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Boss/DarkBowserLaserIndicator.hpp"
#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"

namespace al {
class ActorInitInfo;
class EffectSystem;
class HitSensor;
class LppLine;
class SceneObjHolder;
}  // namespace al

class PlayerActor;
class SePlayObj;
class SuperBowser;

/**
 * @brief Tuning of one variant of Fury Bowser's laser, read from the "InitAttack" init files.
 */
struct SuperBowserLaserStateParam {
    void init(const SuperBowser* pBowser, const al::ActorInitInfo& rInfo);
    void initWithName(const SuperBowser* pBowser, const al::ActorInitInfo& rInfo,
                      const char* pFileName, const char* pKey);

    f32 chargeFrames = 180.0f;                                   // 0x0
    f32 chargeFramesSecondary = 60.0f;                           // 0x4
    s32 chargeFramesUntilTargetLock = 0;                         // 0x8
    s32 chargeFramesUntilTargetLockLaserBeginChase = 5;          // 0xc
    bool isSweepEnable = true;                                   // 0x10
    bool isSweepLeftThenRight = false;                           // 0x11
    s32 preSweepFrames = 60;                                     // 0x14
    f32 sweepFrames = 90.0f;                                     // 0x18
    s32 postSweepFrames = 0;                                     // 0x1c
    f32 sweepOffsetStart = 4400.0f;                              // 0x20
    f32 sweepOffsetEnd = 1400.0f;                                // 0x24
    f32 laserRadiusInner = 150.0f;                               // 0x28
    f32 laserRadiusOuter = 400.0f;                               // 0x2c
    f32 laserRadiusSensor = 400.0f;                              // 0x30
    f32 laserRadiusInnerVerticalOffset = 50.0f;                  // 0x34
    f32 laserDisasterBlockRadius = 400.0f;                       // 0x38
    f32 laserRadiusSensorPlayerOnly = 225.0f;                    // 0x3c
    f32 laserAngleMin = -1.0f;                                   // 0x40
    f32 laserAngleMax = 1.0f;                                    // 0x44
    bool isSweepFollowTarget = true;                             // 0x48
    s32 laserGrowDelayFrames = 8;                                // 0x4c
    f32 penetrationOffsetMin = 150.0f;                           // 0x50
    f32 penetrationOffsetMax = -350.0f;                          // 0x54
    f32 penetrationDegMin = 10.0f;                               // 0x58
    f32 penetrationDegMax = 70.0f;                               // 0x5c
    f32 damageSensorOffset = 0.0f;                               // 0x60
    sead::Vector3f laserColor = {30.0f, 0.0f, 0.0f};             // 0x64
    f32 verticalAimOffset = 0.0f;                                // 0x70
    f32 verticalAimOffsetPlessie = -50.0f;                       // 0x74
    f32 effectScale = 9.0f;                                      // 0x78
    s32 extendFrames = 8;                                        // 0x7c
    f32 followAccelTangent = 0.0015f;                            // 0x80
    f32 followAccelTangentPlessieChaseV2Lv3 = 0.001f;            // 0x84
    f32 followAccelCross = 0.0015f;                              // 0x88
    f32 followAccelCrossPlessieChaseLv2 = 0.00045f;              // 0x8c
    f32 followAccelCrossPlessieChaseLv3 = 0.00045f;              // 0x90
    f32 followAccelNormal = 0.0015f;                             // 0x94
    f32 followDamp = 0.95f;                                      // 0x98
    s32 followFramesPre = 60;                                    // 0x9c
    s32 followFramesPrePlessieChaseLv2 = 5;                      // 0xa0
    s32 followFramesPrePlessieChaseLv3 = 2;                      // 0xa4
    s32 followFrames = 200;                                      // 0xa8
    f32 followOffsetNormal = 0.0f;                               // 0xac
    f32 followOffsetTangent = 0.0f;                              // 0xb0
    f32 followOffsetTangentPlessieChaseV2Lv3 = 3000.0f;          // 0xb4
    f32 followOffsetCross = 0.0f;                                // 0xb8
    f32 followOffsetCrossPlessie;                                // 0xbc
    f32 followOffsetCrossPlessieV2Lv2 = 3500.0f;                 // 0xc0
    f32 followOffsetCrossPlessieV2Lv3 = 4000.0f;                 // 0xc4
    bool isFollowOffsetCrossFlip = false;                        // 0xc8
    bool isFollowOffsetCrossLeftToRight = false;                 // 0xc9
    bool isFollowOffsetCrossLeftToRightLv2 = false;              // 0xca
    s32 laserCount = 1;                                          // 0xcc
    f32 targetVelocityOffsetScale = 0.0f;                        // 0xd0
    f32 targetVelocityOffsetScalePlessieChaseV2 = 100.0f;        // 0xd4
    f32 targetVelocityOffsetRate = 0.05f;                        // 0xd8
    s32 laserEarlyStartFrame = 19;                               // 0xdc
    f32 laserOuterRayAngleThreshold = 30.0f;                     // 0xe0
    f32 laserSlopeThresholdAngle = 45.0f;                        // 0xe4
    s32 laserVelocityLockFrame = 90;                             // 0xe8
    s32 laserVelocityLockFramePlessie = 120;                     // 0xec
    s32 laserVelocityLockFramePlessieChase = 60;                 // 0xf0
    s32 laserVelocityLockFramePlessieChaseV2 = 45;               // 0xf4
    f32 jointPitchBeam = 0.0f;                                   // 0xf8
    f32 jointPitchFace = -45.0f;                                 // 0xfc
    f32 jointPitchNeck = -45.0f;                                 // 0x100
    f32 jointPitchSpine = -45.0f;                                // 0x104
    f32 jointPitchHip = 0.0f;                                    // 0x108
    f32 jointConstraintBeam = 360.0f;                            // 0x10c
    f32 jointConstraintFace = 17.0f;                             // 0x110
    f32 jointConstraintNeck = 17.0f;                             // 0x114
    f32 jointConstraintSpine = 32.0f;                            // 0x118
    f32 jointConstraintHip = 0.0f;                               // 0x11c
    s32 beamEndFrames = 10;                                      // 0x120
    s32 beamEndFramesOfDamage = 7;                               // 0x124
    f32 beamEndAcceleration = 2.0f;                              // 0x128
    f32 laserLightSourceYOffset = 800.0f;                        // 0x12c
    f32 playerLineCheckStartPosYOffset = 0.0f;                   // 0x130
    f32 playerLineCheckStartPosYOffsetBeforeJump = 1000.0f;      // 0x134
};
static_assert(sizeof(SuperBowserLaserStateParam) == 0x138);

/**
 * @brief Laser attack state of Fury Bowser: charges while tracking the player, then fires a
 * beam that is cast as a bundle of rays against the collision, follows (or sweeps across) its
 * target and finally shrinks away.
 */
class SuperBowserLaserState : public al::HostStateBase<SuperBowser>,
                              public al::IUseSceneObjHolder {
public:
    /// One of the collision rays the beam is made of.
    struct Ray {
        bool isHit = false;                           // 0x0
        sead::Vector3f start = sead::Vector3f::zero;  // 0x4
        sead::Vector3f hitPos = sead::Vector3f::zero;  // 0x10
        sead::Vector3f normal = sead::Vector3f::ey;   // 0x1c
        f32 angle = 0.0f;                             // 0x28
        al::HitSensor* sensor = nullptr;              // 0x30
        bool isHitWater;                              // 0x38
    };

    /// Function steering the aim of the beam while it is shot.
    using AimFunc = void (SuperBowserLaserState::*)(const sead::Vector3f&);

    SuperBowserLaserState(SuperBowser* pHost, const al::ActorInitInfo& rInfo);

    void updateAimTrack(const sead::Vector3f& rOrigin);
    void appear() override;
    void setAimFunction();
    virtual void forceKill();
    void disableLaserSensor();
    void laserLightSettingsRestore();
    void exeCharge();
    void startCharge();
    void laserLightSettingsSetColor(const sead::Vector3f& rColor);
    void laserLightSettingsSetDirection(const sead::Vector3f& rDir);
    void updateTargetData();
    void calcLaserTarget(sead::Vector3f& rTarget);
    void updateChargeLook(sead::Vector3f target);
    u32 getChargeFramesUntilTargetLock();
    void lockTarget();
    void updateEmbers();
    void beginNow();
    void exeBegin();
    void updateLaser();
    void exeShoot();
    void exeEnd();
    bool shouldSkipEndAnim() const;
    void startShrink();
    bool isCharging() const;
    bool isShooting() const;
    bool isEnding() const;
    void forceTarget(const sead::Vector3f& rTarget, bool isKeepCross);
    void forceSweepTarget(const sead::Vector3f& rStart, const sead::Vector3f& rEnd);
    void forceTargetDisable();
    const char* getChargeEffectName() const;
    void setBeamColor(sead::Color4f color);
    bool tryDamagePlayer(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool checkPlayerLineSegment(bool isUpdateStart);
    void endAfterDelay();
    void tryUpdateLaserEndEffect();
    sead::Vector3f getLaserOrigin() const;
    sead::Vector3f calcLaserEffectScale();
    void setLaserBeamEffectScale(sead::Vector3f scale);
    void laserLightSettingsRecord();
    void laserLightSettingsSet(const sead::Vector3f& rDir, const sead::Vector3f& rColor);
    void calcSweepPositions();
    void limitLaserAngle();
    void cancel();
    void endLaser();
    void effectsLaserStop();
    void startLaserEndEffect();
    bool isLaserShrinking();
    void enableLaserLightEffects(bool isEnable);
    bool isEnableLaserLightEffects() const;
    const SuperBowserLaserStateParam* getParam() const;
    void updateAimSweep(const sead::Vector3f& rOrigin);
    s32 getFollowFramesPre();
    sead::Vector3f calcLaserAccel() const;
    void calcLaserBasis();
    void calcRayOrigins();
    void initRayInfo();
    void calcHitPosition();
    bool shouldUpdateRay(s32 frame, s32 index) const;
    void doRaySpecialSensorChecks(s32 index);
    bool canRayBlockLaser(s32 index) const;
    void updateLaserLength(sead::Vector3f pos, f32 length);
    void checkHitDisasterSpike();
    void checkHitDisasterBlocks();
    void updateLaserPenetration();
    sead::Vector3f calcTargetVelocityOffset();
    void effectsLaserStart();
    void effectsLaserHitPlay();
    bool shouldPlayWaterHitEffect() const;
    void effectsLaserUpdate();
    void effectsLaserScale();
    void enableLaserSensor();
    void updateMidBeamRate();
    void updateMidBeamSensors();
    sead::Vector3f calcMidBeamPosition(f32 rate) const;
    void calcRayLeadIndex();
    void updateDamagePosition();
    void updateLaserHitEffectMtx();
    const char* getChargeSeName() const;
    bool isHitSurfaceGround(s32 index) const;
    void doRayToSensorCheck(s32 index, al::HitSensor* pSensor);
    bool isRayHitNearClosest(s32 index) const;

    al::SceneObjHolder* getSceneObjHolder() const override { return mSceneObjHolder; }

private:
    static constexpr s32 cRayNum = 15;
    static constexpr s32 cSensorNum = 9;

    PlayerActor* mPlayer = nullptr;                                   // 0x28
    sead::Vector3f mPlayerPrevTrans = sead::Vector3f::zero;           // 0x30
    sead::Vector3f mPlayerVelocity = sead::Vector3f::zero;            // 0x3c
    sead::Vector3f mVelocityOffset = sead::Vector3f::zero;            // 0x48
    al::HitSensor* mMouthSensor = nullptr;                            // 0x58
    sead::Vector3f mSweepStart = sead::Vector3f::zero;                // 0x60
    sead::Vector3f mSweepEnd = sead::Vector3f::zero;                  // 0x6c
    sead::Vector3f mTarget = sead::Vector3f::zero;                    // 0x78
    sead::Vector3f mLaserTarget;                                      // 0x84
    s32 mHitSeCounter = 0;                                            // 0x90
    bool mIsLightSettingsRecorded = false;                            // 0x94
    SuperBowserLaserStateParam mParam;                                // 0x98
    SuperBowserLaserStateParam mParamBigRamp = mParam;                // 0x1d0
    SuperBowserLaserStateParam mParamFinalAttack = mParam;            // 0x308
    SuperBowserLaserStateParam mParamLv4 = mParam;                    // 0x440
    SuperBowserLaserStateParam mParamV2 = mParam;                     // 0x578
    SuperBowserLaserStateParam mParamBigRampV2 = mParam;              // 0x6b0
    SuperBowserLaserStateParam mParamFinalAttackV2 = mParam;          // 0x7e8
    SuperBowserLaserStateParam mParamLv4V2 = mParam;                  // 0x920
    const SuperBowserLaserStateParam* mCurrentParam = &mParam;        // 0xa58
    sead::Vector3f mSavedLightDir = sead::Vector3f::ey;               // 0xa60
    sead::Vector3f mChargeLightDir;                                   // 0xa6c
    sead::Vector3f mSavedLightColor = {1.0f, 1.0f, 1.0f};             // 0xa78
    sead::Vector3f mLaserOrigin = sead::Vector3f::zero;               // 0xa84
    sead::Vector3f mLaserDir = sead::Vector3f::ex;                    // 0xa90
    sead::Vector3f mSide;                                             // 0xa9c
    sead::Vector3f mUp;                                               // 0xaa8
    sead::Vector3f mFront;                                            // 0xab4
    Ray mRays[cRayNum];                                               // 0xac0
    sead::Vector3f mLaserEnd = sead::Vector3f::zero;                  // 0xe80
    sead::Vector3f mSensorPos[cSensorNum] = {
        sead::Vector3f::zero, sead::Vector3f::zero, sead::Vector3f::zero,
        sead::Vector3f::zero, sead::Vector3f::zero, sead::Vector3f::zero,
        sead::Vector3f::zero, sead::Vector3f::zero, sead::Vector3f::zero};  // 0xe8c
    sead::Vector3f mPlayerOnlySensorPos = sead::Vector3f::zero;       // 0xef8
    f32 mLaserLength = 3.4028235e+38f;                                // 0xf04
    s32 mLaserGrowCounter = 0;                                        // 0xf08
    u32 mChargeFrames = 350;                                          // 0xf0c
    f32 mSweepSign = 1.0f;                                            // 0xf10
    al::SceneObjHolder* mSceneObjHolder = nullptr;                    // 0xf18
    bool mIsForceTarget = false;                                      // 0xf20
    bool mIsForceTargetKeepCross = false;                             // 0xf21
    sead::Vector3f mForceTarget = sead::Vector3f::zero;               // 0xf24
    sead::Vector3f mForceSweepEnd = sead::Vector3f::zero;             // 0xf30
    sead::Vector3f mAimVelocity = {0.0f, 0.0f, 0.0f};                 // 0xf3c
    sead::Vector3f mEndVelocity = sead::Vector3f::zero;               // 0xf48
    AimFunc mAimFunc = &SuperBowserLaserState::updateAimTrack;        // 0xf58
    f32 mCrossSign = 1.0f;                                            // 0xf68
    s32 mLaserNum = 0;                                                // 0xf6c
    sead::Matrix34f mLaserCapMtx = sead::Matrix34f::ident;            // 0xf70
    sead::Matrix34f mLaserLightMtx = sead::Matrix34f::ident;          // 0xfa0
    sead::Matrix34f mEmbersMtx = sead::Matrix34f::ident;              // 0xfd0
    s32 mExtendFrame = 0;                                             // 0x1000
    f32 mExtendStartLength = 0.0f;                                    // 0x1004
    sead::Matrix34f mHitEffectMtx = sead::Matrix34f::ident;           // 0x1008
    sead::Matrix34f mLaserMtx = sead::Matrix34f::ident;               // 0x1038
    s32 mClosestRayIndex = 0;                                         // 0x1068
    s32 mLeadRayIndex[2] = {-1, -1};                                  // 0x106c
    s32 mShootFrame = 0;                                              // 0x1074
    f32 mWaterSurfaceY = 0.0f;                                        // 0x1078
    bool mIsHitWater = false;                                         // 0x107c
    s32 mJointAimFrames = 120;                                        // 0x1080
    bool mIsSweep = false;                                            // 0x1084
    bool mIsTargetLocked = false;                                     // 0x1085
    bool mIsAimLocked = false;                                        // 0x1086
    s32 mEndDelayMinFrame = 60;                                       // 0x1088
    s32 mEndDelayFrames = 20;                                         // 0x108c
    s32 mEndDelay = 0;                                                // 0x1090
    s32 mSweepChargeFrames = 15;                                      // 0x1094
    s32 mSweepOffsetStart = 300;                                      // 0x1098
    s32 mSweepOffsetEnd = 1500;                                       // 0x109c
    f32 mEchoRadius = 600.0f;                                         // 0x10a0
    s32 mEchoFrames = 10;                                             // 0x10a4
    al::EffectSystem* mEffectSystem = nullptr;                        // 0x10a8
    al::LppLine* mLaserLight = nullptr;                               // 0x10b0
    f32 mMidBeamRate = 0.0f;                                          // 0x10b8
    f32 mPenetration = 0.0f;                                          // 0x10bc
    f32 mPenetrationRate = 0.1f;                                      // 0x10c0
    f32 mPlayerOnlySensorFrontOffset = 375.0f;                        // 0x10c4
    f32 mPlayerOnlySensorUpOffset = -150.0f;                          // 0x10c8
    bool mIsLaserEnding = false;                                      // 0x10cc
    s32 mEndFrame = 0;                                                // 0x10d0
    bool mIsShooting = false;                                         // 0x10d4
    f32 mAimSpeedMax = 65.0f;                                         // 0x10d8
    f32 mAimSpeedMaxV2 = 1000.0f;                                     // 0x10dc
    sead::Vector3f mLineCheckStart = sead::Vector3f::zero;            // 0x10e0
    DarkBowserLaserIndicator mIndicator{"DarkBowserLaserIndicator"};  // 0x10f0
    bool mIsEnableLaserLightEffects = true;                           // 0x1258
    SePlayObj* mSeTip = nullptr;                                      // 0x1260
    SePlayObj* mSeNear = nullptr;                                     // 0x1268
};
static_assert(sizeof(SuperBowserLaserState) == 0x1270);

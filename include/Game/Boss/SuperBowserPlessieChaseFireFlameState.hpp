#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"

namespace al {
class ActorInitInfo;
class SceneObjHolder;
}  // namespace al

class SePlayObj;
class SuperBowser;
class SuperBowserFireFlamePlessieChase;

/**
 * @brief Tuning of one fireball sweep of the Plessie chase, read from the
 * "InitAttackSuperBowserLastPhase3" init file.
 */
struct SuperBowserPlessieChaseFireFlameSweepParam {
    s32 shootInterval = 10;                        // 0x0
    s32 shootFrames = 45;                          // 0x4
    f32 shootAngle = 20.0f;                        // 0x8
    f32 shootTargetYOffset = 800.0f;               // 0xc
    bool isUseFixedTarget = false;                 // 0x10
    sead::Vector3f fixedTarget = {0.0f, 0.0f, 0.0f};  // 0x14
};
static_assert(sizeof(SuperBowserPlessieChaseFireFlameSweepParam) == 0x20);

/**
 * @brief Tuning of the fireball attacks of the Plessie chase, read from the
 * "InitAttackSuperBowserLastPhase3" init file.
 */
struct SuperBowserPlessieChaseFireFlameParam {
    f32 flameEffectScale = 1.0f;                                   // 0x0
    f32 flameSpeed = 180.0f;                                       // 0x4
    f32 shootOriginOffset = 2500.0f;                               // 0x8
    f32 disappearOffset = 0.0f;                                    // 0xc
    f32 disappearOffsetSweep = 0.0f;                               // 0x10
    s32 sweepShootDelayFrames = 240;                               // 0x14
    sead::Vector3f sweepFirstShotTarget = {-36500.0f, 3800.0f, -9000.0f};  // 0x18
    s32 ringShootInterval = 1;                                     // 0x24
    s32 ringFrames = 8;                                            // 0x28
    s32 ringFramesV2First = 5;                                     // 0x2c
    f32 ringShootTargetYOffset = 200.0f;                           // 0x30
    f32 ringShootTargetYOffsetTierJump = 400.0f;                   // 0x34
    f32 ringPlayerLead = 50.0f;                                    // 0x38
    f32 ringPlayerLeadTierJump = 0.0f;                             // 0x3c
    f32 ringScale = 0.15f;                                         // 0x40
    f32 ringScaleOffsetRange = 0.0f;                               // 0x44
    f32 ringSoundObjectSpeed = 180.0f;                             // 0x48
    f32 ringRotationOffsetV2First = 0.0f;                          // 0x4c
};
static_assert(sizeof(SuperBowserPlessieChaseFireFlameParam) == 0x50);

/**
 * @brief Fireball attacks of Fury Bowser during the Plessie chase: sweeps of fireballs across
 * the player's path and rings of fireballs aimed at the player.
 */
class SuperBowserPlessieChaseFireFlameState : public al::HostStateBase<SuperBowser>,
                                              public al::IUseSceneObjHolder {
public:
    SuperBowserPlessieChaseFireFlameState(SuperBowser* pHost, const al::ActorInitInfo& rInfo);

    void createFireFlamePool(const al::ActorInitInfo& rInfo);
    void initFromYaml();
    al::SceneObjHolder* getSceneObjHolder() const override;
    void appear() override;
    virtual void forceKill();
    void exeWait();
    void exeSweepBegin();
    void calcSweepShootInfo();
    void startJointAim(sead::Vector3f target, s32 frames);
    void exeSweepShoot();
    void shootFireFlame(sead::Vector3f pos, sead::Vector3f dir, f32 disappearOffset,
                        bool isFirstShot);
    void exeSweepDelay();
    void exeRingBegin();
    void calcRingShootInfo();
    void exeRingShoot();
    void exeRingEnd();
    void control() override;
    void updateFlameSweepSoundActorPos();
    void startRingAttack();
    void startOneShotSweep();
    SuperBowserPlessieChaseFireFlameParam getParam() const;
    sead::Vector3f calcRingTargetPos() const;
    sead::Vector3f getPlayerVelocityXZ() const;
    void tryUpdateSoundObjectStop();
    void initSweepParam(SuperBowserPlessieChaseFireFlameSweepParam* pParam, const char* pKey);

private:
    using FlamePool = sead::FixedPtrArray<SuperBowserFireFlamePlessieChase, 32>;
    using SweepFlames = sead::FixedPtrArray<SuperBowserFireFlamePlessieChase, 4>;

    al::SceneObjHolder* mSceneObjHolder = nullptr;                          // 0x28
    FlamePool mFlames;                                                      // 0x30
    SweepFlames mSweepFlames;                                               // 0x140
    SuperBowserPlessieChaseFireFlameParam mParam;                           // 0x170
    const SuperBowserPlessieChaseFireFlameSweepParam* mSweepParam;          // 0x1c0
    SuperBowserPlessieChaseFireFlameSweepParam mSweepParamV1;               // 0x1c8
    SuperBowserPlessieChaseFireFlameSweepParam mSweepParamV2;               // 0x1e8
    SuperBowserPlessieChaseFireFlameSweepParam mSweepParamLv4BigRamp;       // 0x208
    sead::Vector3f mShootDir = sead::Vector3f::ez;                          // 0x228
    sead::Vector3f mSweepStartDir = sead::Vector3f::ez;                     // 0x234
    sead::Vector3f mSweepEndDir = sead::Vector3f::ez;                       // 0x240
    sead::Vector3f mShootOrigin = sead::Vector3f::zero;                     // 0x24c
    sead::Vector3f mRingOrigin = sead::Vector3f::zero;                      // 0x258
    sead::Vector3f mSoundDir = sead::Vector3f::zero;                        // 0x264
    bool mIsSweepReverse = false;                                           // 0x270
    s32 mSweepCount = 0;                                                    // 0x274
    bool mIsOneShot = false;                                                // 0x278
    SePlayObj* mRingSe;                                                     // 0x280
    bool mIsRingSePlaying = false;                                          // 0x288
    SePlayObj* mSweepSe;                                                    // 0x290
    bool mIsSweepSePlaying = false;                                         // 0x298
};
static_assert(sizeof(SuperBowserPlessieChaseFireFlameState) == 0x2a0);

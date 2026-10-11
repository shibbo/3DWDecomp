#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class HitSensor;
}  // namespace al

/**
 * @brief The Black Sun (Bowser's Fury): the shell floating over Lake Lapcat that slowly rises
 * through five steps while the peace timer counts down, then spins up and launches Fury Bowser.
 */
class SuperBowserShell : public al::LiveActor {
public:
    /** @brief Callback run when the shell spins up or finishes launching. */
    using LaunchCallback = void (*)(void*);

    explicit SuperBowserShell(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initFromYaml();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeGrowOverTime0();
    s32 calculatePhaseFrames(s32 phase) const;
    void recalculatePhaseChangeFrames();
    bool updateFreeze(bool isActive);
    void updateAmbientSpin();
    void updateCoGBaseHeightY(bool isLimitSpeed);
    void exeGrowOverTime1();
    void exeGrowOverTime2();
    void exeGrowOverTime3();
    void exeGrowOverTime4();
    void updateForeshadowSpin();
    void exeSpin();
    void rotateByDegrees(f32 degrees);
    void exeLaunch();
    void exeNoOp();
    void exeEarlyRise();
    void updateFloat();
    void forceHide();
    void preLaunch(LaunchCallback pCallback, void* pArg);
    void launch(LaunchCallback pCallback, void* pArg);
    void returnFromDisaster();
    void updateCoGJointPosition();
    void setCoGJointRotation(f32 degrees);
    void stopAllSe();
    void setRainFrames(s32 frames);
    s32 getRainFrames();
    void setCoGJointPosition(sead::Vector3f pos);
    void syncToDisasterTimer();
    s32 calculatePhase() const;
    void skipTo(s32 phase, bool isToChangePhase, bool isToAppear);
    void skipToNextStepStep();
    s32 getLeftoverFrames();
    s32 getWaitSkipFrames();
    s32 getStep4Frames();
    void tryDrawStateDebug();
    void toggleDrawDebugState();
    void setEffectsOn(bool isOn);

private:
    LaunchCallback mLaunchCallback = nullptr;
    void* mLaunchCallbackArg = nullptr;
    LaunchCallback mPreLaunchCallback = nullptr;
    void* mPreLaunchCallbackArg = nullptr;
    s32 mRainFrames = 1200;
    s32 mRumbleTimer = 0;
    f32 mAmbientSpinSpeed = 0.1f;
    f32 mForeshadowSpinSpeed = 2.0f;
    f32 mForeshadowSpinSpeedRain = 5.5f;
    f32 mForeshadowSpinSpeedCurrent = 0.0f;
    s32 mForeshadowSpinAccelFrames = 180;
    s32 mForeshadowSpinDecelFrames = 120;
    f32 mRevSpinSpeed = 0.0f;
    f32 mRevSpinAccel = 0.0f;
    s32 mRevSpinFrames = 100;
    s32 mRevSpinMaxSpeedFrame = 95;
    s32 mRevEndFrame = 101;
    s32 mLaunchTimer = 0;
    s32 mLaunchFrames = 120;
    bool mIsEmissionStarted = false;
    s32 mFloatFrames = 300;
    s32 mFloatFrame = 0;
    f32 mFloatHeight = 1000.0f;
    f32 mCoGRotation = 0.0f;
    sead::Vector3f mCoGJointPos = sead::Vector3f::zero;
    f32 mCoGBaseHeightY = 0.0f;
    f32 mCoGBaseHeightYStart = -25000.0f;
    f32 mCoGBaseHeightYEnd = 0.0f;
    f32 mCoGBaseHeightYMaxSpeed = 50.0f;
    f32 mCoGBaseHeightYEarlyRiseRate = 0.8f;
    f32 mFloatOffsetY = 0.0f;
    sead::Vector3f mModelOffset = sead::Vector3f::zero;
    s32 mAppearFrame = 0;
    s32 mChangePhase01Frame = 0;
    s32 mChangePhaseFrame = 0;
    s32 mLeftoverFrames = 0;
    s32 mWaitSkipFrames = 0;
    bool mIsSynced = false;
    bool mIsEarlyRiseNext = false;
    bool mIsEarlyRise = false;
    bool mIsDrawDebugState = false;
    s32 mStep0Frames = -1;
    s32 mStepFrames = -1;
    s32 mStep4Frames = -1;
};

static_assert(sizeof(SuperBowserShell) == 0x210);

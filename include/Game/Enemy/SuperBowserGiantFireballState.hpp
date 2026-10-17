#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <container/seadPtrArray.h>
namespace al { class ActorInitInfo; class LiveActor; }
class SuperBowser;
class KoopaFireBallGiant;
class SuperBowserRainFireballState;
/** @brief Controls Fury Bowser's pool of giant fireballs. */
class SuperBowserGiantFireballState : public al::HostStateBase<SuperBowser> {
public:
    struct Param {
        s32 mArcFrame = 90;
        f32 mArcHeight = 2500.0f;
        f32 mFallSpeed = 20.0f;
        f32 mFallHeight = 2500.0f;
        f32 mAngleRandomRange = 60.0f;
        f32 mDistanceRandomMin = 150.0f;
        f32 mDistanceRandomMax = 350.0f;
        f32 mShadowScale = 450.0f;
        s32 mShadowGrowTime = 50;
        f32 mMoveSpeed = 0.0f;
        f32 mTrackingRate = 60.0f;
        s32 mHomingFrame = 25;
        s32 mAimFrame = 0;
        s32 mStartEndDelay = 60;
        s32 mPrimaryDelay = 60;
        s32 mSecondaryDelay = 30;
        s32 mArcFrameStepMin = 12;
        s32 mArcFrameStepMax = 60;
        f32 mFlyScale = 1.1f;
        f32 mLandScale = 0.7f;
        f32 mTrackingRateRaidon = 150.0f;
        f32 mFallSpeedRaidon = 20.0f;
        f32 mFallHeightRaidon = 2500.0f;
    };
    SuperBowserGiantFireballState(SuperBowser* pHost, const al::ActorInitInfo& rInfo);
    ~SuperBowserGiantFireballState() override;
    bool update() override;
    virtual void forceKill();
    virtual void appear(int shots, bool secondary);
    void createFireballPool(const al::ActorInitInfo& rInfo);
    void initParam(const al::ActorInitInfo& rInfo);
    bool isShootNowAndForever();
    void exeWait();
    void exeShootForever();
    void shoot();
    void shootNow();
    const Param* getParam();
    void setModeAimOnce();
    void setModeContinuousTracking();
    void setModeVelocitySnapshot();
    void shootNowAndForever();
    bool checkFireballsDone();
    bool checkFireballsLanded();
    void setGiantFireballIDs(SuperBowserRainFireballState* pState, int start);
    int getShotsLeft();
    float getAngleOffset();
private:
    Param mParam;
    sead::FixedPtrArray<KoopaFireBallGiant, 16> mFireballs;
    al::LiveActor* mTarget = nullptr;
    int mShotsLeft = 0;
    int mShotIndex = 0;
    int mDelay = 0;
    float mAngleOffset = 0.0f;
};
static_assert(sizeof(SuperBowserGiantFireballState) == 0x128);

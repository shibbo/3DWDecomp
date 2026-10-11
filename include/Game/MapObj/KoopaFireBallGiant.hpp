#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Enemy/SuperBowserGiantFireballState.hpp"
#include "Library/LiveActor/LiveActor.hpp"

class PlayerActor;
class SuperBowser;
class SuperBowserRainFireballState;

/**
 * @brief Giant fireball spat by Fury Bowser: it arcs over the player, falls, and burns on the
 * ground for a while.
 */
class KoopaFireBallGiant : public al::LiveActor {
public:
    KoopaFireBallGiant(const char* pName, SuperBowser* pBowser);

    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void killOnLanding();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void kill() override;
    virtual void appear(const al::LiveActor* pTarget, s32 index);
    bool hasLanded();
    void killAfterCutscene();
    void setFireballID(SuperBowserRainFireballState* pRainState, s32 id);

    void exeWait();
    void exeArc();
    void updateTargetVelocity();
    void updateFallStartPosition();
    sead::Vector3f getArcPosition(f32 rate);
    bool isInCameraView();
    void exeMove();
    bool forceCheckKillArea(bool isCheckWater);
    void startVelocity();
    void updatePaused();
    sead::Vector3f getTrackingPosition() const;
    bool tryCheckKillArea(bool isCheckWater);
    void tryToLand();
    void exeLandStart();
    void tryConnectToGround();
    void updateConnection();
    void updateEchoPulse();
    void exeLand();
    void exeLandEnd();
    void exeLandOnWater();
    void exeKillWait();
    void exeKillLanding();
    bool checkGround();

private:
    bool isTargetRaidon() const;

    SuperBowser* mBowser;                                         // 0x148
    const SuperBowserGiantFireballState::Param* mParam = nullptr;  // 0x150
    f32 mAttackSensorRadius = 0.0f;                               // 0x158
    f32 mLandSensorRadius = 0.0f;                                 // 0x15c
    bool mIsPaused = false;                                       // 0x160
    const PlayerActor* mTarget = nullptr;                         // 0x168
    sead::Vector3f mTargetPrevTrans = sead::Vector3f::zero;       // 0x170
    sead::Vector3f mTargetVelocity = sead::Vector3f::zero;        // 0x17c
    sead::Vector3f mMoveVelocity = sead::Vector3f::zero;          // 0x188
    sead::Vector3f mFallStartPos = sead::Vector3f::zero;          // 0x194
    sead::Vector3f mShotStartPos = sead::Vector3f::zero;          // 0x1a0
    s32 mIndex = 0;                                               // 0x1ac
    f32 mEchoRadius = 0.0f;                                       // 0x1b0
    bool mIsOnEchoBlock = false;                                  // 0x1b4
    s32 mEchoTimer = 0;                                           // 0x1b8
    const sead::Matrix34f* mConnectedMtx = nullptr;               // 0x1c0
    sead::Matrix34f mConnectionLocalMtx = sead::Matrix34f::ident; // 0x1c8
    SuperBowserRainFireballState* mRainState = nullptr;           // 0x1f8
    s32 mFireballID = -1;                                         // 0x200
    bool mIsKillOnLand = false;                                   // 0x204
    al::LiveActor* mConnectedHost = nullptr;                      // 0x208
    f32 mRandomAngle = 0.0f;                                      // 0x210
    f32 mRandomDistance = 0.0f;                                   // 0x214
    bool mIsInCameraView = false;                                 // 0x218
    f32 mBodyEffectScale = 5.0f;                                  // 0x21c
};

static_assert(sizeof(KoopaFireBallGiant) == 0x220);

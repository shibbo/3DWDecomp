#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CollisionPartsFilterActor;
}

class SuperBowserRainFireballState;

/**
 * @brief Fireball spat by Bowser (and by fireball generators): it flies straight, burns on the
 * ground for a while and dies out. In Bowser's Fury it is also used for the fireball rain.
 */
class KoopaFireBall : public al::LiveActor {
public:
    /// How the fireball behaves when it hits the ground.
    enum class LandType : s32 {
        Normal = 0,     ///< Burn on the ground for 240 frames.
        Short = 1,      ///< Burn on the ground for 120 frames.
        VeryShort = 2,  ///< Burn on the ground for 30 frames.
        NoLand = 3,     ///< Fly through collision and vanish at the end of its life.
    };

    KoopaFireBall(const char* pName, bool isGiant, al::LiveActor* pHost);

    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void extinguish();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void kill() override;
    void appearAttack(const sead::Vector3f& rPos, const sead::Vector3f& rTarget, f32 speed);
    void appearSingleMode(const sead::Vector3f& rPos);
    void setActionName(const char* pActionName);
    void setFireballID(SuperBowserRainFireballState* pRainState, s32 id);

    void exeWait();
    void exeMove();
    bool checkGround();
    void exeLandStart();
    void tryConnectToGround();
    void updateConnection();
    void updateEchoPulse();
    void exeLand();
    void exeLandEnd();
    void exeLandOnWater();
    void exeExtinguish();
    void exeSpawnCheck();
    void getSpawnCheckArrow(sead::Vector3f* pPos, sead::Vector3f* pDir) const;

private:
    f32 mAttackSensorRadius = 0.0f;                               // 0x144
    f32 mLandSensorRadius = 0.0f;                                 // 0x148
    s32 mLifeFrame = 600;                                         // 0x14c
    bool mIsGiant;                                                // 0x150
    bool mIsExtinguishOnSlope = false;                            // 0x151
    bool mIsKillOnCollide = false;                                // 0x152
    bool mIsKoopaLastAction = false;                              // 0x153
    LandType mLandType = LandType::Normal;                        // 0x154
    const char* mActionName = "Shot";                             // 0x158
    al::LiveActor* mHost;                                         // 0x160
    al::CollisionPartsFilterActor* mCollisionFilter = nullptr;    // 0x168
    bool mIsSingleMode = false;                                   // 0x170
    sead::Vector3f mMoveVelocity = sead::Vector3f::zero;          // 0x174
    bool mIsPaused = false;                                       // 0x180
    f32 mShadowSize = 100.0f;                                     // 0x184
    bool mIsLandingOnWater = false;                               // 0x188
    f32 mEchoRadius = 0.0f;                                       // 0x18c
    const sead::Matrix34f* mConnectedMtx = nullptr;               // 0x190
    sead::Matrix34f mConnectionLocalMtx = sead::Matrix34f::ident; // 0x198
    SuperBowserRainFireballState* mRainState = nullptr;           // 0x1c8
    s32 mFireballID = -1;                                         // 0x1d0
    bool mIsKillOnLand = false;                                   // 0x1d4
    al::LiveActor* mConnectedHost = nullptr;                      // 0x1d8
    s32 mSpawnCheckStep = 0;                                      // 0x1e0
    bool mIsOnEchoBlock = false;                                  // 0x1e4
    s32 mEchoTimer = 0;                                           // 0x1e8
};

static_assert(sizeof(KoopaFireBall) == 0x1f0);

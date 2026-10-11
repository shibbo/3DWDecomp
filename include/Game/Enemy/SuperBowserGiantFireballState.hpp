#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

/**
 * @brief Fury Bowser's state that rains giant fireballs (KoopaFireBallGiant) on the player.
 * @note Only the members used by reconstructed code are declared.
 */
class SuperBowserGiantFireballState : public al::ActorStateBase {
public:
    /// Tuning parameters shared by all giant fireballs of one attack.
    struct Param {
        s32 mArcFrame;              // 0x0
        f32 mArcHeight;             // 0x4
        f32 mFallSpeed;             // 0x8
        f32 mFallHeight;            // 0xc
        f32 mAngleRandomRange;      // 0x10
        f32 mDistanceRandomMin;     // 0x14
        f32 mDistanceRandomMax;     // 0x18
        u8 _1c[0x24 - 0x1c];
        f32 mMoveSpeed;             // 0x24
        f32 mTrackingRate;          // 0x28
        s32 mHomingFrame;           // 0x2c
        s32 mAimFrame;              // 0x30
        u8 _34[0x40 - 0x34];
        s32 mArcFrameStepMin;       // 0x40
        s32 mArcFrameStepMax;       // 0x44
        f32 mFlyScale;              // 0x48
        f32 mLandScale;             // 0x4c
        f32 mTrackingRateRaidon;    // 0x50
        f32 mFallSpeedRaidon;       // 0x54
        f32 mFallHeightRaidon;      // 0x58
    };

    const Param* getParam();
    bool isShootNowAndForever();
    f32 getAngleOffset();
};

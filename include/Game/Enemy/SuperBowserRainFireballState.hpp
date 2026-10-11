#pragma once

#include <basis/seadTypes.h>

/**
 * @brief Fury Bowser's state that rains fireballs around the player in a ring.
 * @note Only the members used by reconstructed code are declared.
 */
class SuperBowserRainFireballState {
public:
    /// Tuning parameters of the fireball rain.
    struct Param {
        u8 _0[0x14];
        f32 mFallSpeed;        // 0x14
        u8 _18[0x24];
        f32 mFlyScale;         // 0x3c
        f32 mLandScale;        // 0x40
        f32 mCheckHeight;      // 0x44
        s32 mCheckRingFrame;   // 0x48
    };

    const Param* getParam();
    bool canFireballDoCheck(s32 id);
    void getOutOfLine(s32 id);
};

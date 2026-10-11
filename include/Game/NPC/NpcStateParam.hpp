#pragma once

#include <basis/seadTypes.h>

/**
 * @brief Movement parameters shared by the NPC states (wander, chase, run away...).
 * @note The fields have not been reconstructed yet.
 */
class NpcStateParam {
public:
    NpcStateParam();
    NpcStateParam(f32 _0, f32 _4, f32 _8, f32 unused0, f32 unused1, f32 unused2, f32 unused3,
                  f32 _c, s32 _10, f32 _14);

    /** @return Drop height checked ahead of the NPC to avoid falling off cliffs. */
    f32 getFallCheckDrop() const { return mFallCheckDrop; }

    /** @return Gravity applied to the NPC. */
    f32 getGravity() const { return mGravity; }

private:
    f32 mGravity;  // 0x0
    u8 _4[0xc - 0x4];
    f32 mFallCheckDrop;  // 0xc
    u8 _10[0x18 - 0x10];
};

static_assert(sizeof(NpcStateParam) == 0x18);

/**
 * @brief Parameters of the NPC turn state: when an NPC turns to face a nearby player.
 */
class NpcStateTurnParam {
public:
    NpcStateTurnParam(f32 startTurnAngle, f32 endTurnAngle, f32 turnDegree, f32 searchRadius,
                      bool isEnableTurn, bool isTurnOnlyWaitAfter, s32 turnEndStep);

    f32 mStartTurnAngle;        // 0x0
    f32 mEndTurnAngle;          // 0x4
    f32 mTurnDegree;            // 0x8
    f32 mSearchRadius;          // 0xc
    bool mIsEnableTurn;         // 0x10
    bool mIsTurnOnlyWaitAfter;  // 0x11
    s32 mTurnEndStep;           // 0x14
};

static_assert(sizeof(NpcStateTurnParam) == 0x18);

/**
 * @brief Parameters of the NPC rumble (squash when trampled) reaction.
 */
class NpcStateRumbleParam {
public:
    NpcStateRumbleParam(s32 frames, f32 frequency, f32 angleOffset, f32 amplitude, f32 baseScale);

    s32 mFrames;        // 0x0
    f32 mFrequency;     // 0x4
    f32 mAngleOffset;   // 0x8
    f32 mAmplitude;     // 0xc
    f32 mBaseScale;     // 0x10
};

static_assert(sizeof(NpcStateRumbleParam) == 0x14);

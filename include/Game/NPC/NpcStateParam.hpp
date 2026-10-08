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
 * @brief Parameters of the NPC turn state.
 * @note The fields have not been reconstructed yet.
 */
class NpcStateTurnParam {
public:
    NpcStateTurnParam(f32 _0, f32 _4, f32 _8, f32 _c, bool _10, bool _11, s32 _14);

private:
    u8 _0[0x18];
};

static_assert(sizeof(NpcStateTurnParam) == 0x18);

/**
 * @brief Parameters of the NPC rumble state.
 * @note The fields have not been reconstructed yet.
 */
class NpcStateRumbleParam {
public:
    NpcStateRumbleParam(s32 _0, f32 _4, f32 _8, f32 _c, f32 _10);

private:
    u8 _0[0x14];
};

static_assert(sizeof(NpcStateRumbleParam) == 0x14);

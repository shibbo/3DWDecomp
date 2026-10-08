#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

class NpcStateParam;
class NpcTargetFinder;

/**
 * @brief Parameters of the NPC run away state.
 * @note Only the fields used by reconstructed code are named.
 */
class NpcStateRunAwayParam {
public:
    NpcStateRunAwayParam();
    NpcStateRunAwayParam(f32 runAccel, s32 _4, f32 _8, f32 _c, f32 turnDegree, f32 _14, f32 _18,
                         bool _1c, bool isEnableCliffCheck, bool isEnableShoreCheck,
                         const char* pRunActionName, const char* pWaitActionName, f32 _30,
                         s32 _34);

    /** @return Maximum turn per step, in degrees. */
    f32 getTurnDegree() const { return mTurnDegree; }

private:
    u8 _0[0x10];
    f32 mTurnDegree;  // 0x10
    alignas(8) u8 _18[0xa0 - 0x18];
};

/**
 * @brief NPC state that runs away from the target of a target finder.
 * @note Only what reconstructed code needs is declared so far.
 */
class NpcStateRunAway : public al::ActorStateBase {
public:
    NpcStateRunAway(al::LiveActor* pHost, sead::Vector3f* pFront, NpcTargetFinder* pTargetFinder,
                    const NpcStateParam* pParam, const NpcStateRunAwayParam* pRunAwayParam,
                    bool _78, bool* _80);

    /** @return Position the NPC runs away from. */
    const sead::Vector3f& getThreatPos() const { return mThreatPos; }

    /** @return Whether the NPC runs straight ahead instead of away from the threat. */
    bool isRunStraight() const { return mIsRunStraight; }

private:
    u8 _20[0x28 - 0x20];
    sead::Vector3f mThreatPos;  // 0x28
    u8 _34[0x7c - 0x34];
    bool mIsRunStraight;  // 0x7c
    u8 _7d[0x80 - 0x7d];
};

static_assert(sizeof(NpcStateRunAway) == 0x80);

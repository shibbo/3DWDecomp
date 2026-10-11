#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

class NpcStateParam;

/**
 * @brief Parameters of the NPC wander state.
 * @note Only the fields used by reconstructed code are named.
 */
class NpcStateWanderParam {
public:
    NpcStateWanderParam(s32 _0, s32 _4, f32 walkAccel, f32 turnDegree, f32 _10, f32 _14, f32 _18,
                        f32 _1c, f32 _20, bool isEnableCliffCheck, bool isEnableShoreCheck,
                        const char* pWalkActionName, const char* pWaitActionName, bool _a0,
                        s32 _a4, f32 _a8);

    /** @return Acceleration applied while walking. */
    f32 getWalkAccel() const { return mWalkAccel; }

    /** @return Maximum turn per step, in degrees. */
    f32 getTurnDegree() const { return mTurnDegree; }

private:
    u8 _0[0x8];
    f32 mWalkAccel;  // 0x8
    f32 mTurnDegree;  // 0xc
    alignas(8) u8 _10[0xa8 - 0x10];
};

static_assert(sizeof(NpcStateWanderParam) == 0xa8);

/**
 * @brief NPC state that wanders around a center point.
 * @note Only what reconstructed code needs is declared so far.
 */
class NpcStateWander : public al::ActorStateBase {
public:
    NpcStateWander(al::LiveActor* pHost, sead::Vector3f* pFront, const NpcStateParam* pParam,
                   const NpcStateWanderParam* pWanderParam);

    sead::Vector3f getWanderCenter() const;
    sead::Vector3f getTargetPos() const;

private:
    u8 _20[0x48 - 0x20];
};

static_assert(sizeof(NpcStateWander) == 0x48);

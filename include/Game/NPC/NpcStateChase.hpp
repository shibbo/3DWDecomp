#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

class NpcStateParam;
class NpcTargetFinder;

/**
 * @brief Parameters of the NPC chase state.
 * @note Only the fields used by reconstructed code are named.
 */
class NpcStateChaseParam {
public:
    NpcStateChaseParam(f32 runAccel, f32 _4, f32 _8, f32 turnDegree, f32 _10, bool _14,
                       bool isEnableCliffCheck, bool isEnableShoreCheck,
                       const char* pRunActionName, const char* pWaitActionName, f32 chaseRange);

    /** @return Acceleration applied while running. */
    f32 getRunAccel() const { return mRunAccel; }

    /** @return Maximum turn per step, in degrees. */
    f32 getTurnDegree() const { return mTurnDegree; }

    /** @return Whether the NPC avoids running off cliffs. */
    bool isEnableCliffCheck() const { return mIsEnableCliffCheck; }

private:
    f32 mRunAccel;  // 0x0
    u8 _4[0x8];
    f32 mTurnDegree;  // 0xc
    u8 _10[0x15 - 0x10];
    bool mIsEnableCliffCheck;  // 0x15
    u8 _16[0x18 - 0x16];
    alignas(8) u8 _18[0x90 - 0x18];
};

static_assert(sizeof(NpcStateChaseParam) == 0x90);

/**
 * @brief NPC state that chases the target of a target finder.
 * @note Only what reconstructed code needs is declared so far.
 */
class NpcStateChase : public al::ActorStateBase {
public:
    NpcStateChase(al::LiveActor* pHost, sead::Vector3f* pFront, NpcTargetFinder* pTargetFinder,
                  const NpcStateParam* pParam, const NpcStateChaseParam* pChaseParam,
                  bool _48, bool* _50);

private:
    u8 _20[0x58 - 0x20];
};

static_assert(sizeof(NpcStateChase) == 0x58);

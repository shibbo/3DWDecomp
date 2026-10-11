#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;
class TargetFinder;
class WalkerStateJumpParam;

/** @brief Tuning for WalkerStateFindPlayer (turn time and action). */
class WalkerStateFindPlayerParam {
public:
    WalkerStateFindPlayerParam();
    WalkerStateFindPlayerParam(s32 turnTime, f32 turnDegree, bool isJump, const char* pTurnAction);

    s32 mTurnTime;    // 0x00
    f32 mTurnDegree;  // 0x04

private:
    alignas(8) u8 _8[0x40];
};

static_assert(sizeof(WalkerStateFindPlayerParam) == 0x48);

/** @brief Walker state that turns toward a newly found player. */
class WalkerStateFindPlayer : public al::ActorStateBase {
public:
    WalkerStateFindPlayer(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
                          const TargetFinder* pTargetFinder, const WalkerStateParam* pParam,
                          const WalkerStateFindPlayerParam* pFindParam,
                          const WalkerStateJumpParam* pJumpParam);

private:
    u8 _20[0x30];
};

static_assert(sizeof(WalkerStateFindPlayer) == 0x50);

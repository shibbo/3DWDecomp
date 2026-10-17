#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;
class TargetFinder;
class WalkerStateJumpParam;
class WalkerStateJump;

/** @brief Tuning for WalkerStateFindPlayer (turn time and action). */
class WalkerStateFindPlayerParam {
public:
    WalkerStateFindPlayerParam();
    WalkerStateFindPlayerParam(s32 turnTime, f32 turnDegree, bool isJump, const char* pTurnAction);

    s32 mTurnTime;    // 0x00
    f32 mTurnDegree;  // 0x04

    bool mIsJump;
    sead::FixedSafeString<32> mTurnAction;
};

static_assert(sizeof(WalkerStateFindPlayerParam) == 0x48);

/** @brief Walker state that turns toward a newly found player. */
class WalkerStateFindPlayer : public al::ActorStateBase {
public:
    WalkerStateFindPlayer(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
                          const TargetFinder* pTargetFinder, const WalkerStateParam* pParam,
                          const WalkerStateFindPlayerParam* pFindParam,
                          const WalkerStateJumpParam* pJumpParam);
    /** @brief Destroys the player-detection state. */
    ~WalkerStateFindPlayer() override = default;
    void appear() override;
    void exeFind();
    void exeFindJump();
    void exeJump();

private:
    sead::Vector3f* mFrontDir;
    const TargetFinder* mTargetFinder;
    const WalkerStateParam* mParam;
    const WalkerStateFindPlayerParam* mFindParam;
    void* mUnused;
    WalkerStateJump* mJumpState;
};

static_assert(sizeof(WalkerStateFindPlayer) == 0x50);

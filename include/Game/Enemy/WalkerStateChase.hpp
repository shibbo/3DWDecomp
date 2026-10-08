#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;
class TargetFinder;

/** @brief Tuning for WalkerStateChase (speeds, turning and actions). */
class WalkerStateChaseParam {
public:
    WalkerStateChaseParam();
    WalkerStateChaseParam(f32 accel, f32 turnDegree, f32 lostDistance, f32 runAnimRate,
                          f32 moveAnimRate, bool isKeepChase, bool isUseRunAction,
                          const char* pRunAction, const char* pWaitAction, f32 chaseTime);

    /**
     * @brief Gets the fourth constructor value, which hosts also use as a turn speed in degrees.
     * @return Turn speed in degrees.
     */
    f32 getRunAnimRate() const { return mRunAnimRate; }

    /**
     * @brief Gets the chase acceleration (first constructor value).
     * @return Acceleration per step.
     */
    f32 getAccel() const { return mAccel; }

private:
    alignas(8) f32 mAccel;  // 0x00
    u8 _4[0x8];
    f32 mRunAnimRate;  // 0x0C
    u8 _10[0x80];
};

static_assert(sizeof(WalkerStateChaseParam) == 0x90);

/** @brief Walker state that chases the target found by a TargetFinder. */
class WalkerStateChase : public al::ActorStateBase {
public:
    WalkerStateChase(al::LiveActor* pHost, sead::Vector3f* pFrontDir, TargetFinder* pTargetFinder,
                     const WalkerStateParam* pParam, const WalkerStateChaseParam* pChaseParam,
                     bool isEnableJump, bool* pIsJumping);

private:
    u8 _20[0x38];
};

static_assert(sizeof(WalkerStateChase) == 0x58);

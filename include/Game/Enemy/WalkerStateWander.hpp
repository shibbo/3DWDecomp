#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;
class TargetFinder;

/** @brief Tuning for WalkerStateWander (wander timing, speeds and actions). */
class WalkerStateWanderParam {
public:
    WalkerStateWanderParam();
    WalkerStateWanderParam(s32 waitTime, s32 walkTime, f32 accel, f32 turnRate, f32 range,
                           f32 searchRange, bool isUseTargetFinder, const char* pWalkAction,
                           const char* pWaitAction);

    s32 mWaitTime;     // 0x00
    s32 mWalkTime;     // 0x04
    f32 mAccel;        // 0x08
    f32 mTurnRate;     // 0x0C
    f32 mRange;        // 0x10
    f32 mSearchRange;  // 0x14

private:
    alignas(8) u8 _18[0x78];
};

static_assert(sizeof(WalkerStateWanderParam) == 0x90);

/** @brief Walker state that wanders around a center point. */
class WalkerStateWander : public al::ActorStateBase {
public:
    WalkerStateWander(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
                      const WalkerStateParam* pParam, const WalkerStateWanderParam* pWanderParam,
                      TargetFinder* pTargetFinder);

    void setWanderCenter(const sead::Vector3f& rCenter);
    bool isWait();

private:
    u8 _20[0x28];
};

static_assert(sizeof(WalkerStateWander) == 0x48);

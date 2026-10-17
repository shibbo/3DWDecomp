#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;
class TargetFinder;
class TerritoryMover;

/** @brief Tuning for WalkerStateWander (wander timing, speeds and actions). */
class WalkerStateWanderParam {
public:
    WalkerStateWanderParam();
    WalkerStateWanderParam(s32 waitTime, s32 walkTime, f32 accel, f32 turnRate, f32 range,
                           f32 searchRange, bool avoidLedges, const char* pWalkAction,
                           const char* pWaitAction);

    s32 mWaitTime;     // 0x00
    s32 mWalkTime;     // 0x04
    f32 mAccel;        // 0x08
    f32 mTurnRate;     // 0x0C
    f32 mRange;        // 0x10
    f32 mSearchRange;  // 0x14

    bool mAvoidLedges;
    sead::FixedSafeString<32> mWalkAction;
    sead::FixedSafeString<32> mWaitAction;
};

static_assert(sizeof(WalkerStateWanderParam) == 0x90);

/** @brief Walker state that wanders around a center point. */
class WalkerStateWander : public al::ActorStateBase {
public:
    WalkerStateWander(al::LiveActor* pHost, sead::Vector3f* pFrontDir,
                      const WalkerStateParam* pParam, const WalkerStateWanderParam* pWanderParam,
                      TargetFinder* pTargetFinder);
    /** @brief Destroys the wandering state. */
    ~WalkerStateWander() override = default;
    void appear() override;
    void exeWait();
    void exeWalk();

    void setWanderCenter(const sead::Vector3f& rCenter);
    bool isWait();

private:
    sead::Vector3f* mFrontDir;
    TerritoryMover* mTerritoryMover = nullptr;
    const WalkerStateParam* mParam;
    const WalkerStateWanderParam* mWanderParam;
    TargetFinder* mTargetFinder;
};

static_assert(sizeof(WalkerStateWander) == 0x48);

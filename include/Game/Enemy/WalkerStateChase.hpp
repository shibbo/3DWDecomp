#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/NerveStateBase.hpp"

struct WalkerStateParam;
class TargetFinder;

/** @brief Tuning for WalkerStateChase (speeds, turning and actions). */
class WalkerStateChaseParam {
public:
    WalkerStateChaseParam();
    WalkerStateChaseParam(f32 accel, f32 minTime, f32 maxTime, f32 turnDegree,
                          f32 endTime, bool isKeepChase, bool avoidLedges,
                          const char* pRunAction, const char* pWaitAction, f32 value88);

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

    f32 mAccel;  // 0x00
    s32 mMinTime;
    s32 mMaxTime;
    f32 mRunAnimRate;  // 0x0C
    s32 mEndTime;
    bool mIsKeepChase;
    bool mAvoidLedges;
    sead::FixedSafeString<32> mRunAction;
    sead::FixedSafeString<32> mWaitAction;
    f32 mValue88;
};

static_assert(sizeof(WalkerStateChaseParam) == 0x90);

/** @brief Walker state that chases the target found by a TargetFinder. */
class WalkerStateChase : public al::ActorStateBase {
public:
    WalkerStateChase(al::LiveActor* pHost, sead::Vector3f* pFrontDir, TargetFinder* pTargetFinder,
                     const WalkerStateParam* pParam, const WalkerStateChaseParam* pChaseParam,
                     bool endOnLostTarget, bool* pIsOnGround);
    /** @brief Destroys the chase state. */
    ~WalkerStateChase() override = default;
    void appear() override;
    void exeStart();
    void exeEnd();

private:
    sead::Vector3f* mFrontDir;
    const WalkerStateParam* mParam;
    const WalkerStateChaseParam* mChaseParam;
    bool mEndOnLostTarget;
    bool* mIsOnGround;
    s32 mLostTargetStep = 0;
    TargetFinder* mTargetFinder;
};

static_assert(sizeof(WalkerStateChase) == 0x58);

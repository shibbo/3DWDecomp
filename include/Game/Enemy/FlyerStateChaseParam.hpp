#pragma once
#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

/** @brief Settings for flying pursuit and its stopping phase. */
struct FlyerStateChaseParam {
    FlyerStateChaseParam();
    FlyerStateChaseParam(f32 accel, s32 chaseTime, f32 turnDegree, f32 endFriction,
                        s32 endTime, s32 accelTime, const char* pChaseAction, const char* pWaitAction);
    f32 mAccel;
    s32 mChaseTime;
    f32 mTurnDegree;
    f32 mEndFriction;
    s32 mEndTime;
    s32 mAccelTime;
    sead::FixedSafeString<32> mChaseAction;
    sead::FixedSafeString<32> mWaitAction;
};
static_assert(sizeof(FlyerStateChaseParam) == 0x88);

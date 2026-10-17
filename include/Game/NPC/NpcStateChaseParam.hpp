#pragma once

#include <prim/seadSafeString.h>

class NpcStateChaseParam {
public:
    /** @brief Gets chase acceleration. @return chase acceleration. */
    float getRunAccel() const { return mAcceleration; }
    /** @brief Gets the turn limit in degrees. @return the turn limit in degrees. */
    float getTurnDegree() const { return mTurnDegrees; }
    /** @brief Gets whether cliff avoidance is enabled. @return whether cliff avoidance is enabled. */
    bool isEnableCliffCheck() const { return mAvoidFalls; }
    NpcStateChaseParam();
    NpcStateChaseParam(float acceleration, float minFrames, float maxFrames,
                       float turnDegrees, float endFrames, bool isEndless,
                       bool avoidFalls, bool avoidAreas, const char* pChaseAction,
                       const char* pEndAction, float param88);

    float mAcceleration;
    int mMinFrames;
    int mMaxFrames;
    float mTurnDegrees;
    int mEndFrames;
    bool mIsEndless;
    bool mAvoidFalls;
    bool mAvoidAreas;
    sead::FixedSafeString<32> mChaseAction;
    sead::FixedSafeString<32> mEndAction;
    float mParam88;
};

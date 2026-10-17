#pragma once

#include <prim/seadSafeString.h>

class NpcStateRunAwayParam {
public:
    /** @brief Gets the turn limit in degrees. @return the turn limit in degrees. */
    float getTurnDegree() const { return mTurnDegrees; }
    NpcStateRunAwayParam();
    NpcStateRunAwayParam(float acceleration, int fastFrames, float param8, float maxFrames,
                         float turnDegrees, float randomTurnDegrees, float endFrames,
                         bool isEndless, bool avoidFalls, bool avoidAreas,
                         const char* pRunAction, const char* pEndAction, float param90,
                         int randomTurnInterval);

    float mAcceleration;
    int mFastFrames;
    float mParam8;
    int mMaxFrames;
    float mTurnDegrees;
    int mEndFrames;
    bool mIsEndless;
    bool mAvoidFalls;
    bool mAvoidAreas;
    sead::FixedSafeString<32> mRunAction;
    sead::FixedSafeString<32> mEndAction;
    float mParam90;
    float mRandomTurnDegrees;
    int mRandomTurnInterval;
};

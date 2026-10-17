#pragma once

class NpcTargetFinderParam {
public:
    /** @brief Gets the maximum chase distance. @return the maximum chase distance. */
    float getChaseRange() const { return mChaseDistance; }
    NpcTargetFinderParam();
    NpcTargetFinderParam(float sightDistance, float horizontalAngle, float verticalAngle,
                        unsigned int targetGraceFrames, float senseDistance, float maxHeight,
                        float maxDepth, float chaseDistance, bool param20,
                        unsigned int scanInterval);

    float mSightDistance;
    float mHorizontalAngle;
    float mVerticalAngle;
    unsigned int mTargetGraceFrames;
    float mSenseDistance;
    float mMaxHeight;
    float mMaxDepth;
    float mChaseDistance;
    bool mParam20;
    unsigned int mCandidateCapacity;
    unsigned int mScanInterval;
};

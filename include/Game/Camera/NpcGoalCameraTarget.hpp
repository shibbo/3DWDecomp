#pragma once

#include <math/seadVector.h>

namespace al {
class LiveActor;
}

class GoalCameraCollisionFilter;

/**
 * @brief Finds a camera position that shows both an NPC and the player for a goal cutscene.
 * @note Only what reconstructed code needs is declared so far.
 */
class NpcGoalCameraTarget {
public:
    NpcGoalCameraTarget(sead::Vector3f* pCameraAt, sead::Vector3f* pCameraPos);

    void setMinAlignmentAngle(f32 angle);
    void setIntervalAngle(f32 angle);
    void setPlayerMustBeVisible(bool isMustBeVisible);
    bool isActorsVisible(const al::LiveActor* pActor, const sead::Vector3f& rCameraPos,
                         const sead::Vector3f& rActorPos, const sead::Vector3f& rPlayerPos,
                         const GoalCameraCollisionFilter* pFilter) const;
    bool isSafeViewAngle(const al::LiveActor* pActor, const sead::Vector3f& rCameraPos,
                         const sead::Vector3f& rTarget,
                         const GoalCameraCollisionFilter* pFilter) const;
    f32 calcDistance(const al::LiveActor* pActor, f32 distance) const;
    bool calcSafeAngle(const sead::Vector3f& rActorPos, const sead::Vector3f& rPlayerPos,
                       const al::LiveActor* pActor, const sead::Vector3f* pLookAt, f32 distance,
                       const f32* pHeight) const;

private:
    sead::Vector3f* mCameraAt;
    sead::Vector3f* mCameraPos;
    f32 mMinAlignmentAngle;
    f32 mIntervalAngle;
    bool _18;
    bool mIsPlayerMustBeVisible;
};

static_assert(sizeof(NpcGoalCameraTarget) == 0x20);

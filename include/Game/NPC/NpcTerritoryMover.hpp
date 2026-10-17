#pragma once

#include <container/seadObjArray.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
}

class NpcTerritoryMover {
public:
    NpcTerritoryMover(const al::LiveActor* pActor, float radius, float spacing);
    void decideNextTargetPos();
    bool isReachedTarget(const al::LiveActor* pActor, float distance) const;
    void calculateTargets();
    /** @brief Stores the territory center. @param rPosition New center position. */
    void setCenter(const sead::Vector3f& rPosition) { mCenter.set(rPosition); }
    /** @brief Stores the next target. @param rPosition New target position. */
    void setTarget(const sead::Vector3f& rPosition) { mTargetPosition.set(rPosition); }
    /** @brief Gets the territory center. @return Current center position. */
    const sead::Vector3f& getCenter() const { return mCenter; }
    /** @brief Gets the current target. @return Current target position. */
    const sead::Vector3f& getTarget() const { return mTargetPosition; }

private:
    struct Target {
        sead::Vector3f mPosition;
        bool mIsValidated;
    };

    float mRadius;
    sead::Vector3f mCenter;
    sead::Vector3f mTargetPosition;
    float mSpacing;
    const al::LiveActor* mActor;
    sead::ObjArray<Target> mTargets;
};

#pragma once

#include "Project/Collision/CollisionPartsFilterBase.hpp"

namespace al {
class CollisionParts;
class LiveActor;
}  // namespace al

/**
 * @brief Collision filter of the goal cameras, ignoring the collision of some actors.
 */
class GoalCameraCollisionFilter : public al::CollisionPartsFilterBase {
public:
    GoalCameraCollisionFilter(const al::LiveActor* pActor);

    bool isInvalidParts(const al::CollisionParts& rParts) const override;

private:
    const al::LiveActor* mActor;
    bool _10;
};

static_assert(sizeof(GoalCameraCollisionFilter) == 0x18);

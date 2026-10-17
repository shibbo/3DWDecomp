#pragma once

#include <math/seadVector.h>

namespace al {
class CollisionParts;
}

class IUseCollisionPartsMtx;
struct PlayerProperty;

/// Moves the player along with the collision parts they stand on.
class PlayerGroundFollower {
public:
    PlayerGroundFollower(IUseCollisionPartsMtx* pCollisionPartsMtx);

    void followGround(PlayerProperty* pProperty, sead::Vector3f* pFollowVel,
                      sead::Vector3f* pFollowRotate, sead::Vector3f* pFollowFront,
                      const al::CollisionParts* pParts);

private:
    IUseCollisionPartsMtx* mCollisionPartsMtx;  // 0x0
};

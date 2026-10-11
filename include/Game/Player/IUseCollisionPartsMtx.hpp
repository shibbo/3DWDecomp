#pragma once

#include <math/seadMatrix.h>

namespace al {
class CollisionParts;
}

/// Remembers the base matrices of the collision parts the player stands on (to follow them).
class IUseCollisionPartsMtx {
public:
    virtual bool isRegistered(const al::CollisionParts* pParts) const = 0;
    virtual void registerParts(const al::CollisionParts* pParts) = 0;
    virtual const sead::Matrix34f* getPrevBaseMtx(const al::CollisionParts* pParts) const = 0;
    virtual const sead::Matrix34f* getBaseMtx(const al::CollisionParts* pParts) const = 0;
};

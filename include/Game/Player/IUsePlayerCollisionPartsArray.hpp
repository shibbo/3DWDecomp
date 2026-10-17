#pragma once

#include <container/seadPtrArray.h>

namespace al {
class CollisionParts;
}

using CollisionPartsArray = sead::ConstPtrArray<al::CollisionParts>;

/// The collision parts the player touched this frame, per side (implemented by PlayerCollider).
class IUsePlayerCollisionPartsArray {
public:
    virtual const CollisionPartsArray* getFloorPartsArray() const = 0;
    virtual const CollisionPartsArray* getCeilingPartsArray() const = 0;
    virtual const CollisionPartsArray* getFrontPartsArray() const = 0;
    virtual const CollisionPartsArray* getFrontHemispherePartsArray() const = 0;
    virtual const CollisionPartsArray* getBackHemispherePartsArray() const = 0;
};

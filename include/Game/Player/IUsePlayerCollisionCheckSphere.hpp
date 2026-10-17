#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class CollisionParts;
}

/// Checks a sphere against the map (implemented by PlayerCollisionCheckSphere).
class IUsePlayerCollisionCheckSphere {
public:
    virtual bool checkSphere(const sead::Vector3f& rPos, f32 radius) = 0;
    virtual u32 getNum() const = 0;
    virtual const sead::Vector3f& getPos(u32 index) const = 0;
    virtual const sead::Vector3f& getNormal(u32 index) const = 0;
    virtual f32 getDepth(u32 index) const = 0;
    virtual const sead::Vector3f& getMoveVec(u32 index) const = 0;
    virtual const al::CollisionParts* getCollisionParts(u32 index) const = 0;
    virtual const char* getMapCodeName(u32 index) const = 0;
    virtual const char* getWallCodeName(u32 index) const = 0;
    virtual const char* getMaterialCodeName(u32 index) const = 0;
};

#pragma once

#include <math/seadVector.h>

namespace al {
class CollisionParts;
class Triangle;
}

class ResultCheckArrow {
public:
    ResultCheckArrow(s32 capacity);
    void clear();
    bool append(const sead::Vector3f& rPos, const sead::Vector3f& rNormal,
                const al::CollisionParts* pParts, const al::Triangle& rTriangle);
    const al::Triangle* getTriangle(s32 index) const;

    u32 mCapacity;
    u32 mCount;
    sead::Vector3f* mPositions;
    sead::Vector3f* mNormals;
    const al::CollisionParts* mLastCollisionParts;
    const al::CollisionParts** mCollisionParts;
    al::Triangle* mTriangles;
};

static_assert(sizeof(ResultCheckArrow) == 0x30);

#pragma once

#include <math/seadVector.h>

namespace al {
class LiveActor;
}

class TerritoryMover {
public:
    TerritoryMover(float radius);
    void decideNextTargetPos(const al::LiveActor* pActor);
    bool isReachedTarget(const al::LiveActor* pActor, float distance) const;

    float mRadius;
    sead::Vector3f mCenter;
    sead::Vector3f mTargetPos;
};

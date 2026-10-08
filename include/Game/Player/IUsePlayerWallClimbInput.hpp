#pragma once

#include <math/seadVector.h>

/// The stick input projected onto the climbed wall (implemented by PlayerWallClimbInput).
class IUsePlayerWallClimbInput {
public:
    virtual void retainDirection() = 0;
    virtual const sead::Vector3f& getMoveVec() const = 0;
};

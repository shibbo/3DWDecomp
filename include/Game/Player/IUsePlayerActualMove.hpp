#pragma once

#include <math/seadVector.h>

/// How far the player really moved in the last frame (implemented by PlayerActualMove).
class IUsePlayerActualMove {
public:
    virtual const sead::Vector3f& getActualMove() const = 0;
};

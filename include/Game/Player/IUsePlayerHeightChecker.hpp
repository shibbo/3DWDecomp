#pragma once

#include <basis/seadTypes.h>

/// The player's height above the ground (implemented by PlayerHeightChecker).
class IUsePlayerHeightChecker {
public:
    virtual bool isAboveGround() const = 0;
    virtual f32 getHeight() const = 0;
};

#pragma once

#include <math/seadVector.h>

/// Where the player last wall jumped (implemented by PlayerWallJumpInfo).
class IUsePlayerWallJumpInfo {
public:
    virtual bool isWallJumping() const = 0;
    virtual bool isInAirAfterWallJump() const = 0;
    virtual const sead::Vector3f& getLastWallJumpPos() const = 0;
    virtual const sead::Vector3f& getLastWallNormal() const = 0;
    virtual const sead::Vector3f& getFirstWallJumpPos() const = 0;
    virtual const sead::Vector3f& getFirstWallJumpNormal() const = 0;
};

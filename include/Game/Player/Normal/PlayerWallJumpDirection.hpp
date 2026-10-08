#pragma once

#include <math/seadVector.h>

/// Tracks the direction the player jumps off the wall they cling to.
class PlayerWallJumpDirection {
public:
    void init();
    void update();
    void getWallJumpDirection(sead::Vector3f* pOut) const;
    void getWallNormal(sead::Vector3f* pOut) const;
};

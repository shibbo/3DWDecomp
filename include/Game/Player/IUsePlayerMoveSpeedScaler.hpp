#pragma once

#include <basis/seadTypes.h>

/// Scales the player's move speed (implemented by PlayerActor).
class IUsePlayerMoveSpeedScaler {
public:
    virtual f32 getSpeedScale() const = 0;
};

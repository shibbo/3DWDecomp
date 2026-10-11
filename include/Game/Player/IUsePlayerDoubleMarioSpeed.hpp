#pragma once

#include <basis/seadTypes.h>

/// Speed factor while the player is doubled by a Double Cherry (implemented by
/// PlayerDoubleMarioSpeedControl).
class IUsePlayerDoubleMarioSpeed {
public:
    virtual f32 getSpeedRate() const = 0;
};

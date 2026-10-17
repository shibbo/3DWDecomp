#pragma once

#include <basis/seadTypes.h>

/// The player's horizontal speed averaged over the last frames
/// (implemented by PlayerHorizontalSpeedAverage).
class IUsePlayerHorizontalSpeedAverage {
public:
    virtual void resetHorizontalSpeedAverage() = 0;
    virtual f32 getHorizontalSpeedAverage() const = 0;
};

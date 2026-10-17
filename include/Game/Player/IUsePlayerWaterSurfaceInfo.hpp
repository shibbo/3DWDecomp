#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

/// The water surface near the player (implemented by PlayerWaterSurfaceFinder).
class IUsePlayerWaterSurfaceInfo {
public:
    virtual bool isWaterSurfaceExist() const = 0;
    virtual f32 getWaterSurfaceHeight() const = 0;
    virtual sead::Vector3f getWaterSurfacePosition() const = 0;
};

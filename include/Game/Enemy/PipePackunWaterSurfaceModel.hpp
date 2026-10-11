#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Ripple model shown where a Pipe Piranha Plant's stem crosses the water surface. */
class PipePackunWaterSurfaceModel : public al::LiveActor {
public:
    explicit PipePackunWaterSurfaceModel(const char* pName);

    void setSurfaceCoord(f32 coord);
    void setCoord(f32 coord);

private:
    u8 _144[0x180 - 0x144];
};

static_assert(sizeof(PipePackunWaterSurfaceModel) == 0x180);

#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Ripple model shown where a Pipe Piranha Plant's stem crosses the water surface. */
class PipePackunWaterSurfaceModel : public al::LiveActor {
public:
    explicit PipePackunWaterSurfaceModel(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    const sead::Matrix34f* getBaseMtx() const override;
    void exeWait();
    void exeMove();
    void exeEnd();
    void setSurfaceCoord(f32 coord);
    void setCoord(f32 coord);

private:
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;
    f32 mSurfaceCoord = 0.0f;
    f32 mCoord = 0.0f;
};

static_assert(sizeof(PipePackunWaterSurfaceModel) == 0x180);

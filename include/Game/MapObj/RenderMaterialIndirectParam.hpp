#pragma once

#include <math/seadVector.h>

// Layout reconstructed from the ChameleonStateUtil interpolation routines.
struct RenderMaterialIndirectParam {
    float mBlendRate = 0.1f;
    float mIntensity = 0.0f;
    sead::Vector4f mVector0{1.0f, 1.0f, 1.0f, 0.0f};
    sead::Vector4f mVector1{1.0f, 1.0f, 1.0f, 0.0f};
    sead::Vector4f mVector2{1.0f, 1.0f, 1.0f, 1.0f};
};

static_assert(sizeof(RenderMaterialIndirectParam) == 0x38);

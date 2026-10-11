#pragma once

#include <math/seadVector.h>
#include "utility/aglParameter.h"

#include "Library/Light/DirectionParam.hpp"

namespace al {
class PlaneParam : public DirectionParam {
public:
    PlaneParam() = default;

    void initialize(const sead::Vector3f& rDir, agl::utl::ParameterObj* pParamObj,
                    const char* pPlaneName);

    f32 getDistanceFromOrigin() const { return *mDistanceFromOrigin; }

private:
    agl::utl::Parameter<f32> mDistanceFromOrigin;
};

static_assert(sizeof(PlaneParam) == 0x38);

}  // namespace al

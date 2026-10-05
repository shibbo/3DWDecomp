#pragma once

#include <math/seadVector.h>

class WaterFlowAccessor {
public:
    virtual void calcSpeed(sead::Vector3f* pSpeed, const sead::Vector3f& rPosition) const;
};

#pragma once
#include <math/seadVector.h>

class TentackRockBase {
public:
    virtual void startFall(const sead::Vector3f& rPosition) = 0;
    virtual bool isDeadRock() const = 0;
};

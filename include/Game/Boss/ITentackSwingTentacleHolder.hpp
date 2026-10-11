#pragma once
#include <basis/seadTypes.h>

class TentackTentacle;

/** @brief Interface for owners that number the tentacles allowed to swing. */
class ITentackSwingTentacleHolder {
public:
    virtual s32 calcSwingTentacleId(const TentackTentacle* pTentacle) const = 0;
};

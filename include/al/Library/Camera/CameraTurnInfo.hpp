#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {

/**
 * @brief A request for the camera to turn towards a direction.
 * @note Only what reconstructed code needs is named so far.
 */
class CameraTurnInfo {
public:
    const char* mRequesterName;  // 0x0
    sead::Vector3f mDir;         // 0x8
    f32 _14;
    f32 _18;
    bool _1c;
    bool _1d;
};

static_assert(sizeof(CameraTurnInfo) == 0x20);

}  // namespace al

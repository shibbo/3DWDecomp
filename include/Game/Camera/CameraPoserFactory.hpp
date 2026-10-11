#pragma once

#include "Library/Camera/CameraPoserFactory_RS.hpp"

namespace al {

/**
 * Project camera poser factory registering the game's camera posers.
 */
class CameraPoserFactory : public CameraPoserFactory_RS {
public:
    CameraPoserFactory(const char* pName);
};

static_assert(sizeof(CameraPoserFactory) == 0x20);

}  // namespace al

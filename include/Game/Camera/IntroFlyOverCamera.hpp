#pragma once

#include <basis/seadTypes.h>
#include "Camera/FlyOverCamera.hpp"

/**
 * @brief Fly-over camera playing the intro of a phase.
 * @note Only what reconstructed code needs is declared so far.
 */
class IntroFlyOverCamera : public FlyOverCamera {
public:
    explicit IntroFlyOverCamera(const char* pName);

    void finalFinishCameraPlay(bool isSkip);

private:
    u8 mUnreconstructed[0x18];
};
static_assert(sizeof(IntroFlyOverCamera) == 0x1e8);

#pragma once

#include <basis/seadTypes.h>
#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Camera flying over the island of Bowser's Fury along a rail.
 * @note Only what reconstructed code needs is declared so far.
 */
class FlyOverCamera : public al::LiveActor {
public:
    explicit FlyOverCamera(const char* pName);

    virtual void finishInit();
    virtual bool isPaused() const;
    virtual void exeCameraPlay();
    virtual void finishCameraPlay();
    virtual void tryCancel();

    /**
     * Checks whether the camera finished playing.
     * @return Whether the camera play ended (flag at 0x16a).
     */
    bool isEndCameraPlay() const { return mUnreconstructed[0x16a - 0x144]; }

private:
    u8 mUnreconstructed[0x8c];
};
static_assert(sizeof(FlyOverCamera) == 0x1d0);

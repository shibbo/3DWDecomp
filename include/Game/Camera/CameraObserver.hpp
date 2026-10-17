#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
class PlayerHolder;
}  // namespace al

/**
 * @brief Watches the camera state shared with the camera director and shows its layouts.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CameraObserver {
public:
    CameraObserver(al::PlayerHolder* pPlayerHolder, bool* p0, bool* p1, s32* p2, bool* p3,
                   bool* p4, const al::LayoutInitInfo& rInfo);

    void update();

private:
    u8 _0[0x50];
};

static_assert(sizeof(CameraObserver) == 0x50);

#pragma once

#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"

namespace al {
class LiveActor;
class LayoutInitInfo;
}  // namespace al

/** @brief Name plate layout that follows a ghost player actor. */
class GhostMiiNameplate : public al::SimpleLayoutAppearWaitEnd {
public:
    GhostMiiNameplate(al::LiveActor* pActor, const al::LayoutInitInfo& rInfo);

    void init(const char16_t* pName, bool isMii, bool isTimeAttack);

private:
    u8 _128[0x140 - 0x128];
};

namespace GhostMiiNameplateFunction {
void updateShowHideFromCameraDistance(GhostMiiNameplate* pNameplate, bool isClipped);
}  // namespace GhostMiiNameplateFunction

#pragma once
#include "Library/Scene/Scene.hpp"
class StageWipeKeeper;
/** @brief Bowser's Fury title scene, partially reconstructed. */
class SingleModeTitleScene : public al::Scene {
public:
    explicit SingleModeTitleScene(StageWipeKeeper* pWipeKeeper);
    bool isReturnToTopMenu() const;
private:
    unsigned char mUnreconstructedE8[0x30];
};
static_assert(sizeof(SingleModeTitleScene) == 0x118);

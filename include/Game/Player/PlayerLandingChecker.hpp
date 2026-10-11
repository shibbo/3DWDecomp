#pragma once

#include <basis/seadTypes.h>

class IUsePlayerCollision;

/// Detects the frame the player lands.
class PlayerLandingChecker {
public:
    PlayerLandingChecker(const IUsePlayerCollision* pCollision);
    void update();

private:
    const IUsePlayerCollision* mCollision;  // 0x0
    u8 _8[0x10 - 0x8];
};
static_assert(sizeof(PlayerLandingChecker) == 0x10);

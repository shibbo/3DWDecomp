#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Warning marker shown on the target while Fury Bowser charges his laser.
 * @note Only what reconstructed code needs is declared so far.
 */
class DarkBowserLaserIndicator : public al::LiveActor {
public:
    explicit DarkBowserLaserIndicator(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void fadeIn(const al::LiveActor* pHost, const al::LiveActor* pTarget, u32 frames);
    void fadeOut();
    void forceKill();

private:
    u8 mUnreconstructed[0x24];
};
static_assert(sizeof(DarkBowserLaserIndicator) == 0x168);

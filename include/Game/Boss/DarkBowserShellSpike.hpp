#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
}  // namespace al

class DarkBowser;

/**
 * @brief Fury Bowser's shell spike attack state.
 * @note Only what reconstructed code needs is declared so far.
 */
class DarkBowserShellSpike : public al::NerveStateBase {
public:
    DarkBowserShellSpike(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    void forceKill();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);

    /**
     * @brief Sets the attack level (how aggressive the attack is).
     * @param level Attack level.
     */
    void setLevel(s32 level) { mLevel = level; }

private:
    u8 _11[0x20 - 0x11];  // starts in the tail padding of al::NerveStateBase
    s32 mLevel;  // 0x20
    u8 _24[0xB8 - 0x24];
};
static_assert(sizeof(DarkBowserShellSpike) == 0xb8);

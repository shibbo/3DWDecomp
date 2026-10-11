#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class DarkBowser;

/**
 * @brief Fury Bowser's fire bomb attack state.
 * @note Only what reconstructed code needs is declared so far.
 */
class DarkBowserFirebomb : public al::NerveStateBase {
public:
    DarkBowserFirebomb(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    /**
     * @brief Sets the attack level (how aggressive the attack is).
     * @param level Attack level.
     */
    void setLevel(s32 level) { mLevel = level; }

private:
    u8 _11[0x78 - 0x11];  // starts in the tail padding of al::NerveStateBase
    s32 mLevel;  // 0x78
    u8 _7C[0x80 - 0x7C];
};
static_assert(sizeof(DarkBowserFirebomb) == 0x80);

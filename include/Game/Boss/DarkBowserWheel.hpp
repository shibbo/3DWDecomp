#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class DarkBowser;

/**
 * @brief Fury Bowser's rolling shell wheel attack state.
 * @note Only what reconstructed code needs is declared so far.
 */
class DarkBowserWheel : public al::NerveStateBase {
public:
    DarkBowserWheel(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    /**
     * @brief Sets the attack level (how aggressive the attack is).
     * @param level Attack level.
     */
    void setLevel(s32 level) { mLevel = level; }

private:
    u8 _11[0x44 - 0x11];  // starts in the tail padding of al::NerveStateBase
    s32 mLevel;  // 0x44
    u8 _48[0x68 - 0x48];
};
static_assert(sizeof(DarkBowserWheel) == 0x68);

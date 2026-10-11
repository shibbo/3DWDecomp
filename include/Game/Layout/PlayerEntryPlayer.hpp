#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief 3D player model that jumps in when a player decides a character in the entry window.
 * @note Only the members used by already-decompiled callers are declared.
 */
class PlayerEntryPlayer : public al::LiveActor {
public:
    void appearPlayer(s32 userId);
    void startDecision();
    void startJumpOut();
    bool isLand() const;
    bool isEndDecision() const;
    bool isDead() const;

    /** @brief Releases the model held back from its start-decision animation. */
    void requestStartDecision() { mIsWaitStartDecision = false; }

private:
    u8 _144[0x160 - 0x144];  // members start in al::LiveActor's tail padding
    bool mIsWaitStartDecision;
};

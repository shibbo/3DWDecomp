#pragma once
#include "Library/LiveActor/LiveActor.hpp"

/** @brief Runner interface used by its jump state. */
class Runner : public al::LiveActor {
public:
    bool trySetTransOnGround();
    bool isGrounding() const;
    /** @brief Tests whether to skip jump anticipation. @return Whether the jump can begin immediately. */
    bool isImmediateJump() const { return mIsImmediateJump; }
private:
    u8 _144[0x2f];
    bool mIsImmediateJump;
};

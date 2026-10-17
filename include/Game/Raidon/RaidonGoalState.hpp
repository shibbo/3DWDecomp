#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class RaidonBase;

/// Plessie stopping at the goal position.
class RaidonGoalState : public al::NerveStateBase {
public:
    RaidonGoalState(const char* pName, RaidonBase* pHost);

    /** @param isSwim Whether Plessie reaches the goal while swimming. */
    void setIsSwim(bool isSwim) { mIsSwim = isSwim; }

private:
    u8 _11[0x20 - 0x11];
    bool mIsSwim;  // 0x20
    u8 _21[0x30 - 0x21];
};

static_assert(sizeof(RaidonGoalState) == 0x30);

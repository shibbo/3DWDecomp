#pragma once

#include <basis/seadTypes.h>

/// The invincible (Super Star) dash state (implemented by PlayerInvincibleDash).
class IUsePlayerInvincibleDash {
public:
    virtual f32 getRate() const = 0;
    virtual bool isPossibleToPlayDashAnim() const = 0;
};

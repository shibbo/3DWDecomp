#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerLandingObserver.hpp"
#include "Player/IUsePlayerRaccoonDogFallTask.hpp"

/// The state of the tanooki slow fall, reset when the player lands.
class PlayerRaccoonDogFallTask : public IUsePlayerRaccoonDogFallTask,
                                 public IUsePlayerLandingObserver {
public:
    PlayerRaccoonDogFallTask();

    void setup() override;
    bool isFirstFalling() const override;
    bool isPossibleToDampVelocity() const override;
    void validateDamp() override;
    void invalidateDamp() override;
    void notifyLanding() override;

private:
    u8 _10[0x18 - 0x10];
};
static_assert(sizeof(PlayerRaccoonDogFallTask) == 0x18);

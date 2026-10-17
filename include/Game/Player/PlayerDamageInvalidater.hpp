#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerDamageInvalidCheck.hpp"

/// Keeps track of the reasons the player can't take damage right now.
class PlayerDamageInvalidater : public IUsePlayerDamageInvalidCheck {
public:
    PlayerDamageInvalidater();
    void update();

    void invalidateForInfinite() override;
    void validateForInfinite() override;
    void invalidateForStatue() override;
    void validateForStatue() override;
    void invalidateForHelp() override;
    void validateForHelp() override;
    void invalidateDamage(u32 frame) override;
    bool isInvalid() const override;
    void reset() override;
    bool isFlashValid() const override;
    void invalidateFlash() override;
    void validateFlash() override;
    bool isPipeInvalid() const override;
    void invalidateForPipe() override;
    void validateForPipe() override;
    bool isInvalidFrame() const override;
    bool isInfiniteValid() const override;

private:
    u8 _8[0x18 - 0x8];
};
static_assert(sizeof(PlayerDamageInvalidater) == 0x18);

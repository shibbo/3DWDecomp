#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerHorizontalSpeedAverage.hpp"

class IUsePlayerCollision;
class PlayerConstParam;
struct PlayerProperty;

/// Averages the player's horizontal speed over the last frames.
class PlayerHorizontalSpeedAverage : public IUsePlayerHorizontalSpeedAverage {
public:
    PlayerHorizontalSpeedAverage(const PlayerConstParam* pConstParam);
    void update();
    void recordSpeed();
    void calcAverage();

    void resetHorizontalSpeedAverage() override;
    f32 getHorizontalSpeedAverage() const override;

    void setProperty(const PlayerProperty* pProperty) { mProperty = pProperty; }

    void setCollision(const IUsePlayerCollision* pCollision) { mCollision = pCollision; }

private:
    const PlayerConstParam* mConstParam;  // 0x8
    const PlayerProperty* mProperty;  // 0x10
    const IUsePlayerCollision* mCollision;  // 0x18
    u8 _20[0xa8 - 0x20];
};
static_assert(sizeof(PlayerHorizontalSpeedAverage) == 0xa8);

#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerSizeTrigger.hpp"

class IUsePlayerCollisionSize;
class PlayerFigureDirector;

/// Detects the frame the player grows big.
class PlayerSizeTrigger : public IUsePlayerSizeTrigger {
public:
    PlayerSizeTrigger(const PlayerFigureDirector* pFigureDirector,
                      const IUsePlayerCollisionSize* pCollisionSize);
    void init();
    void update();

    bool isBigTrig() const override;

private:
    u8 _8[0x28 - 0x8];
};
static_assert(sizeof(PlayerSizeTrigger) == 0x28);

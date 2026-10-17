#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerForwardBent.hpp"

class IUsePlayerWaterFlowField;
class PlayerConstParam;
class PlayerFigureDirector;
struct PlayerProperty;

/// Bends the player forward while it runs fast.
class PlayerForwardBent : public IUsePlayerForwardBent {
public:
    PlayerForwardBent(PlayerProperty* pProperty, const IUsePlayerWaterFlowField* pWaterFlowField,
                      const PlayerFigureDirector* pFigureDirector,
                      const PlayerConstParam* pConstParam);
    void update();
    void updateRunningState();
    void updateBent();

    void invalidateForwardBent() override;
    void validateForwardBent() override;
    void forceClearForwardBend() override;
    void clearForwardBent() override;
    bool isBent() const override;

private:
    u8 _8[0x30 - 0x8];
};
static_assert(sizeof(PlayerForwardBent) == 0x30);

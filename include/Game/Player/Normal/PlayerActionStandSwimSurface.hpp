#pragma once

#include <basis/seadTypes.h>

#include "Player/PlayerAction.hpp"
#include "Player/PlayerDef.hpp"

class IUsePlayerCharaQuery;
class IUsePlayerCheckArea;
class IUsePlayerForwardBent;
class IUsePlayerWaterFlowField;
class IUsePlayerWaterSurfaceInfo;
class PlayerFigureDirector;
class PlayerTrigger;
struct PlayerActionArg;

/// The player's stand swim surface action: floating upright at the water surface, bobbing on it
/// and swimming along it.
class PlayerActionStandSwimSurface : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionStandSwimSurface, PlayerAction)

public:
    PlayerActionStandSwimSurface(const PlayerActionArg* pArg,
                                 const PlayerFigureDirector* pFigureDirector,
                                 const IUsePlayerWaterFlowField* pWaterFlowField,
                                 const IUsePlayerWaterSurfaceInfo* pWaterSurfaceInfo,
                                 IUsePlayerForwardBent* pForwardBent,
                                 const IUsePlayerCheckArea* pCheckArea,
                                 const IUsePlayerCharaQuery* pCharaQuery, PlayerTrigger* pTrigger);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    void updateCounter();
    void updateAnim(bool isInWaterNoSink);

private:
    void updateClimbFigure(EPlayerFigure figure);

protected:
    const PlayerActionArg* mArg;                          // 0x8
    const PlayerFigureDirector* mFigureDirector;          // 0x10
    const IUsePlayerWaterFlowField* mWaterFlowField;      // 0x18
    const IUsePlayerWaterSurfaceInfo* mWaterSurfaceInfo;  // 0x20
    const IUsePlayerCheckArea* mCheckArea;                // 0x28
    const IUsePlayerCharaQuery* mCharaQuery;              // 0x30
    IUsePlayerForwardBent* mForwardBent;                  // 0x38
    u32 mCounter = 0;                                     // 0x40, frames since the last paddle
    bool mIsClimbFigure = false;                          // 0x44, the player wears a cat suit
    PlayerTrigger* mTrigger;                              // 0x48
};

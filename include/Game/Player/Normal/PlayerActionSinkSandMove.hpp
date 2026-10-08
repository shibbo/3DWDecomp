#pragma once

#include <math/seadVector.h>

#include "Player/PlayerAction.hpp"
#include "Player/PlayerActionArg.hpp"

class PlayerConstParam;
class PlayerFigureDirector;

/// The player's walking through quicksand (a slower ground move without dashes).
class PlayerActionSinkSandMove : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionSinkSandMove, PlayerAction)

public:
    PlayerActionSinkSandMove(PlayerActionArg* pArg, const PlayerFigureDirector* pFigureDirector);

    void move() override;
    void update() override;
    void updateAnimRate(f32 speed, bool isBrake);
    void setup() override;
    void teardown() override;

private:
    /** @brief Gets the player's tuning values. */
    const PlayerConstParam* getConstParam() const { return mArg->mConstParam; }

    PlayerActionArg* mArg;                         // 0x8
    const PlayerFigureDirector* mFigureDirector;  // 0x10
    u32 mFastFrame = 0;                            // 0x18
    sead::Vector3f mFloorNormal = {0.0f, 0.0f, 0.0f};  // 0x1c
};

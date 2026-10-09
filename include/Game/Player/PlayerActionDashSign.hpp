#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerActionEnd.hpp"
#include "Player/PlayerAction.hpp"
#include "Player/PlayerActionArg.hpp"

class PlayerConstParam;

/// The player's dash sign: running on the spot before a dash, for a few animation loops.
class PlayerActionDashSign : public PlayerAction, public IUsePlayerActionEnd {
    SEAD_RTTI_OVERRIDE(PlayerActionDashSign, PlayerAction)

public:
    PlayerActionDashSign(PlayerActionArg* pArg, PlayerProperty* pProperty);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    /** @brief Whether the dash sign finished (loops used up or the stick released). */
    bool isEnd() const override { return mIsEnd; }

private:
    /** @brief Gets the player's tuning values. */
    const PlayerConstParam* getConstParam() const { return mArg->mConstParam; }

    PlayerActionArg* mArg;                             // 0x10
    PlayerProperty* mProperty;                         // 0x18
    bool mIsEnd = false;                               // 0x20
    u32 mLoopCount = 0;                                // 0x24
    f32 mAnimRate = 0.0f;                              // 0x28
    u32 mAnimFrame = 0;                                // 0x2c
    sead::Vector3f mFloorNormal = sead::Vector3f::zero;  // 0x30
};

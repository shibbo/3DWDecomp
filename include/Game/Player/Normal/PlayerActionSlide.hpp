#pragma once

#include <math/seadVector.h>

#include "Player/PlayerAction.hpp"

class IUsePlayerCollisionSize;
struct PlayerActionArg;

/// The player's slide action: sliding down a slope (or a "Slide" floor) on the belly.
class PlayerActionSlide : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionSlide, PlayerAction)

public:
    PlayerActionSlide(const PlayerActionArg* pArg, IUsePlayerCollisionSize* pCollisionSize);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    void calcVelocity();
    void calcModelOffsetMtx();
    void calcTilt();

private:
    const PlayerActionArg* mArg;                        // 0x8
    IUsePlayerCollisionSize* mCollisionSize;            // 0x10
    sead::Vector3f mDownward = {0.0f, 0.0f, 0.0f};      // 0x18, downhill direction of the floor
    sead::Vector3f mFloorNormal = {0.0f, 0.0f, 0.0f};   // 0x24
};

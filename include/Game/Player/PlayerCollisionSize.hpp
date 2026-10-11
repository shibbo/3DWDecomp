#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerCollisionSize.hpp"

class IUsePlayerCollision;

/// Switches the player's collision between its sizes (mini, super, squatting, climbing).
class PlayerCollisionSize : public IUsePlayerCollisionSize {
public:
    PlayerCollisionSize(IUsePlayerCollision* pCollision);

    void changeShort() override;
    void changeSuper() override;
    void squat() override;
    void standUp() override;
    void changeClimbShort() override;
    void changeClimbSuper() override;
    bool isShort() const override;

private:
    IUsePlayerCollision* mCollision;  // 0x8
    u8 _10[0x18 - 0x10];
};
static_assert(sizeof(PlayerCollisionSize) == 0x18);

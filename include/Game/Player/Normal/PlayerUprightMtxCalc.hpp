#pragma once

#include <basis/seadTypes.h>

class IUsePlayerCollision;
struct PlayerProperty;

/// Blends the player's up direction towards the floor normal (used while climbing).
class PlayerUprightMtxCalc {
public:
    PlayerUprightMtxCalc(const IUsePlayerCollision* pCollision, PlayerProperty* pProperty);

    void update(f32 blendRate);
    void clear();

private:
    unsigned char _0[0x20];
};

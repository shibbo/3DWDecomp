#pragma once

#include <basis/seadTypes.h>

class PlayerActionGraph;
struct PlayerProperty;

/// Ends the player's super dash when it stops.
class PlayerSuperDashResetter {
public:
    PlayerSuperDashResetter();
    void update(const PlayerProperty* pProperty, const PlayerActionGraph* pActionGraph);

private:
    u8 _0[0x10];
};
static_assert(sizeof(PlayerSuperDashResetter) == 0x10);

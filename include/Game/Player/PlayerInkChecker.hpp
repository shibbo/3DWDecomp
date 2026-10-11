#pragma once

#include <basis/seadTypes.h>

class PlayerActor;
class PlayerConstParam;
class PlayerSimpleFlag;
struct PlayerProperty;

/// Checks whether the player stands in ink.
class PlayerInkChecker {
public:
    PlayerInkChecker(const PlayerActor* pActor, const PlayerProperty* pProperty,
                     const PlayerConstParam* pConstParam, PlayerSimpleFlag* pFlag, bool isGigaValid);
    void update();
    bool isInInk() const;

private:
    u8 _0[0x48];
};
static_assert(sizeof(PlayerInkChecker) == 0x48);

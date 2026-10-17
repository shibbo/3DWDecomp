#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerCeilingCheck.hpp"

class IUsePlayerCollisionCheckSphereMove;
class PlayerConstParam;
class PlayerFigureDirector;
struct PlayerProperty;

/// Checks whether there is room above the player to stand up.
class PlayerCeilingCheck : public IUsePlayerCeilingCheck {
public:
    PlayerCeilingCheck(IUsePlayerCollisionCheckSphereMove* pSphereMove,
                       const PlayerProperty* pProperty, const PlayerFigureDirector* pFigureDirector,
                       const PlayerConstParam* pConstParam);
    void update();
    void clear();

    bool hasSpaceToStandUp() const override;
    bool hasSpaceToStandUpForBig() const override;
    s32 getSpaceLevel() const override;

private:
    u8 _8[0x38 - 0x8];
};
static_assert(sizeof(PlayerCeilingCheck) == 0x38);

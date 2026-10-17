#pragma once

#include <math/seadVector.h>

#include "Player/PlayerDef.hpp"

class PlayerActor;

/// Queries and figure changes on a player, used by PlayerGroup (implemented by PlayerActorAccessor).
class IUsePlayerActorAccessor {
public:
    virtual EPlayerChara getChara(const PlayerActor* pPlayer) const = 0;
    virtual EPlayerFigure getFigureType(const PlayerActor* pPlayer) const = 0;
    virtual void change(PlayerActor* pPlayer, EPlayerFigure figure) = 0;
    virtual void changeForce(PlayerActor* pPlayer, EPlayerFigure figure) = 0;
    virtual const sead::Vector3f& getTrans(const PlayerActor* pPlayer) const = 0;
    virtual bool isActive(const PlayerActor* pPlayer) const = 0;
};

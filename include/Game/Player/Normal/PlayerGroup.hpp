#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Player/PlayerDef.hpp"

namespace al {
class HitSensor;
}

class IUsePlayerActorAccessor;
class PlayerActor;

/// The players of a scene, including the copies of double cherries, grouped by character.
class PlayerGroup {
public:
    typedef sead::PtrArray<PlayerActor> PlayerArray;

    PlayerGroup();

    void init(int playerNum, IUsePlayerActorAccessor* pAccessor);
    void append(PlayerActor* pPlayer);
    void changeFigure(PlayerActor* pPlayer, EPlayerFigure figure);
    void changeFigureForce(PlayerActor* pPlayer, EPlayerFigure figure);
    void calcGroupCenterPos(sead::Vector3f* pPos, const PlayerActor* pPlayer) const;
    void calcGroupHeadPos(sead::Vector3f* pPos, const PlayerActor* pPlayer,
                          const sead::Vector3f& rOffset) const;
    bool isLast(const PlayerActor* pPlayer) const;
    void killAllExcept(PlayerActor* pPlayer);
    void killAllExceptWithScore(al::HitSensor* pSensor);
    void removeAllEquipOfAllDoubleMarioExceptWithScore(al::HitSensor* pSensor);
    int calcDoubleMarioTotalNum() const;

private:
    PlayerArray* mCharaPlayers;                  // 0x0, one array per EPlayerChara
    IUsePlayerActorAccessor* mAccessor;          // 0x8
};

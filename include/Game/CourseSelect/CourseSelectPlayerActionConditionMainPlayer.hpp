#pragma once
#include "Player/PlayerActionCondition.hpp"
class IUsePlayerCharaQuery;
class GameDataHolder;
class CourseSelectPlayerActor;
class CourseSelectPlayerActionConditionMainPlayer : public PlayerActionCondition {
public:
    CourseSelectPlayerActionConditionMainPlayer(const IUsePlayerCharaQuery* pQuery, GameDataHolder* pGameData, CourseSelectPlayerActor* pPlayer);
    bool check() override;
private:
    const IUsePlayerCharaQuery* mpQuery;
    GameDataHolder* mpGameData;
    CourseSelectPlayerActor* mpPlayer;
};
static_assert(sizeof(CourseSelectPlayerActionConditionMainPlayer) == 0x20);

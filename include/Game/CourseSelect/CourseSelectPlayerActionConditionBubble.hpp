#pragma once
#include "Player/PlayerActionCondition.hpp"
class GameDataHolder;
class CourseSelectPlayerActor;
namespace al { class PlayerHolder; }
class CourseSelectPlayerActionConditionBubble : public PlayerActionCondition {
public:
    CourseSelectPlayerActionConditionBubble(GameDataHolder* pGameData, al::PlayerHolder* pPlayers, int characterType, CourseSelectPlayerActor* pPlayer);
    bool check() override;
private:
    GameDataHolder* mpGameData;
    al::PlayerHolder* mpPlayers;
    int mCharacterType;
    CourseSelectPlayerActor* mpPlayer;
};
static_assert(sizeof(CourseSelectPlayerActionConditionBubble) == 0x28);

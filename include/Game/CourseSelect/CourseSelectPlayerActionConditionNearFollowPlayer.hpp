#pragma once

#include "Player/PlayerActionCondition.hpp"

class GameDataHolder;
namespace al {
class PlayerHolder;
}

class CourseSelectPlayerActionConditionNearFollowPlayer : public PlayerActionCondition {
public:
    CourseSelectPlayerActionConditionNearFollowPlayer(GameDataHolder* pGameData,
                                                     al::PlayerHolder* pPlayers, int characterType);
    bool check() override;

private:
    GameDataHolder* mpGameData;
    al::PlayerHolder* mpPlayers;
    int mCharacterType;
};

static_assert(sizeof(CourseSelectPlayerActionConditionNearFollowPlayer) == 0x20);

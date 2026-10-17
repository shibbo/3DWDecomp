#pragma once

#include "Player/PlayerActionCondition.hpp"
#include <math/seadVector.h>

class GameDataHolder;
namespace al {
class PlayerHolder;
}

class CourseSelectPlayerActionConditionBubbleEnd : public PlayerActionCondition {
public:
    CourseSelectPlayerActionConditionBubbleEnd(GameDataHolder* pGameData, al::PlayerHolder* pPlayers,
                                              int characterType, const sead::Vector3f* pReturnPos);
    bool check() override;

private:
    GameDataHolder* mpGameData;
    al::PlayerHolder* mpPlayers;
    int mCharacterType;
    const sead::Vector3f* mpReturnPos;
};

static_assert(sizeof(CourseSelectPlayerActionConditionBubbleEnd) == 0x28);

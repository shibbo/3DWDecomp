#pragma once
#include "Player/PlayerActionCondition.hpp"
class CourseSelectPlayerActor;
class CourseSelectPlayerActionConditionPush : public PlayerActionCondition {
public:
    explicit CourseSelectPlayerActionConditionPush(CourseSelectPlayerActor* pPlayer);
    bool check() override;
private:
    CourseSelectPlayerActor* mpPlayer;
};
static_assert(sizeof(CourseSelectPlayerActionConditionPush) == 0x10);

#include "CourseSelect/CourseSelectPlayerActionConditionPush.hpp"
#include "CourseSelect/CourseSelectPlayerActor.hpp"

/** @brief Watch the course-map player for a received push. */
CourseSelectPlayerActionConditionPush::CourseSelectPlayerActionConditionPush(CourseSelectPlayerActor* pPlayer)
    : mpPlayer(pPlayer) {}

/** @brief Allow the push action when the actor has received a push. */
bool CourseSelectPlayerActionConditionPush::check() {
    return mpPlayer->isReceivePush();
}

#include "CourseSelect/CourseSelectPlayerActionConditionMainPlayer.hpp"
#include "CourseSelect/CourseSelectPlayerActor.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"

/** @brief Keep the player and character context for the main-player condition. */
CourseSelectPlayerActionConditionMainPlayer::CourseSelectPlayerActionConditionMainPlayer(const IUsePlayerCharaQuery* pQuery, GameDataHolder* pGameData, CourseSelectPlayerActor* pPlayer)
    : mpQuery(pQuery), mpGameData(pGameData), mpPlayer(pPlayer) {}

/** @brief Test whether this actor is the course map's current main player. */
bool CourseSelectPlayerActionConditionMainPlayer::check() {
    return CourseSelectDirector::getCourseSelectDirector(mpPlayer)->getMainPlayerActor() == mpPlayer;
}

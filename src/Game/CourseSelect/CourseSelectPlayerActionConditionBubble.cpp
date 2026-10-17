#include "CourseSelect/CourseSelectPlayerActionConditionBubble.hpp"
#include "CourseSelect/CourseSelectPlayerActor.hpp"
#include "CourseSelect/CourseSelectPlayerFunction.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

/** @brief Watch the player's distance from its course-map follow target. */
CourseSelectPlayerActionConditionBubble::CourseSelectPlayerActionConditionBubble(GameDataHolder* pGameData, al::PlayerHolder* pPlayers, int characterType, CourseSelectPlayerActor* pPlayer)
    : mpGameData(pGameData), mpPlayers(pPlayers), mCharacterType(characterType), mpPlayer(pPlayer) {}

/** @brief Enter a bubble beyond 900 units, provided player input is valid. */
bool CourseSelectPlayerActionConditionBubble::check() {
    if (mpPlayer->getInput()->getInvalidationFrames() != 0) return false;
    int userId = rc::tryCalcControlUserIdByCharacterType(GameDataHolderAccessor(mpGameData), mCharacterType, false);
    if (userId < 0) return false;
    auto* pPlayer = rc::findPlayerActorFirstByUserId(mpPlayers, userId);
    auto* pFollowPlayer = CourseSelectPlayerFunction::tryFindFollowPlayer(pPlayer);
    if (!pFollowPlayer || pPlayer == pFollowPlayer) return false;
    return (al::getTrans(pFollowPlayer) - al::getTrans(pPlayer)).squaredLength() > 900.0f * 900.0f;
}

#include "CourseSelect/CourseSelectPlayerActionConditionNearFollowPlayer.hpp"
#include "CourseSelect/CourseSelectPlayerFunction.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

/**
 * @brief Create a condition checking the distance to a character's follow player.
 * @param pGameData Game data used to resolve the character's control user.
 * @param pPlayers Holder containing the player actors.
 * @param characterType Character whose player and follow target are checked.
 */
CourseSelectPlayerActionConditionNearFollowPlayer::CourseSelectPlayerActionConditionNearFollowPlayer(
    GameDataHolder* pGameData, al::PlayerHolder* pPlayers, int characterType)
    : mpGameData(pGameData), mpPlayers(pPlayers), mCharacterType(characterType) {}

/**
 * @brief Test whether the player is within 300 units of its follow target.
 * @return False for an inactive character; otherwise true if no distinct follow
 * target exists or the distance to that target is strictly less than 300 units.
 */
bool CourseSelectPlayerActionConditionNearFollowPlayer::check() {
    int userId = rc::tryCalcControlUserIdByCharacterType(GameDataHolderAccessor(mpGameData), mCharacterType, false);
    if (userId < 0) {
        return false;
    }
    auto* pPlayer = rc::findPlayerActorFirstByUserId(mpPlayers, userId);
    auto* pFollowPlayer = CourseSelectPlayerFunction::tryFindFollowPlayer(pPlayer);
    if (pFollowPlayer == nullptr || pPlayer == pFollowPlayer) {
        return true;
    }
    return (al::getTrans(pFollowPlayer) - al::getTrans(pPlayer)).squaredLength() < 300.0f * 300.0f;
}

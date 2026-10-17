#include "CourseSelect/CourseSelectPlayerActionConditionBubbleEnd.hpp"
#include "CourseSelect/CourseSelectPlayerFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

/**
 * @brief Create a condition that checks whether a bubbled player can return safely.
 * @param pGameData Game data used to resolve the character's control user.
 * @param pPlayers Holder containing the course-select players.
 * @param characterType Character whose bubble exit is checked.
 * @param pReturnPos Fallback position used without a distinct follow player; must remain valid.
 */
CourseSelectPlayerActionConditionBubbleEnd::CourseSelectPlayerActionConditionBubbleEnd(
    GameDataHolder* pGameData, al::PlayerHolder* pPlayers, int characterType, const sead::Vector3f* pReturnPos)
    : mpGameData(pGameData), mpPlayers(pPlayers), mCharacterType(characterType), mpReturnPos(pReturnPos) {}

/**
 * @brief Check return distance, ground support, and collision clearance.
 * @return Whether the active player is close enough to its return position and can leave the bubble.
 */
bool CourseSelectPlayerActionConditionBubbleEnd::check() {
    int userId = rc::tryCalcControlUserIdByCharacterType(GameDataHolderAccessor(mpGameData), mCharacterType, false);
    if (userId < 0) {
        return false;
    }
    auto* pPlayer = rc::findPlayerActorFirstByUserId(mpPlayers, userId);
    auto* pFollowPlayer = CourseSelectPlayerFunction::tryFindFollowPlayer(pPlayer);
    sead::Vector3f returnPos = pFollowPlayer != nullptr ? al::getTrans(pFollowPlayer) : *mpReturnPos;
    if (pPlayer == pFollowPlayer) {
        returnPos = *mpReturnPos;
    }
    if ((returnPos - al::getTrans(pPlayer)).squaredLength() > 200.0f * 200.0f) {
        return false;
    }

    {
        sead::Vector3f rayStart = al::getTrans(pPlayer);
        sead::Vector3f rayDir(0.0f, -610.0f, 0.0f);
        rayStart.y += 10.0f;
        sead::Vector3f hitPos;
        if (!alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, nullptr, rayStart, rayDir, nullptr, nullptr)) {
            return false;
        }
    }

    const auto& rPlayerPos = al::getTrans(pPlayer);
    sead::Vector3f rayStart = returnPos;
    sead::Vector3f rayEnd = al::getTrans(pPlayer);
    rayStart.y += 50.0f;
    rayEnd.y += 50.0f;
    sead::Vector3f rayDir = rayEnd - rayStart;
    sead::Vector3f hitPos;
    if (!al::isNearZero(rayDir, 0.001f) &&
        alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, nullptr, rayStart, rayDir, nullptr, nullptr)) {
        return false;
    }

    sead::Vector3f playerCenter = rPlayerPos;
    playerCenter.y += 50.0f;
    return alCollisionUtil::checkStrikeSphere(pPlayer, playerCenter, 35.0f, nullptr, nullptr) == 0;
}

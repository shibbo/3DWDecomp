#include "CourseSelect/CourseSelectPlayerFunction.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

/**
 * @brief Find the last eligible active player preceding this player in user order.
 * @param pActor Player whose follow target should be found.
 * @return The preceding living player outside a leave demo, or null if none exists.
 */
al::LiveActor* CourseSelectPlayerFunction::tryFindFollowPlayer(al::LiveActor* pActor) {
    int userId = rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(pActor), rc::getPlayerInputPort(pActor));
    int userIds[4];
    int userNum = rc::findActiveUserIdList(userIds, GameDataHolderAccessor(pActor));
    int followUserId = -1;
    for (int i = 0; i < userNum; i++) {
        if (userIds[i] == userId)
            break;
        if (!rc::isDeadControlUserInStage(GameDataHolderAccessor(pActor), userIds[i]) &&
            !CourseSelectDirector::getCourseSelectDirector(pActor)->isPlayLeaveDemo(userIds[i]))
            followUserId = userIds[i];
    }
    if (followUserId == -1)
        return nullptr;
    int port = rc::getControlUserPortNumber(GameDataHolderAccessor(pActor), followUserId);
    return rc::tryFindPlayerFromInputPort(pActor, port, false);
}

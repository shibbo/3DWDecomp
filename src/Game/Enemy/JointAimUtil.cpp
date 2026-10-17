#include "Enemy/JointAimUtil.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace JointAimUtil {
/** @brief Gradually aims the eyes toward a nearby active player.
 * @param pActor Actor searching for a player. @param pInfo Eye joint aim parameters.
 * @param radius Player search radius. @param rate Amount to adjust the aiming strength each update.
 */
void updateEyeJointInfo(al::LiveActor* pActor, al::JointAimInfo* pInfo, f32 radius, f32 rate) {
    al::LiveActor* pPlayer = rc::tryFindNearestActivePlayerActorInSphere(pActor, radius);
    if (pPlayer) {
        pInfo->setTargetPos(al::getTrans(pPlayer));
        pInfo->addPowerRate(rate);
    } else {
        pInfo->subPowerRate(rate);
    }
}
}

#include "Enemy/SyumockUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointSpringControllerHolder.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace SyumockUtil {
/** @brief Loads the eye joint's aiming limits. @param pActor Syumock actor. @return Created eye aiming parameters. */
al::JointAimInfo* createSyumockEyeJointInfo(al::LiveActor* pActor) {
    auto* pInfo = new al::JointAimInfo;
    pInfo->setBaseAimLocalDir(sead::Vector3f::ex);
    pInfo->setBaseUpLocalDir(sead::Vector3f::ey);
    pInfo->setBaseSideLocalDir(sead::Vector3f::ez);
    pInfo->setEnableBackAim(true);
    al::ByamlIter iter(al::getModelResourceYaml(pActor, "EyeAimInfo", nullptr));
    f32 left = 8.0f;
    f32 right = 8.0f;
    f32 up = 3.0f;
    f32 down = 0.0f;
    al::tryGetByamlF32(&left, iter, "LeftLimit");
    al::tryGetByamlF32(&right, iter, "RightLimit");
    al::tryGetByamlF32(&up, iter, "UpLimit");
    al::tryGetByamlF32(&down, iter, "DownLimit");
    pInfo->setLimitDegreeOval(left, right, up, down);
    f32 rate = 0.3f;
    al::tryGetByamlF32(&rate, iter, "InterpoleRate");
    pInfo->setInterpoleRate(rate);
    return pInfo;
}

/** @brief Creates the joint spring controllers. @param pActor Syumock actor. @return Created controller holder. */
al::JointSpringControllerHolder* initJointSyumockJointSpringController(al::LiveActor* pActor) {
    auto* pHolder = new al::JointSpringControllerHolder;
    pHolder->init(pActor, "JointSpringControllerInfo");
    return pHolder;
}

/** @brief Tests the Syumock attack category. @param pMsg Incoming message. @return Whether the message belongs to this category. */
bool isMsgSyumockDeath(const al::SensorMsg* pMsg) {
    return al::isMsgPlayerInvincibleAttack(pMsg) ||
           al::isMsgPlayerInvincibleTouch(pMsg) ||
           al::isMsgPlayerObjStatueDrop(pMsg) ||
           al::isMsgPlayerStatueTouch(pMsg) ||
           al::isMsgPlayerGiantAttack(pMsg);
}

/** @brief Tests the Syumock attack category. @param pMsg Incoming message. @return Whether the message belongs to this category. */
bool isMsgSyumockDeathSingleMode(const al::SensorMsg* pMsg) {
    return al::isMsgDisasterSpikeAttack(pMsg) ||
           al::isMsgEnemyAttackFire(pMsg) ||
           al::isMsgLaserAttack(pMsg) ||
           al::isMsgPlayerInvincibleAttack(pMsg) ||
           al::isMsgPlayerInvincibleTouch(pMsg) ||
           rc::isMsgBobsledBodyAttack(pMsg) ||
           al::isMsgPlayerGiantTouch(pMsg) ||
           al::isMsgExplosion(pMsg) ||
           al::isMsgPlayerObjStatueDrop(pMsg) ||
           al::isMsgPlayerStatueTouch(pMsg);
}

/** @brief Tests the Syumock attack category. @param pMsg Incoming message. @return Whether the message belongs to this category. */
bool isMsgSyumockPush(const al::SensorMsg* pMsg) {
    return al::isMsgKouraThrow(pMsg) ||
           al::isMsgNpcTouch(pMsg);
}

/** @brief Tests whether an attack can trigger a reaction. @param pMsg Incoming message. @param reactionStep Time since the previous reaction. @return Whether the actor should react. */
bool isMsgSyumockReaction(const al::SensorMsg* pMsg, s32 reactionStep) {
    if (al::isMsgPlayerFireBallAttack(pMsg)) { return true; }
    if (al::isMsgPlayerBoomerangBreak(pMsg)) { return true; }
    if (al::isMsgKillerAttack(pMsg)) { return true; }
    if (al::isMsgPlayerCooperationHipDrop(pMsg) ||
        al::isMsgPlayerGiantHipDrop(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg)) {
        return reactionStep > 30;
    }
    return false;
}

/** @brief Tests whether an attack can trigger a reaction. @param pMsg Incoming message. @param reactionStep Time since the previous reaction. @return Whether the actor should react. */
bool isMsgSyumockReactionSingleMode(const al::SensorMsg* pMsg, s32 reactionStep) {
    if (al::isMsgPlayerFireBallAttack(pMsg)) { return true; }
    if (al::isMsgPlayerBoomerangBreak(pMsg)) { return true; }
    if (al::isMsgBallTrample(pMsg)) { return true; }
    if (al::isMsgBallAttack(pMsg)) { return true; }
    if (al::isMsgKeyThrow(pMsg) ||
        al::isMsgKouraThrow(pMsg) ||
        al::isMsgKickKouraReflect(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) ||
        al::isMsgPlayerHipDropAll(pMsg)) {
        return reactionStep > 30;
    }
    return false;
}

/** @brief Awards the kill and drops the actor's item.
 * @param pActor Defeated Syumock. @param pMsg Attack message. @param pOther Attacker sensor. @param pSelf Syumock sensor.
 */
void doKill(al::LiveActor* pActor, const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    al::startHitReactionDeath(pActor);
    rc::addScoreCombo(pActor, pOther, pMsg, 0.0f);
    al::setAppearItemAttackerSensor(pActor, pOther);
    al::appearItemTiming(pActor, "\u6b7b\u4ea1");
    pActor->kill();
}
} // namespace SyumockUtil

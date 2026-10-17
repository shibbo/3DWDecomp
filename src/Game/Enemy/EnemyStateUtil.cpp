#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "MapObj/DoorKey.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace EnemyStateUtil {
/** @brief Checks messages that press an enemy down.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @return Whether the check or request succeeds.
 */
bool isMsgPressDownForCrossoverSensor(const al::SensorMsg* pMsg, const al::HitSensor* pOther, const al::HitSensor* pSelf) {
    return al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
           al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgBallTrample(pMsg);
}

/** @brief Requests a stomp reaction and optional item-drop attribution.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param isSetItemFactor Whether to record the attack for item drops.
 * @return Whether the check or request succeeds.
 */
bool tryRequestPressDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, bool isSetItemFactor) {
    if (!isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf)) {
        return false;
    }
    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    if (isSetItemFactor) {
        rc::setAppearItemFactorByMsg(al::getSensorHost(pSelf), pMsg, pOther);
    }
    return true;
}

/** @brief Accepts a stomp and enters the supplied state.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param pHost Actor receiving the state change.
 * @param pNextNerve State to enter after accepting the attack.
 * @param isSetItemFactor Whether to record the attack for item drops.
 * @return Whether the check or request succeeds.
 */
bool tryRequestPressDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, al::LiveActor* pHost, const al::Nerve* pNextNerve, bool isSetItemFactor) {
    if (tryRequestPressDown(pMsg, pOther, pSelf, isSetItemFactor)) {
        al::setNerve(pHost, pNextNerve);
        return true;
    }
    return false;
}

/** @brief Checks attacks that cause standard knockback.
 * @param pMsg Incoming attack message.
 * @return Whether the check or request succeeds.
 */
bool isMsgBlowDown(const al::SensorMsg* pMsg) {
    return al::isMsgBallAttack(pMsg) ||
           al::isMsgBallAttackHold(pMsg) ||
           al::isMsgBallAttackDRCHold(pMsg) ||
           al::isMsgBlockUpperPunch(pMsg) ||
           al::isMsgEnemyAttackFire(pMsg) ||
           al::isMsgEnemyAttackBoomerang(pMsg) ||
           al::isMsgExplosion(pMsg) ||
           al::isMsgKickKouraAttack(pMsg) ||
           al::isMsgKickStoneAttack(pMsg) ||
           al::isMsgKillerAttack(pMsg) ||
           al::isMsgPlayerBodyAttack(pMsg) ||
           al::isMsgPlayerBodyLanding(pMsg) ||
           al::isMsgPlayerBoomerangAttack(pMsg) ||
           al::isMsgPlayerClimbAttack(pMsg) ||
           al::isMsgPlayerCooperationHipDrop(pMsg) ||
           al::isMsgPlayerFireBallAttack(pMsg) ||
           al::isMsgPlayerGiantAttack(pMsg) ||
           al::isMsgPlayerGiantHipDrop(pMsg) ||
           al::isMsgPlayerInvincibleAttack(pMsg) ||
           al::isMsgPlayerKouraAttack(pMsg) ||
           al::isMsgPlayerSlidingAttack(pMsg) ||
           al::isMsgPlayerTailAttack(pMsg) ||
           al::isMsgPlayerSpinAttack(pMsg) ||
           rc::isMsgSkateShoesAttack(pMsg) ||
           al::isMsgLaserAttack(pMsg) ||
           al::isMsgDisasterSpikeAttack(pMsg) ||
           al::isMsgKeyThrow(pMsg) ||
           al::isMsgNekoAttack(pMsg);
}

/** @brief Checks knockback attacks accepted by spiked enemies.
 * @param pMsg Incoming attack message.
 * @return Whether the check or request succeeds.
 */
bool isMsgBlowDownForSpike(const al::SensorMsg* pMsg) {
    if (al::isMsgPlayerBodyAttack(pMsg)) {
        return false;
    }
    if (al::isMsgPlayerSlidingAttack(pMsg)) {
        return false;
    }
    return isMsgBlowDown(pMsg) || al::isMsgBallTrample(pMsg) || al::isMsgEnemyAttack(pMsg);
}

/** @brief Checks knockback attacks accepted by ghost enemies.
 * @param pMsg Incoming attack message.
 * @return Whether the check or request succeeds.
 */
bool isMsgBlowDownForGhost(const al::SensorMsg* pMsg) {
    if (al::isMsgBallAttackHold(pMsg)) {
        return false;
    }
    if (al::isMsgBallAttackDRCHold(pMsg)) {
        return false;
    }
    if (al::isMsgPlayerSlidingAttack(pMsg)) {
        return false;
    }
    return isMsgBlowDown(pMsg) || al::isMsgBallTrample(pMsg);
}

/** @brief Configures knockback when the attack is supported.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param pState Knockback state to configure.
 * @param isSetItemFactor Whether to record the attack for item drops.
 * @return Whether the check or request succeeds.
 */
bool tryRequestBlowDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, bool isSetItemFactor) {
    if (isMsgBlowDown(pMsg) || al::isMsgKeyThrow(pMsg)) {
        requestBlowDown(pMsg, pOther, pSelf, pState, isSetItemFactor);
        return true;
    }
    return false;
}

/** @brief Enables collision and configures the knockback reaction and launch direction.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param pState Knockback state to configure.
 * @param isSetItemFactor Whether to record the attack for item drops.
 */
void requestBlowDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, bool isSetItemFactor) {
    al::onCollide(pState->mHostActor);
    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    rc::startHitReactionBlowHitMessage(pMsg, pState->mHostActor, pOther, pSelf);
    if (isSetItemFactor) {
        rc::setAppearItemFactorByMsg(al::getSensorHost(pSelf), pMsg, pOther);
    }
    pState->setBlowDir(al::getSensorHost(pOther));
}

/** @brief Configures knockback from an enemy fire attack.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param pState Knockback state to configure.
 * @param isSetItemFactor Whether to record the attack for item drops.
 * @return Whether the check or request succeeds.
 */
bool tryRequestAttackFireBlowDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, bool isSetItemFactor) {
    if (al::isMsgEnemyAttackFire(pMsg)) {
        requestBlowDown(pMsg, pOther, pSelf, pState, isSetItemFactor);
        return true;
    }
    return false;
}

/** @brief Accepts knockback and enters the supplied host state.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param pState Knockback state to configure.
 * @param pNextNerve State to enter after accepting the attack.
 * @param isSetItemFactor Whether to record the attack for item drops.
 * @return Whether the check or request succeeds.
 */
bool tryRequestBlowDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, const al::Nerve* pNextNerve, bool isSetItemFactor) {
    if (tryRequestBlowDown(pMsg, pOther, pSelf, pState, isSetItemFactor)) {
        al::setNerve(pState->mHostActor, pNextNerve);
        return true;
    }
    return false;
}

/** @brief Accepts knockback and enters the supplied host state.
 * @param pMsg Incoming attack message.
 * @param pOther Attacking sensor.
 * @param pSelf Receiving sensor.
 * @param pState Knockback state to configure.
 * @param pNextNerve State to enter after accepting the attack.
 * @param isSetItemFactor Whether to record the attack for item drops.
 * @return Whether the check or request succeeds.
 */
bool tryRequestAttackFireBlowDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, const al::Nerve* pNextNerve, bool isSetItemFactor) {
    if (tryRequestAttackFireBlowDown(pMsg, pOther, pSelf, pState, isSetItemFactor)) {
        al::setNerve(pState->mHostActor, pNextNerve);
        return true;
    }
    return false;
}

/** @brief Checks attacks that can travel through clear pipes.
 * @param pMsg Incoming attack message.
 * @return Whether the check or request succeeds.
 */
bool isMsgRouteDokanAttack(const al::SensorMsg* pMsg) {
    return rc::isMsgRouteDokanKouraAttack(pMsg) || al::isMsgEnemyRouteDokanAttack(pMsg) ||
           al::isMsgEnemyRouteDokanFire(pMsg) || al::isMsgPlayerRouteDokanFireBallAttack(pMsg) ||
           al::isMsgBallRouteDokanAttack(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg);
}

/** @brief Checks lethal areas and collision materials.
 * @param pActor Actor to check.
 * @return Whether the check or request succeeds.
 */
bool isKillByAreaOrMaterialCode(const al::LiveActor* pActor) {
    return rc::isInDeathArea(pActor) || rc::isCollidedDamageFire(pActor) ||
           rc::isCollidedPoison(pActor) || rc::isCollidedInkSlow(pActor);
}

/** @brief Removes an actor touching a lethal area or material.
 * @param pActor Actor to check.
 * @return Whether the check or request succeeds.
 */
bool tryKillByAreaOrMaterialCode(al::LiveActor* pActor) {
    if (al::tryKillByDeathArea(pActor)) {
        return true;
    }
    if (rc::isCollidedDamageFire(pActor) || rc::isCollidedPoison(pActor) ||
        rc::isCollidedInkSlow(pActor)) {
        pActor->kill();
        return true;
    }
    return false;
}

/** @brief Plays the death reaction and removes an actor touching a lethal surface.
 * @param pActor Actor to check.
 * @return Whether the check or request succeeds.
 */
bool tryKillByAreaOrMaterialCodeWithHitReaction(al::LiveActor* pActor) {
    if (isKillByAreaOrMaterialCode(pActor)) {
        al::startHitReactionDeath(pActor);
        if (al::isEqualString(pActor->getName(), "DoorKey")) {
            static_cast<DoorKey*>(pActor)->triggerKillForce(true);
        } else {
            pActor->kill();
        }
        return true;
    }
    return false;
}
}

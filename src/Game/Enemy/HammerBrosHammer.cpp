#include "Enemy/HammerBrosHammer.hpp"
#include "Enemy/BrosFunction.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(HammerBrosHammer, Move)
NERVE_DECL(HammerBrosHammer, Wait)
NERVES_MAKE_NOSTRUCT(HammerBrosHammer, Move, Wait)
}

/** @brief Creates a reusable hammer. @param pName Actor name. */
HammerBrosHammer::HammerBrosHammer(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the hammer and keeps it hidden until attached. @param rInfo Scene information. */
void HammerBrosHammer::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "HammerBrosHammer", nullptr);
    al::initNerve(this, &NrvHammerBrosHammerMove, 0);
    al::invalidateClipping(this);
    makeActorDead();
}

/** @brief Attacks contacted actors, excluding the throwing enemy.
 * @param pSelf Hammer sensor. @param pOther Contacted sensor. */
void HammerBrosHammer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) &&
         al::sendMsgEnemyAttack(pOther, pSelf)) ||
        (al::isSensorEnemy(pOther) && !al::isMySensor(pOther, mHost) &&
         al::sendMsgEnemyAttackBoomerang(pOther, pSelf)) ||
        (al::isSensorRide(pOther) && al::sendMsgEnemyAttackBoomerang(pOther, pSelf))) {
        al::startHitReactionHit(this);
        kill();
        return;
    }
    if (GameDataFunction::isSingleMode(this) &&
        (al::isSensorMapObj(pOther) || al::isSensorNpc(pOther) || al::isSensorKickKoura(pOther)) &&
        al::sendMsgEnemyAttackBoomerang(pOther, pSelf)) {
        al::startHitReactionHit(this);
        kill();
    }
}

/** @brief Handles attacks and Piranha Plant eating messages.
 * @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Hammer sensor.
 * @return Whether the message was handled. */
bool HammerBrosHammer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (al::isMsgPlayerBodyAttack(pMsg)) {
        return false;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    if (EnemyStateUtil::isMsgBlowDown(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
        al::isMsgNekoAttack(pMsg)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::appearItem(this);
        al::startHitReactionDisappear(this);
        kill();
        return true;
    }
    if (GameDataFunction::isSingleMode(this)) {
        if (rc::isMsgPackunEatStart(pMsg)) {
            return true;
        }
        if (rc::isMsgPackunEat(pMsg)) {
            al::startHitReactionDisappear(this);
            kill();
            return true;
        }
    }
    return false;
}

/** @brief Allows touch assist to destroy the hammer and release its item.
 * @param pMsg Incoming message. @param pPointer Touch pointer. @param pTarget Touch target.
 * @return Whether the assist was handled. */
bool HammerBrosHammer::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                            al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::setAppearItemFactor(this, "\u9593\u63a5\u653b\u6483", nullptr);
        al::appearItem(this);
        al::startHitReactionDisappear(this);
        kill();
        return true;
    }
    return false;
}

/** @brief Attaches the hammer to the throwing actor's hand.
 * @param pMtx Hand transform. @param rTrans Position offset. @param rRotate Rotation offset.
 * @param pAction Attached animation. */
void HammerBrosHammer::attach(const sead::Matrix34f* pMtx, const sead::Vector3f& rTrans,
                             const sead::Vector3f& rRotate, const char* pAction) {
    mFollowMtx = pMtx;
    mOffset = rTrans;
    mRotate = rRotate;
    rc::updateWeaponPose(this, mFollowMtx, mOffset, mRotate);
    al::resetPosition(this, false);
    al::setVelocityZero(this);
    al::offCollide(this);
    al::setNerve(this, &NrvHammerBrosHammerWait);
    al::startAction(this, pAction);
    al::invalidateHitSensors(this);
    al::invalidateScreenPointTargetAll(this);
    makeActorAppeared();
}

/** @brief Throws the hammer and enables its interactions.
 * @param pHost Throwing actor. @param rVelocity Initial velocity. */
void HammerBrosHammer::shoot(const al::LiveActor* pHost, const sead::Vector3f& rVelocity) {
    mHost = pHost;
    al::validateScreenPointTargetAll(this);
    al::validateHitSensors(this);
    al::setVelocity(this, rVelocity);
    al::onCollide(this);
    al::setNerve(this, &NrvHammerBrosHammerMove);
    al::startAction(this, "Spin");
    makeActorAppeared();
}

/** @brief Follows the hand while waiting to be thrown. */
void HammerBrosHammer::exeWait() {
    rc::updateWeaponPose(this, mFollowMtx, mOffset, mRotate);
}

/** @brief Spins through the air until colliding or reaching its lifetime. */
void HammerBrosHammer::exeMove() {
    al::addVelocityToGravity(this, 0.8f);
    al::scaleVelocity(this, 0.997f);
    al::getRotatePtr(this)->x += -30.0f;
    if (al::isCollided(this) || al::isGreaterStep(this, 600)) {
        al::startHitReactionDisappear(this);
        kill();
    }
}

#include "Enemy/TakoboBullet.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/BombStateExplosion.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(TakoboBullet, Move)
NERVE_DECL(TakoboBullet, SupportFreeze)
NERVE_DECL(TakoboBullet, Explode)
NERVE_DECL(TakoboBullet, FallDown)
NERVE_DECL(TakoboBullet, ExplodeSmall)
NERVES_MAKE_STRUCT(TakoboBullet, Move, SupportFreeze, Explode, FallDown, ExplodeSmall)
}

/** @brief Creates a projectile belonging to an enemy. @param pName Actor name. @param pHost Firing enemy. */
TakoboBullet::TakoboBullet(const char* pName, al::LiveActor* pHost)
    : al::LiveActor(pName), mHost(pHost) {}

/** @brief Initializes the projectile, freeze support and explosion. @param rInfo Scene information. */
void TakoboBullet::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TakoboBullet", nullptr);
    al::initNerve(this, &NrvTakoboBullet.Move, 2);
    mSupportFreeze = new ActorStateSupportFreeze(this, nullptr);
    al::initNerveState(this, mSupportFreeze, &NrvTakoboBullet.SupportFreeze,
                       "DRC\u306a\u3067\u306a\u3067");
    mExplosion = new BombStateExplosion(this, false, nullptr);
    al::initNerveState(this, mExplosion, &NrvTakoboBullet.Explode, "\u7206\u767a");
    makeActorDead();
}

/** @brief Resets the lifetime and explosion before appearing. */
void TakoboBullet::appear() {
    al::setNerve(this, &NrvTakoboBullet.Move);
    al::invalidateClipping(this);
    mExplosion->reset();
    mLifeStep = 0;
    al::LiveActor::appear();
}

/** @brief Attacks players and other enemies, then starts exploding.
 * @param pSelf Projectile sensor. @param pOther Contacted sensor. */
void TakoboBullet::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvTakoboBullet.FallDown) ||
        al::isNerve(this, &NrvTakoboBullet.ExplodeSmall)) {
        return;
    }
    if (al::isSensorPlayer(pOther) && al::isSensorEnemyAttack(pSelf)) {
        al::sendMsgEnemyAttack(pOther, pSelf);
        al::sendMsgPush(pOther, pSelf);
        al::setNerve(this, &NrvTakoboBullet.Explode);
        return;
    }
    if (al::getSensorHost(pOther) == mHost) {
        return;
    }
    if (al::isNerve(this, &NrvTakoboBullet.Explode)) {
        mExplosion->attackSensor(pSelf, pOther, nullptr);
    } else if (al::isSensorEnemyBody(pOther) && al::isSensorEnemyAttack(pSelf) &&
               (rc::sendMsgTakoboBulletAttack(pOther, pSelf) ||
                al::sendMsgExplosion(pOther, pSelf, nullptr))) {
        al::setNerve(this, &NrvTakoboBullet.Explode);
    }
}

/** @brief Handles attacks, projectile collisions and goal cleanup.
 * @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Receiver sensor.
 * @return Whether the message was handled. */
bool TakoboBullet::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvTakoboBullet.Explode)) {
        return false;
    }
    if (al::isNerve(this, &NrvTakoboBullet.ExplodeSmall)) {
        return false;
    }
    if (al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDisappear(this);
        al::setVelocityZero(this);
        makeActorDead();
        return true;
    }
    if (al::isNerve(this, &NrvTakoboBullet.FallDown)) {
        return false;
    }
    if (al::isMsgKickKouraAttack(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerBodyLanding(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerClimbSlidingAttack(pMsg) ||
        al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
        al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
        al::isMsgPlayerGiantAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvTakoboBullet.FallDown);
        return true;
    }
    if (rc::isMsgTakoboBulletAttack(pMsg)) {
        al::setNerve(this, &NrvTakoboBullet.Explode);
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    return false;
}

/** @brief Handles touch attacks and freeze assistance.
 * @param pMsg Incoming message. @param pPointer Touch pointer. @param pTarget Touch target.
 * @return Whether the assist was handled. */
bool TakoboBullet::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvTakoboBullet.FallDown)) {
        return false;
    }
    if (al::isNerve(this, &NrvTakoboBullet.Explode)) {
        return false;
    }
    if (al::isNerve(this, &NrvTakoboBullet.ExplodeSmall)) {
        return false;
    }
    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::setNerve(this, &NrvTakoboBullet.FallDown);
        return true;
    }
    if (mSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvTakoboBullet.SupportFreeze)) {
            al::setNerve(this, &NrvTakoboBullet.SupportFreeze);
        }
        return true;
    }
    return false;
}

/** @brief Flies until its lifetime expires or it collides. */
void TakoboBullet::exeMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FlyWait");
    }
    if (incrementCountAndChagneNerve()) {
        return;
    }
    if (al::isCollided(this)) {
        al::setNerve(this, &NrvTakoboBullet.Explode);
    }
}

/** @brief Advances the lifetime and expires the projectile. @return Whether it expired. */
bool TakoboBullet::incrementCountAndChagneNerve() {
    if (++mLifeStep >= 120) {
        al::setNerve(this, &NrvTakoboBullet.ExplodeSmall);
        return true;
    }
    return false;
}

/** @brief Runs the damaging explosion and removes the actor when it ends. */
void TakoboBullet::exeExplode() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Explode");
    }
    if (al::updateNerveState(this)) {
        makeActorDead();
    }
}

/** @brief Removes an expired projectile with a small disappearance reaction. */
void TakoboBullet::exeExplodeSmall() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startHitReactionDisappear(this);
        makeActorDead();
    }
}

/** @brief Falls after being struck, disappearing on impact or after sixty frames. */
void TakoboBullet::exeFallDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FallDown");
        al::setVelocityJump(this, 10.0f);
    }
    if (al::isCollided(this) || al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvTakoboBullet.ExplodeSmall);
        return;
    }
    al::addVelocityToGravity(this, 0.98f);
}

/** @brief Updates freeze assistance while continuing the lifetime countdown. */
void TakoboBullet::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvTakoboBullet.Move);
        return;
    }
    incrementCountAndChagneNerve();
}

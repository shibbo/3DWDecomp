#include "Enemy/BombStateExplosion.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
BombStateExplosionParam sDefaultParam = {300.0f, 150.0f, 20.0f, {4.0f, 3.0f, 1.0f, 1.0f}, 2};
NERVE_DECL(BombStateExplosion, Explosion)
NERVES_MAKE_NOSTRUCT(BombStateExplosion, Explosion)
}

/** @brief Creates an explosion state with optional player damage.
 * @param pActor Bomb actor. @param isUseExplosionSensor Whether to enable the player explosion sensor.
 * @param pParam Explosion settings, or nullptr for defaults.
 */
BombStateExplosion::BombStateExplosion(al::LiveActor* pActor, bool isUseExplosionSensor,
                                     const BombStateExplosionParam* pParam)
    : al::ActorStateBase("\u30dc\u30e0\u7206\u767a\u30b9\u30c6\u30fc\u30c8", pActor),
      mIsUseExplosionSensor(isUseExplosionSensor), mIsAttackToPlayer(true),
      mParam(pParam ? pParam : &sDefaultParam) {}

/** @brief Initializes the actor and explosion nerve. */
void BombStateExplosion::init() {
    reset();
    initNerve(&NrvBombStateExplosionExplosion, 0);
}

/** @brief Restores the bomb's model, collision, and inactive explosion sensors. */
void BombStateExplosion::reset() {
    al::showModelIfHide(mHostActor);
    al::onCollide(mHostActor);
    al::validateHitSensors(mHostActor);
    al::invalidateHitSensor(mHostActor, "Explosion");
    al::setSensorRadius(mHostActor, "Explosion", 0.0f);
    if (mIsUseExplosionSensor) {
        al::invalidateHitSensor(mHostActor, "ExplosionPlayer");
        al::setSensorRadius(mHostActor, "ExplosionPlayer", mParam->radiusMin);
    }
    mIsAttackToPlayer = true;
}

/** @brief Starts an explosion. */
void BombStateExplosion::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvBombStateExplosionExplosion);
}

/** @brief Sends explosion damage through the appropriate sensor.
 * @param pSelf Bomb sensor. @param pOther Receiving sensor. @param pCounter Enemy combo counter.
 */
void BombStateExplosion::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther, al::ComboCounter* pCounter) {
    if (al::isSensorPlayer(pOther)) {
        if (mIsAttackToPlayer && al::isSensorName(pSelf, "ExplosionPlayer")) {
            al::sendMsgExplosion(pOther, pSelf, nullptr);
        }
    } else if (al::isSensorName(pSelf, "Explosion")) {
        al::sendMsgExplosion(pOther, pSelf, pCounter);
    }
}

/** @brief Sends explosion damage through the appropriate sensor.
 * @param pSelf Bomb sensor. @param pOther Receiving sensor. @param pCounter Enemy combo counter.
 */
void BombStateExplosion::attackSensorGiga(al::HitSensor* pSelf, al::HitSensor* pOther, al::ComboCounter* pCounter) {
    if (al::isSensorPlayer(pOther)) {
        if (mIsAttackToPlayer && al::isSensorName(pSelf, "ExplosionPlayer")) {
            al::sendMsgGigaEnemyAttack(pOther, pSelf);
        }
    } else if (al::isSensorName(pSelf, "Explosion")) {
        al::sendMsgExplosion(pOther, pSelf, pCounter);
    }
}

/** @brief Expands the explosion and triggers its light and hit reactions. */
void BombStateExplosion::exeExplosion() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(mHostActor);
        al::setVelocityZero(mHostActor);
        al::offCollide(mHostActor);
        al::startHitReactionExplode(mHostActor);
        al::invalidateHitSensors(mHostActor);
        al::validateHitSensor(mHostActor, "Explosion");
        if (mIsUseExplosionSensor) {
            al::validateHitSensor(mHostActor, "ExplosionPlayer");
        }
    }
    if (al::isStep(this, mParam->attackStep)) {
        al::startHitReaction(mHostActor, "\u7206\u767a\u632f\u52d5");
    }
    if (mIsUseExplosionSensor && al::isStep(this, 1)) {
        al::invalidateHitSensor(mHostActor, "ExplosionPlayer");
    }
    al::requestPrePassLightColor(mHostActor, "\u7206\u767a", mParam->lightColor);
    f32 radius = al::getSensorRadius(mHostActor, "Explosion");
    if (mParam->radiusMax <= radius) {
        kill();
    } else {
        al::setSensorRadius(mHostActor, "Explosion", radius + mParam->radiusSpeed);
    }
}

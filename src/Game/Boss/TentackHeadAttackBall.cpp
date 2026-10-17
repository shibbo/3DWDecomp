#include "Boss/TentackHeadAttackBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
const sead::Vector3f cShotOffset(0.0f, 500.0f, 250.0f);
}

/**
 * @brief Creates a projectile with its lifetime counter reset.
 * @param pName Actor name.
 */
TentackHeadAttackBall::TentackHeadAttackBall(const char* pName) : al::LiveActor(pName) {
}

/**
 * @brief Loads the projectile resources and leaves it inactive until fired.
 * @param rInfo Actor placement and scene initialization information.
 */
void TentackHeadAttackBall::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TentackHeadAttackBall", nullptr);
    makeActorDead();
}

/** @brief Plays the disappearance reaction and removes the projectile. */
void TentackHeadAttackBall::kill() {
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}

/** @brief Removes the projectile after 120 control updates. */
void TentackHeadAttackBall::control() {
    if (++mLifeFrame >= 120) {
        kill();
    }
}

/**
 * @brief Sends a fire attack to players, their weapons, map objects, and enemies.
 * @param pSelf Projectile attack sensor.
 * @param pOther Contacted sensor.
 */
void TentackHeadAttackBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayerOrPlayerWeapon(pOther) || al::isSensorMapObj(pOther) ||
        al::isSensorEnemy(pOther)) {
        al::sendMsgEnemyAttackFire(pOther, pSelf);
    }
}

/**
 * @brief Acknowledges player fireballs and requests their hit reaction.
 * @param pMsg Incoming sensor message.
 * @param pSelf Projectile sensor.
 * @param pOther Attacking sensor.
 * @return True when the message is a player fireball attack.
 */
bool TentackHeadAttackBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                     al::HitSensor* pOther) {
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pOther, pSelf);
        return true;
    }
    return false;
}

/**
 * @brief Fires from the source's local offset toward a point above the target.
 * @param pSource Actor providing the launch pose.
 * @param rTarget Target world position, aimed at with a 100-unit upward offset.
 */
void TentackHeadAttackBall::shot(const al::LiveActor* pSource, const sead::Vector3f& rTarget) {
    mLifeFrame = 0;
    sead::Vector3f launchPos(0.0f, 0.0f, 0.0f);
    al::calcTransLocalOffset(&launchPos, pSource, cShotOffset);
    sead::Vector3f direction = rTarget + sead::Vector3f(0.0f, 100.0f, 0.0f);
    direction -= launchPos;
    al::setVelocityToDirection(this, direction, 15.0f);
    al::resetPosition(this, launchPos, false);
    appear();
    al::startAction(this, "Shot");
}

/** @brief Destroys the projectile's base actor resources. */
TentackHeadAttackBall::~TentackHeadAttackBall() = default;

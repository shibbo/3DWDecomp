#include "Boss/KoopaLastFireBreath.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KoopaLastFireBreath, Move);
NERVE_DECL(KoopaLastFireBreath, Collided);
NERVES_MAKE_NOSTRUCT(KoopaLastFireBreath, Move, Collided)
}

/**
 * @brief Creates the final-boss fire-breath projectile.
 * @param pName Actor name.
 */
KoopaLastFireBreath::KoopaLastFireBreath(const char* pName) : al::LiveActor(pName) {
}

/**
 * @brief Loads the breath variant of the fireball and leaves it inactive.
 * @param rInfo Actor placement and scene initialization information.
 */
void KoopaLastFireBreath::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "KoopaFireBall", "Breath");
    al::initNerve(this, &NrvKoopaLastFireBreathMove, 0);
    al::invalidateClipping(this);
    makeActorDead();
}

/**
 * @brief Sends a fire attack when the attacking sensor touches a player.
 * @param pSelf Projectile sensor, which must be an enemy attack sensor to deal damage.
 * @param pOther Contacted sensor.
 */
void KoopaLastFireBreath::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgEnemyAttackFire(pOther, pSelf);
    }
}

/**
 * @brief Aims and launches the projectile with collision initially disabled.
 * @param rStart Launch world position.
 * @param rTarget Target world position; a zero direction falls back to world up.
 * @param speed Projectile speed.
 */
void KoopaLastFireBreath::appearAttack(const sead::Vector3f& rStart,
                                     const sead::Vector3f& rTarget, float speed) {
    sead::Vector3f direction = rTarget - rStart;
    if (al::normalizeOrZero(&direction)) {
        direction = sead::Vector3f::ey;
    }
    al::makeQuatFrontNoSupport(al::getQuatPtr(this), direction);
    al::setTrans(this, rStart);
    al::setVelocity(this, direction * speed);
    al::setNerve(this, &NrvKoopaLastFireBreathMove);
    al::offCollide(this);
    appear();
}

/** @brief Enables collision at step 30 and limits flight to 180 steps. */
void KoopaLastFireBreath::exeMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Shot");
    }
    if (al::isStep(this, 30)) {
        al::onCollide(this);
    }
    if (al::isCollided(this)) {
        al::setNerve(this, &NrvKoopaLastFireBreathCollided);
        return;
    }
    if (!al::isLessStep(this, 180)) {
        kill();
    }
}

/** @brief Removes the projectile 30 steps after collision. */
void KoopaLastFireBreath::exeCollided() {
    if (!al::isLessStep(this, 30)) {
        kill();
    }
}

/** @brief Destroys the projectile's base actor resources. */
KoopaLastFireBreath::~KoopaLastFireBreath() = default;

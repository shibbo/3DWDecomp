#include "Boss/BunbunArm.hpp"
#include "Boss/BunbunStateSpinAttack.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"

/**
 * @brief Creates the arm actor controlled by Bunbun's spin attack.
 * @param pName Actor name.
 * @param pSpinAttack State controlling whether attack sensors are active.
 */
BunbunArm::BunbunArm(const char* pName, BunbunStateSpinAttack* pSpinAttack)
    : al::LiveActor(pName), mSpinAttack(pSpinAttack) {
}

/**
 * @brief Initializes the spin-arm model and leaves the actor inactive.
 * @param rInfo Actor placement and scene initialization information.
 */
void BunbunArm::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BunbunSpinArm", nullptr);
    makeActorDead();
}

/**
 * @brief Pushes and attacks eligible contacts while the spin sensor is active.
 * @param pSelf Arm attack sensor.
 * @param pOther Contacted sensor.
 */
void BunbunArm::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!mSpinAttack->isValidAttackSensor()) {
        return;
    }
    if (al::isSensorPlayerOrPlayerWeapon(pOther) || al::isSensorKoopaJr(pOther)) {
        if (!isHitCylinder(pSelf, pOther)) {
            return;
        }
        al::sendMsgPush(pOther, pSelf);
    } else if (!al::isSensorEnemyBody(pOther)) {
        return;
    }
    al::sendMsgEnemyAttack(pOther, pSelf);
}

/**
 * @brief Tests overlap within the arm's gravity-aligned 200-unit-high cylinder.
 * @param pSelf Arm sensor providing the horizontal center and radius.
 * @param pOther Sensor tested against the cylinder.
 * @return True when height and combined sensor radius constraints are satisfied.
 */
bool BunbunArm::isHitCylinder(al::HitSensor* pSelf, al::HitSensor* pOther) const {
    sead::Vector3f heightVec = al::getSensorPos(pOther) - al::getTrans(this);
    al::parallelizeVec(&heightVec, al::getGravity(this), heightVec);
    float height = (-al::getGravity(this)).dot(heightVec);
    if (height < 0.0f || height > 200.0f) {
        return false;
    }
    sead::Vector3f distanceVec = al::getSensorPos(pOther) - al::getSensorPos(pSelf);
    al::verticalizeVec(&distanceVec, al::getGravity(this), distanceVec);
    return distanceVec.length() < al::getSensorRadius(pSelf) + al::getSensorRadius(pOther);
}

/** @brief Destroys the arm's base actor resources. */
BunbunArm::~BunbunArm() = default;

#include "Enemy/Killer.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/KillerGenerator.hpp"
#include "Enemy/KillerStateFly.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace EnemyStateUtil {
bool isMsgPressDownForCrossoverSensor(const al::SensorMsg* pMsg,
                                     const al::HitSensor* pOther, const al::HitSensor* pSelf);
bool isMsgBlowDown(const al::SensorMsg* pMsg);
}
namespace rc {
void emitEcho(const al::LiveActor* pActor, const sead::Vector3f& rTrans,
              float radius, int step, bool isForce);
bool isInDeathArea(const al::LiveActor* pActor);
}

namespace {
NERVE_DECL(Killer, Appear)
NERVE_DECL(Killer, SupportFreeze)
NERVE_DECL(Killer, BlowDown)
NERVE_DECL(Killer, FlyWait)
NERVE_DECL(Killer, Explode)
NERVE_DECL(Killer, Reflect)
NERVE_DECL(Killer, FallDown)
NERVE_DECL(Killer, HipDropDown)
NERVE_DECL(Killer, StandBy)
const struct {
    KillerNrvAppear Appear;
    KillerNrvSupportFreeze SupportFreeze;
    KillerNrvBlowDown BlowDown;
    KillerNrvFlyWait FlyWait;
    KillerNrvExplode Explode;
    KillerNrvReflect Reflect;
} NrvKiller;
NERVES_MAKE_NOSTRUCT(Killer, FallDown, HipDropDown, StandBy)

const char* const cMagnumAttackSensors[] = {"Attack1", "Attack2"};
const char* const cNormalAttackSensors[] = {"Attack"};

sead::Vector3f cNormalItemOffset(0.0f, 100.0f, 0.0f);
sead::Vector3f cMagnumItemOffset(0.0f, 300.0f, 250.0f);
ActorStateSupportFreezeParam cNormalFreezeParam(true, 15, false, true, 120, cNormalItemOffset);
ActorStateSupportFreezeParam cMagnumFreezeParam(true, 15, false, true, 120, cMagnumItemOffset);
EnemyStateBlowDownParam cBlowDownParam(false);

/** @brief Selects the model archive for a projectile type.
 * @param type Normal, search, Magnum, or search Magnum type.
 * @return Corresponding model archive name.
 */
const char* getKillerArchive(int type) {
    switch (type) {
    case 1:
        return "KillerSearch";
    case 2:
        return "KillerMagnum";
    case 3:
        return "KillerMagnumSearch";
    default:
        return "Killer";
    }
}
}

/** @brief Constructs a Bullet Bill projectile owned by a generator.
 * @param pName Actor name.
 * @param type Normal, search, Magnum, or search Magnum type.
 * @param pGenerator Generator controlling firing and lifetime settings.
 * @param pFilter Initial collision filter for the launcher.
 */
Killer::Killer(const char* pName, int type, KillerGenerator* pGenerator,
               const al::CollisionPartsFilterBase* pFilter)
    : al::LiveActor(pName), mGenerator(pGenerator), mFilter(pFilter), mType(type) {}

/** @brief Initializes the selected model and its freeze, knockback, and flight states.
 * @param rInfo Actor placement and scene information.
 */
void Killer::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, getKillerArchive(mType), nullptr);
    al::initNerve(this, &NrvKiller.Appear, 3);
    mIsSingleMode = al::isSingleMode(rInfo);
    mStateSupportFreeze = new ActorStateSupportFreeze(
        this, isMagnum(mType) ? &cMagnumFreezeParam : &cNormalFreezeParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &cBlowDownParam);
    mStateFly = new KillerStateFly(this);
    al::initNerveState(this, mStateSupportFreeze, &NrvKiller.SupportFreeze, "DRC拘束状態");
    al::initNerveState(this, mStateBlowDown, &NrvKiller.BlowDown, "吹き飛び状態");
    al::initNerveState(this, mStateFly, &NrvKiller.FlyWait, "飛行状態");
    if (isSearch(mType)) {
        al::initJointControllerKeeper(this, 1);
        al::initJointLocalZRotator(this, &mJointRotation, "AllRoot");
    }
    makeActorDead();
}

/** @brief Resets collision, sensors, shadows, and support state for reuse. */
void Killer::appear() {
    al::LiveActor::appear();
    al::setColliderFilterCollisionParts(this, mFilter);
    if (!mIsSingleMode) {
        al::invalidateClipping(this);
    }
    invalidateAttackSensors();
    al::invalidateHitSensor(this, "Explosion");
    al::invalidateShadow(this);
    mStateSupportFreeze->resetAppearItem();
    al::offCollide(this);
    mJointRotation = 0.0f;
    mIsCollide = false;
}

/** @brief Disables the attack sensors associated with the projectile size. */
void Killer::invalidateAttackSensors() {
    const char* const* pSensors = isMagnum(mType) ? cMagnumAttackSensors : cNormalAttackSensors;
    unsigned int count = isMagnum(mType) ? 2 : 1;
    for (unsigned int i = 0; i < count; i++) {
        al::invalidateHitSensor(this, pSensors[i]);
    }
}

/** @brief Kills the projectile and clears saved flight time. */
void Killer::kill() {
    al::LiveActor::kill();
    mStateFly->reset();
}

/** @brief Plays the switch-death reaction and removes an active projectile. */
void Killer::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }
    al::startHitReaction(this, "スイッチで消滅");
    kill();
}

/** @brief Sends projectile attacks and propagates its explosion shock wave.
 * @param pSelf Projectile sensor.
 * @param pOther Contacted actor's sensor.
 */
void Killer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isNerve(this, &NrvKiller.FlyWait) &&
        !al::isNerve(this, &NrvKiller.SupportFreeze) && !al::isNerve(this, &NrvKiller.Explode)) {
        return;
    }
    if (al::isSensorEnemyAttack(pSelf) && isMagnum(mType) &&
        rc::sendMsgKillerMagnumExplosion(pOther, pSelf)) {
        al::setNerve(this, &NrvKiller.Explode);
        return;
    }
    if (al::isSensorEye(pSelf) && al::isSensorEnemyBody(pOther) &&
        al::isNerve(this, &NrvKiller.Explode)) {
        rc::sendMsgKillerShockWave(pOther, pSelf);
    }
    if (al::isNerve(this, &NrvKiller.Explode)) {
        return;
    }
    if (al::isSensorEnemyBody(pSelf)) {
        if (al::isSensorPlayerOrPlayerWeapon(pOther) || al::isNoCollide(this)) {
            return;
        }
        if (al::sendMsgExplosion(pOther, pSelf, nullptr)) {
            if (!isMagnum(mType)) {
                al::setNerve(this, &NrvKiller.Explode);
            }
            return;
        }
    }
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) &&
               al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
        al::setNerve(this, &NrvKiller.Explode);
    }
}

/** @brief Handles explosions, player attacks, stomps, and reflection.
 * @param pMsg Incoming sensor message.
 * @param pOther Attacking sensor.
 * @param pSelf Projectile sensor receiving the message.
 * @return Whether the message was handled.
 */
bool Killer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if ((!al::isNerve(this, &NrvKiller.FlyWait) && !al::isNerve(this, &NrvKiller.Reflect) &&
         !al::isNerve(this, &NrvKiller.SupportFreeze)) || !al::isSensorEnemyBody(pSelf)) {
        return false;
    }
    if (!(isMagnum(mType) && al::isMsgExplosion(pMsg)) &&
        (al::isMsgExplosion(pMsg) || al::isMsgKillerAttack(pMsg) ||
         rc::isMsgKillerMagnumExplosion(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg))) {
        al::setNerve(this, &NrvKiller.Explode);
        if (rc::tryFindRelativeControlUserId(pOther) != -1) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            al::appearItem(this);
        }
        if (rc::isMsgKillerMagnumExplosion(pMsg) && !isMagnum(mType)) {
            return false;
        }
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    if ((isMagnum(mType) && (al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
                            al::isMsgPlayerBodyAttackReflect(pMsg))) ||
        EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        if (al::isMsgPlayerObjHipDropAll(pMsg)) {
            mHipDropActor = al::getSensorHost(pOther);
            al::setNerve(this, &NrvKillerHipDropDown);
        } else {
            al::setNerve(this, &NrvKillerFallDown);
        }
        return true;
    }
    if (mIsCollide && !al::isNerve(this, &NrvKiller.Reflect) &&
        (al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
         al::isMsgPlayerTailAttack(pMsg) || al::isMsgKillerReflect(pMsg) ||
         rc::isMsgSpinnerAttack(pMsg))) {
        sead::Vector3f dir(0.0f, 0.0f, 0.0f);
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        if (al::isNearZero(dir, 0.001f)) {
            return false;
        }
        al::faceToDirection(this, dir);
        al::startHitReaction(this, "跳ね返し");
        if (!rc::isMsgSpinnerAttack(pMsg) && !al::isMsgKillerReflect(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }
        al::setNerve(this, &NrvKiller.Reflect);
        return true;
    }
    if ((!al::isMsgEnemyAttackFire(pMsg) && !al::isMsgExplosion(pMsg) &&
         !al::isMsgPlayerClimbAttack(pMsg) && !al::isMsgPlayerFireBallAttack(pMsg) &&
         !al::isMsgPlayerSpinAttack(pMsg) && !al::isMsgPlayerTailAttack(pMsg) &&
         EnemyStateUtil::isMsgBlowDown(pMsg)) ||
        (mIsSingleMode && rc::isMsgBobsledBodyAttack(pMsg)) || rc::isMsgJumpPanelAction(pMsg)) {
        if (mIsSingleMode && rc::isMsgBobsledBodyAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        if (!isMagnum(mType) || (al::isSensorPlayer(pOther) && rc::isPlayerGiant(pOther))) {
            mStateBlowDown->setBlowDir(al::getSensorHost(pOther));
            al::setNerve(this, &NrvKiller.BlowDown);
        } else {
            al::setNerve(this, &NrvKillerFallDown);
        }
        return true;
    }
    return false;
}

/** @brief Routes touch assistance into the support-freeze state during flight.
 * @param pMsg Incoming touch message.
 * @param pPointer Screen pointer generating the message.
 * @param pTarget Touched screen-point target.
 * @return Whether the freeze state handled the message.
 */
bool Killer::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    if (!isNerveFlyWait() && !al::isNerve(this, &NrvKiller.SupportFreeze)) {
        return false;
    }
    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (isNerveFlyWait()) {
            al::setNerve(this, &NrvKiller.SupportFreeze);
        }
        return true;
    }
    return false;
}

/** @brief Checks whether the projectile is alive and past its launch animation.
 * @return Whether active flight is in progress.
 */
bool Killer::isNerveFlyWait() {
    return al::isAlive(this) && al::isNerve(this, &NrvKiller.FlyWait) && mStateFly->getStepFly() > 0;
}

/** @brief Enables world collision when permitted by the generator settings. */
void Killer::tryOnCollide() {
    mIsCollide = true;
    if (mGenerator->mIsValidateCollision) {
        al::onCollide(this);
        al::setColliderFilterCollisionParts(this, nullptr);
    }
}

/** @brief Plays the appearance animation and enters standby. */
void Killer::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        mGenerator->tryStartHostAction("Appear");
    }
    if (al::isStep(this, 1)) {
        al::showModelIfHide(this);
    }
    al::setNerveAtActionEnd(this, &NrvKillerStandBy);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKillerStandBy);
    }
}

/** @brief Allows the waiting projectile to be clipped with its launcher. */
void Killer::exeStandBy() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
    }
}

/** @brief Updates flight and sends a collision explosion when flight finishes. */
void Killer::exeFlyWait() {
    if (al::updateNerveState(this)) {
        al::HitSensor* pWall = al::tryGetCollidedWallSensor(this);
        if (pWall) {
            al::sendMsgExplosionCollide(pWall, al::getHitSensor(this, "Body1"), nullptr);
        }
        al::setNerve(this, &NrvKiller.Explode);
    }
}

/** @brief Falls under gravity after a stomp and disappears on landing or timeout. */
void Killer::exeFallDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FallDown");
        al::setVelocityJump(this, 12.0f);
        al::onCollide(this);
    }
    al::addVelocityToGravity(this, 1.0f);
    mJointRotation = al::converge(mJointRotation, 0.0f, 1.0f);
    if (al::isCollidedGround(this) || al::isGreaterStep(this, 300) || rc::isInDeathArea(this)) {
        al::startHitReaction(this, "落下して消滅");
        al::appearItem(this);
        al::offCollide(this);
        kill();
    }
}

/** @brief Falls with the ground-pound attacker's vertical speed. */
void Killer::exeHipDropDown() {
    if (al::isFirstStep(this)) {
        al::tryStartAction(this, "FallDown");
        al::setVelocityZero(this);
        float speed = al::getVelocity(mHipDropActor).y;
        if (speed > 0.0f) {
            speed = -0.4f;
        }
        if (speed > -6.0f && al::isEqualString(mHipDropActor->getName(), "KoopaJr")) {
            speed = -6.0f;
        }
        al::setVelocityY(this, speed);
        al::onCollide(this);
    }
    if (al::isCollidedGround(this) || al::isGreaterStep(this, 90) || rc::isInDeathArea(this)) {
        al::appearItem(this);
        al::startHitReaction(this, "ヒップドロップされて消滅");
        al::offCollide(this);
        kill();
    }
}

/** @brief Stops briefly for the reflection animation before resuming flight. */
void Killer::exeReflect() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reflect");
        al::setVelocityZero(this);
        mJointRotation = 0.0f;
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKiller.FlyWait);
    }
}

/** @brief Expands the explosion sensor and removes the projectile at its maximum radius. */
void Killer::exeExplode() {
    if (al::isFirstStep(this)) {
        al::validateHitSensor(this, "Explosion");
        al::setSensorRadius(this, "Explosion", 100.0f);
        al::startHitReaction(this, "接触して爆発");
        rc::emitEcho(this, al::getTrans(this), 600.0f, 90, false);
        al::hideModelIfShow(this);
        al::offCollide(this);
    }
    float radius = al::getSensorRadius(this, "Explosion");
    if (radius > 250.0f) {
        kill();
    } else {
        al::setSensorRadius(this, "Explosion", radius + 15.0f);
    }
}

/** @brief Updates knockback and drops an item when the knockback state finishes. */
void Killer::exeBlowDown() {
    if (al::isFirstStep(this)) {
        mJointRotation = 0.0f;
    }
    if (al::updateNerveState(this)) {
        al::appearItem(this);
        al::startHitReaction(this, "吹き飛び消滅");
        kill();
    }
}

/** @brief Resumes flight after touch assistance releases the projectile. */
void Killer::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvKiller.FlyWait);
    }
}

/** @brief Appears directly in standby for a switch-controlled launcher. */
void Killer::forceStandBy() {
    appear();
    al::validateClipping(this);
    if (isSearch(mType) && !isMagnum(mType)) {
        al::startAction(this, "Standby");
    }
    al::setNerve(this, &NrvKillerStandBy);
}

/** @brief Appears at the generator position, initially hiding the model. */
void Killer::startStandByAppear() {
    appear();
    al::setNerve(this, &NrvKiller.Appear);
    al::resetPosition(this, al::getTrans(mGenerator), false);
    al::hideModelIfShow(this);
}

/** @brief Starts flight and the corresponding launcher action. */
void Killer::startFlyWait() {
    mGenerator->tryStartHostAction("Attack");
    al::setNerve(this, &NrvKiller.FlyWait);
    al::invalidateClipping(this);
    al::validateShadow(this);
}

/** @brief Enables the attack sensors associated with the projectile size. */
void Killer::validateAttackSensors() {
    const char* const* pSensors = isMagnum(mType) ? cMagnumAttackSensors : cNormalAttackSensors;
    unsigned int count = isMagnum(mType) ? 2 : 1;
    for (unsigned int i = 0; i < count; i++) {
        al::validateHitSensor(this, pSensors[i]);
    }
}

/** @brief Gets the configured projectile lifetime.
 * @return Maximum flight duration in steps.
 */
int Killer::getStepDisappear() const { return mGenerator->mStepDisappear; }

/** @brief Gets the configured flight acceleration.
 * @return Generator acceleration setting.
 */
float Killer::getAccel() const { return mGenerator->mAccelFly; }

/** @brief Gets acceleration relative to its default value.
 * @return Generator acceleration ratio.
 */
float Killer::getAccelRate() const { return mGenerator->getAccelRate(); }

/** @brief Checks whether a projectile type uses the Magnum model.
 * @param type Projectile type.
 * @return Whether the type is Magnum or search Magnum.
 */
bool Killer::isMagnum(int type) { return type == 2 || type == 3; }

/** @brief Checks whether a projectile type homes toward players.
 * @param type Projectile type.
 * @return Whether the type is search or search Magnum.
 */
bool Killer::isSearch(int type) { return type == 1 || type == 3; }

/** @brief Checks whether the projectile is ready to leave the launcher.
 * @return Whether the standby nerve is active.
 */
bool Killer::isStandBy() const { return al::isNerve(this, &NrvKillerStandBy); }

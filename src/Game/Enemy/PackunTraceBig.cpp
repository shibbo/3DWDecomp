#include "Enemy/PackunTraceBig.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(PackunTraceBig, Appear)
NERVE_DECL(PackunTraceBig, Reaction)
NERVE_DECL(PackunTraceBig, Wait)
NERVES_MAKE_NOSTRUCT(PackunTraceBig, Appear, Reaction, Wait)
}

/** @brief Constructs the remains associated with a large Piranha Plant.
 * @param pOwner Plant whose pose is copied when the remains appear.
 */
PackunTraceBig::PackunTraceBig(al::LiveActor* pOwner)
    : al::LiveActor("パックン残骸"), mOwner(pOwner) {
    getName();
}

/** @brief Initializes the model and collision connector, leaving the actor inactive.
 * @param rInfo Actor placement and scene information.
 */
void PackunTraceBig::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "PackunTraceBig", nullptr);
    al::initNerve(this, &NrvPackunTraceBigAppear, 0);
    mConnector = al::createMtxConnector(this);
    makeActorDead();
}

/** @brief Attaches the remains to the placed collision geometry. */
void PackunTraceBig::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mConnector, this, false);
}

/** @brief Follows the collision pose while preserving the current facing direction. */
void PackunTraceBig::control() {
    sead::Vector3f front;
    al::calcQuatFront(&front, al::getQuat(this));
    al::connectPoseQT(this, mConnector);
    sead::Quatf quat = al::getQuat(this);
    quat.setMul(mBaseQuat, quat);
    sead::Vector3f up;
    al::calcQuatUp(&up, quat);
    sead::Vector3f side = up.cross(front);
    if (!al::isNearZero(side, 0.001f)) {
        al::makeQuatUpFront(al::getQuatPtr(this), up, front);
    }
}

/** @brief Appears at the owner's pose with a random yaw and an appearance reaction. */
void PackunTraceBig::appear() {
    al::copyPose(this, mOwner);
    al::LiveActor::appear();
    al::rotateQuatYDirDegree(this, al::getRandomDegree());
    al::startHitReactionAppear(this);
}

/** @brief Pushes players and Bowser Jr. away from the remains.
 * @param pSelf Sensor belonging to the remains.
 * @param pOther Contacted sensor.
 */
void PackunTraceBig::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/** @brief Reacts to ordinary attacks and breaks under a giant player's attack.
 * @param pMsg Incoming sensor message.
 * @param pOther Attacker's sensor.
 * @param pSelf Receiving sensor belonging to the remains.
 * @return Whether the attack is accepted; fireballs trigger a reaction but return false.
 */
bool PackunTraceBig::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    // The original performs two calls to the same giant-attack predicate.
    if (al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReactionDeath(this);
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvPackunTraceBigReaction)) {
        return false;
    }
    if (al::isMsgTrampleAll(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)
        || al::isMsgPlayerRollingAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg)
        || al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg)
        || al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg)
        || al::isMsgPlayerSpinAttack(pMsg)) {
        al::setNerve(this, &NrvPackunTraceBigReaction);
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        al::setNerve(this, &NrvPackunTraceBigReaction);
    }
    return false;
}

/** @brief Plays the appearance animation before entering the idle state. */
void PackunTraceBig::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunTraceBigWait);
    }
}

/** @brief Starts the idle animation on entry. */
void PackunTraceBig::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/** @brief Plays an attack reaction before returning to idle. */
void PackunTraceBig::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPackunTraceBigWait);
    }
}

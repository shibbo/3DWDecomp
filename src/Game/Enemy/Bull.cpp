#include "Enemy/Bull.hpp"

#include <cmath>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Bull, Wait)
NERVE_DECL(Bull, BlowDown)
NERVE_DECL(Bull, SupportFreeze)
NERVE_DECL(Bull, AttackSuccess)
NERVE_DECL(Bull, Blow)
NERVE_DECL(Bull, PressDown)
NERVE_DECL(Bull, Trampled)
NERVE_DECL(Bull, Jump)
NERVE_DECL(Bull, Revival)
NERVE_DECL(Bull, Find)
NERVE_DECL(Bull, Turn)
NERVE_DECL(Bull, Stop)
NERVE_DECL(Bull, Brake)
NERVE_DECL(Bull, Run)
NERVE_DECL(Bull, Angry)
NERVE_DECL(Bull, TrampledEnd)
// Non-const nerve objects: the game keeps them in .data in this order.
BullNrvWait NrvBullWait;
BullNrvBlowDown NrvBullBlowDown;
BullNrvSupportFreeze NrvBullSupportFreeze;
BullNrvAttackSuccess NrvBullAttackSuccess;
BullNrvBlow NrvBullBlow;
BullNrvPressDown NrvBullPressDown;
BullNrvTrampled NrvBullTrampled;
BullNrvJump NrvBullJump;
BullNrvRevival NrvBullRevival;
BullNrvFind NrvBullFind;
BullNrvTurn NrvBullTurn;
BullNrvStop NrvBullStop;
BullNrvBrake NrvBullBrake;
BullNrvRun NrvBullRun;
BullNrvAngry NrvBullAngry;
BullNrvTrampledEnd NrvBullTrampledEnd;

EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 200.0f, 0.0f));

constexpr s32 cRevivalTime = 400;
constexpr f32 cScoreComboFactor = 150.0f;
constexpr f32 cGravity = 1.5f;
constexpr f32 cFriction = 0.94f;
}  // namespace

/**
 * @brief Constructs a Bull.
 * @param pName Actor name.
 */
Bull::Bull(const char* pName) : al::LiveActor(pName) {
    // The bull starts its own blow down actions, so the state must not start one.
    sBlowDownParam.mAction = nullptr;
}

/**
 * @brief Initializes the model, collider, nerves and the blow down / support freeze states.
 * @param rInfo Placement info of the actor.
 */
void Bull::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    mInitScale = al::getScale(this);
    al::initNerve(this, &NrvBullWait, 2);

    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    al::initNerveState(this, mStateBlowDown, &NrvBullBlowDown, "[state]吹き飛ばし");
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateSupportFreeze, &NrvBullSupportFreeze, "[state]DRC拘束");
    makeActorAppeared();
    mInitTrans = al::getTrans(this);
    mInitFront = al::getFront(this);
}

/**
 * @brief Pushes other enemies away and attacks enemies and players it touches.
 * @param pSelf Sensor of the bull.
 * @param pOther Sensor that was touched.
 */
void Bull::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther)) {
        if (isEnablePush()) {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        }

        if (isEnableAttack()) {
            rc::sendMsgBullAttack(pOther, pSelf);
        }
    }

    if (al::isSensorPlayer(pOther) && al::isSensorEnemyAttack(pSelf) && isEnableAttack() &&
        al::sendMsgEnemyAttack(pOther, pSelf)) {
        al::setVelocityZeroH(this);
        al::turnDirectionToTarget(this, al::getFrontPtr(this), al::getSensorPos(pOther), 1.0f);
        al::setNerve(this, &NrvBullAttackSuccess);
    }
}

/**
 * @brief Checks whether the bull can push other enemies.
 * @return True unless the bull is pressed down or blown down.
 */
bool Bull::isEnablePush() const {
    if (al::isNerve(this, &NrvBullPressDown) || al::isNerve(this, &NrvBullBlowDown)) {
        return false;
    }

    return true;
}

/**
 * @brief Checks whether the bull can attack.
 * @return True unless the bull is defeated, trampled or blown away.
 */
bool Bull::isEnableAttack() const {
    if (al::isNerve(this, &NrvBullPressDown) || al::isNerve(this, &NrvBullBlowDown) ||
        al::isNerve(this, &NrvBullTrampled) || al::isNerve(this, &NrvBullBlow)) {
        return false;
    }

    return true;
}

/**
 * @brief Handles attacks, trampling and jump panels.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the bull.
 * @return True if the message was handled.
 */
bool Bull::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (isEnableTrample(pMsg) &&
        al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
        return true;
    }

    if ((al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg) ||
         al::isMsgPlayerBodyLanding(pMsg) || al::isMsgExplosion(pMsg) ||
         al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
         al::isMsgKickKouraAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
         al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgGoalKill(pMsg) ||
         al::isMsgLaserAttack(pMsg)) &&
        isEnableBlowDown()) {
        EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
        al::setNerve(this, &NrvBullBlowDown);
        return true;
    }

    if ((al::isMsgPlayerTailAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
         al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
         al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg) ||
         al::isMsgPlayerFireBallAttack(pMsg)) &&
        al::isSensorEnemyBody(pSelf) && isEnableTrample(pMsg)) {
        mLife--;
        notice();
        if (mLife <= 0) {
            EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
            al::setNerve(this, &NrvBullBlowDown);
            return true;
        }

        rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
        mIsCovered = false;
        mRevivalTimer = cRevivalTime;
        al::setVelocityZeroH(this);
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getSensorPos(pOther),
                                        180.0f);
        const sead::Vector3f& rFront = al::getFront(this);
        al::addVelocityToDirection(this, -rFront, 35.0f);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvBullBlow);
        return true;
    }

    if ((al::isMsgPlayerTrample(pMsg) || al::isMsgBallTrample(pMsg) ||
         al::isMsgPlayerObjHipDropReflectAll(pMsg)) &&
        al::isSensorEnemyBody(pSelf) && isEnableTrample(pMsg) &&
        al::getActorVelocity(pOther).y < 0.0f) {
        mRevivalTimer = cRevivalTime;
        mLife += al::isMsgPlayerObjHipDropReflectAll(pMsg) ? -2 : -1;
        notice();
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        if (mLife <= 0) {
            al::setVelocityZero(this);
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
            al::invalidateClipping(this);
            al::setNerve(this, &NrvBullPressDown);
            if (mIsCovered) {
                al::getSubActor(this, "ヘルメット壊れモデル")->appear();
            }

            return true;
        }

        rc::addScoreCombo(this, pOther, pMsg, cScoreComboFactor);
        mRevivalTimer = cRevivalTime;
        mIsCovered = false;
        al::setVelocityZeroH(this);
        al::addVelocityToDirection(this, al::getFront(this), 25.0f);
        al::setNerve(this, &NrvBullTrampled);
        return true;
    }

    if (rc::isMsgJumpPanelAction(pMsg) && isEnableJumpPanel()) {
        al::setVelocity(this, sead::Vector3f(0.0f, 50.0f, 0.0f));
        al::setNerve(this, &NrvBullJump);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether an attack message can hit the bull.
 * @param pMsg Received message.
 * @return True if the bull can be hit.
 */
bool Bull::isEnableTrample(const al::SensorMsg* pMsg) const {
    if (al::isNerve(this, &NrvBullPressDown) || al::isNerve(this, &NrvBullBlowDown)) {
        return false;
    }

    if (mLife >= 2) {
        if (al::isMsgPlayerBoomerangAttack(pMsg)) {
            return false;
        }
    } else if (al::isMsgPlayerBoomerangReflect(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvBullTrampled) || al::isNerve(this, &NrvBullBlow)) {
        if (al::isMsgPlayerClimbAttack(pMsg)) {
            return al::isGreaterEqualStep(this, 25);
        }

        return al::isGreaterEqualStep(this, 15);
    }

    return true;
}

/**
 * @brief Checks whether the bull can be blown down.
 * @return True unless the bull is already pressed down or blown down.
 */
bool Bull::isEnableBlowDown() const {
    if (al::isNerve(this, &NrvBullPressDown) || al::isNerve(this, &NrvBullBlowDown)) {
        return false;
    }

    return true;
}

/** @brief Marks the bull as having noticed the player and turns on the notice switch. */
void Bull::notice() {
    mIsNoticed = true;
    al::tryOnStageSwitch(this, "SwitchNoticeOn");
}

/**
 * @brief Checks whether the bull can be launched by a jump panel.
 * @return True unless the bull is defeated or already jumping.
 */
bool Bull::isEnableJumpPanel() const {
    if (al::isNerve(this, &NrvBullPressDown) || al::isNerve(this, &NrvBullBlowDown) ||
        al::isNerve(this, &NrvBullJump)) {
        return false;
    }

    return true;
}

/**
 * @brief Handles touch screen attacks and support freezing.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the bull.
 * @return True if the message was handled.
 */
bool Bull::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg) && isEnableBlowDown() && mLife >= 2) {
        mLife--;
        notice();
        mIsCovered = false;
        mRevivalTimer = cRevivalTime;
        al::setVelocityZeroH(this);
        const sead::Vector3f& rFront = al::getFront(this);
        al::addVelocityToDirection(this, -rFront, 35.0f);
        rc::addScoreCombo(this, pPointer, pMsg, cScoreComboFactor);
        al::setNerve(this, &NrvBullBlow);
        return true;
    }

    if (isEnableSupportFreeze() &&
        mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        notice();
        if (!al::isNerve(this, &NrvBullSupportFreeze)) {
            al::setNerve(this, &NrvBullSupportFreeze);
        }

        return true;
    }

    return false;
}

/**
 * @brief Checks whether the bull can be frozen by the support player.
 * @return True if the bull is in a state that can be frozen.
 */
bool Bull::isEnableSupportFreeze() const {
    if (al::isNerve(this, &NrvBullPressDown) || al::isNerve(this, &NrvBullBlowDown) ||
        al::isNerve(this, &NrvBullTrampled) || al::isNerve(this, &NrvBullRevival) ||
        al::isNerve(this, &NrvBullJump) || al::isNerve(this, &NrvBullBlow)) {
        return false;
    }

    return true;
}

/** @brief Counts down the time until the helmet grows back. */
void Bull::control() {
    if (mRevivalTimer > 0) {
        mRevivalTimer--;
    }
}

/** @brief Restores the bull to its initial position and state. */
void Bull::reappear() {
    makeActorAppeared();
    al::resetPosition(this, mInitTrans, false);
    al::setFront(this, mInitFront);
    al::setNerve(this, &NrvBullWait);
    mLife = mMaxLife;
    mRevivalTimer = 0;
    mIsCovered = true;
    mIsNoticed = false;
}

/**
 * @brief Checks whether the charge should end.
 * @return True if the charge timed out or the target left the area in front of the bull.
 */
bool Bull::isEndRun() const {
    if (al::isGreaterEqualStep(this, 600)) {
        return true;
    }

    if (al::isLessStep(this, 60)) {
        return false;
    }

    sead::Vector3f localPos;
    al::multVecInvPose(&localPos, this, al::getTrans(mTarget));
    if (std::fabs(localPos.x) > 400.0f) {
        return true;
    }

    if (localPos.y > 400.0f || localPos.y < -200.0f) {
        return true;
    }

    return localPos.z > 2000.0f || localPos.z < -300.0f;
}

/** @brief Applies gravity in the air and friction to the velocity. */
void Bull::updateVelocity() {
    if (!al::isOnGround(this, 0, 0.0f)) {
        al::addVelocityToGravity(this, cGravity);
    }

    al::scaleVelocity(this, cFriction);
}

/**
 * @brief Searches for a player to charge at.
 * @return True if a target was found.
 */
bool Bull::tryNextTarget() {
    if (!mIsNoticed && al::isValidSwitchStart(this) && !al::isOnSwitchStart(this)) {
        return false;
    }

    mTarget = rc::tryFindNearestActivePlayerActorInCylinder(this, 2000.0f, -500.0f, 500.0f);
    return mTarget != nullptr;
}

/**
 * @brief Starts growing the helmet back once the revival time has passed.
 * @return True if the revival started.
 */
bool Bull::tryRevival() {
    if (mLife >= mMaxLife || mRevivalTimer > 0) {
        return false;
    }

    al::setNerve(this, &NrvBullRevival);
    return true;
}

/** @brief Waits until a player comes close. */
void Bull::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "WaitCoverd" : "WaitNaked");
    }

    updateVelocity();
    if (tryRevival()) {
        return;
    }

    if (tryNextTarget()) {
        al::setNerve(this, &NrvBullFind);
    }
}

/** @brief Turns towards a player that was just found. */
void Bull::exeFind() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "FindCoverd" : "FindNaked");
    }

    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mTarget), 6.0f);
    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBullTurn);
    }
}

/** @brief Charges forward, stopping at walls or when the target is out of range. */
void Bull::exeRun() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "RunCoverd" : "RunNaked");
    }

    if (trySendMsgAttackCollide()) {
        al::scaleVelocityHV(this, 0.6f, 1.0f);
    } else if (al::isCollidedWallVelocity(this) &&
               al::getFront(this).dot(al::getCollidedWallNormal(this)) < -0.5f) {
        al::setVelocityJump(this, 0.0f);
        al::setNerve(this, &NrvBullStop);
        return;
    }

    if (al::isOnGround(this, 5, 0.0f)) {
        const sead::Vector3f& rFront = al::getFront(this);
        f32 rate = al::calcNerveRate(this, 15);
        al::addVelocityToDirection(this, rFront, rate * (mLife == 1 ? 1.2f : 0.9f));
    }

    updateVelocity();
    if (isEndRun()) {
        al::setNerve(this, &NrvBullBrake);
    }
}

/**
 * @brief Attacks the object of a wall the bull ran into.
 * @return True if the attack was received.
 */
bool Bull::trySendMsgAttackCollide() {
    if (al::isCollidedWall(this) &&
        al::getCollidedWallNormal(this).dot(al::getVelocity(this)) < 0.0f) {
        al::HitSensor* pWallSensor = al::tryGetCollidedWallSensor(this);
        if (rc::sendMsgBullAttack(pWallSensor, al::getHitSensor(this, "Body"))) {
            return true;
        }

        if (al::sendMsgBallAttackCollide(pWallSensor, al::getHitSensor(this, "Body"))) {
            return true;
        }
    }

    return false;
}

/** @brief Brakes at the end of a charge, then looks for the next target. */
void Bull::exeBrake() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "BrakeCoverd" : "BrakeNaked");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        mIsNoticed = false;
        if (tryRevival()) {
            return;
        }

        if (tryNextTarget()) {
            al::setNerve(this, &NrvBullTurn);
        } else {
            al::setNerve(this, &NrvBullWait);
        }
    }
}

/** @brief Stops after running into a wall, then looks for the next target. */
void Bull::exeStop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "StopCoverd" : "StopNaked");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        mIsNoticed = false;
        if (tryRevival()) {
            return;
        }

        if (tryNextTarget()) {
            al::setNerve(this, &NrvBullTurn);
        } else {
            al::setNerve(this, &NrvBullWait);
        }
    }
}

/** @brief Turns towards the target before charging. */
void Bull::exeTurn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "TurnCoverd" : "TurnNaked");
    }

    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mTarget), 6.0f);
    updateVelocity();
    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvBullRun);
    }
}

/** @brief Flies through the air after a jump panel until landing. */
void Bull::exeJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "WaitCoverd" : "WaitNaked");
    }

    updateVelocity();
    if (al::isOnGround(this, 0, 0.0f)) {
        tryNextTarget();
        al::setNerve(this, &NrvBullWait);
    }
}

/** @brief Slides back after losing the helmet to an attack. */
void Bull::exeBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Blow");
    }

    const sead::Vector3f& rFront = al::getFront(this);
    al::addVelocityToDirection(this, rFront, al::calcNerveValue(this, 0, 20, 0.5f, 0.0f));
    updateVelocity();
    if (al::isActionEnd(this)) {
        tryNextTarget();
        al::setNerve(this, &NrvBullAngry);
    }
}

/** @brief Slides forward after being trampled. */
void Bull::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Trampled");
    }

    const sead::Vector3f& rFront = al::getFront(this);
    al::addVelocityToDirection(this, rFront, al::calcNerveValue(this, 0, 20, 0.5f, 0.0f));
    updateVelocity();
    if (al::isActionEnd(this)) {
        tryNextTarget();
        al::setNerve(this, &NrvBullTrampledEnd);
    }
}

/** @brief Recovers from being trampled while turning towards the target. */
void Bull::exeTrampledEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TrampledEnd");
        al::setVelocityZero(this);
    }

    updateVelocity();
    if (mTarget != nullptr) {
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mTarget), 6.0f);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBullAngry);
    }
}

/** @brief Gets angry, then charges at the target if there is one. */
void Bull::exeAngry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Angry");
    }

    if (al::isGreaterStep(this, 30) && mTarget != nullptr) {
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mTarget), 6.0f);
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        if (mTarget != nullptr) {
            al::setNerve(this, &NrvBullRun);
        } else {
            al::setNerve(this, &NrvBullWait);
        }
    }
}

/** @brief Grows the helmet back. */
void Bull::exeRevival() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Revival");
    }

    if (al::isStep(this, 25)) {
        mLife++;
        mIsCovered = true;
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        if (tryNextTarget()) {
            al::setNerve(this, &NrvBullTurn);
        } else {
            al::setNerve(this, &NrvBullWait);
        }
    }
}

/** @brief Gets blown away and defeated. */
void Bull::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "BlowDownCoverd" : "BlowDownNaked");
    }

    if (al::updateNerveState(this)) {
        al::startHitReactionDeath(this);
        al::appearItem(this);
        kill();
    }
}

/** @brief Gets pressed flat and defeated. */
void Bull::exePressDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "PressDownCoverd" : "PressDownNaked");
        al::changeEnvTextureStamp(this);
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Plays the reaction after hitting the player, then looks for the next target. */
void Bull::exeAttackSuccess() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mIsCovered ? "AttackSuccessCoverd" : "AttackSuccessNaked");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        if (tryNextTarget()) {
            al::setNerve(this, &NrvBullTurn);
        } else {
            al::setNerve(this, &NrvBullWait);
        }
    }
}

/** @brief Stays frozen by the support player until released. */
void Bull::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBullWait);
    }
}

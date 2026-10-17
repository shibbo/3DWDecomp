#include "Enemy/BrosStateAttack.hpp"
#include "Enemy/BrosFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(BrosStateAttack, Wait)
NERVE_DECL(BrosStateAttack, Turn)
NERVE_DECL(BrosStateAttack, Sign)
NERVE_DECL(BrosStateAttack, Attack)
NERVE_DECL(BrosStateAttack, End)
NERVES_MAKE_NOSTRUCT(BrosStateAttack, Wait, Turn, Sign, Attack, End)

/** @brief Tests whether an actor faces the nearest player horizontally.
 * @param pActor Actor whose front direction is tested.
 * @param degree Maximum angle from the player.
 * @return Whether the player lies within the allowed angle.
 */
bool isFacingPlayer(const al::LiveActor* pActor, f32 degree) {
    sead::Vector3f front;
    al::calcFrontDir(&front, pActor);
    return al::isFaceToTargetDegreeH(pActor, al::findNearestPlayerPos(pActor), front, degree);
}
}

/** @brief Initializes attack animations with fallbacks for missing variants.
 * @param pHost Bros enemy.
 * @param pMoveStepKeeper Platform controller used to select animations and facing.
 */
BrosStateAttack::BrosStateAttack(al::LiveActor* pHost, const BrosMoveStepKeeper* pMoveStepKeeper)
    : al::ActorStateBase("ブロス状態[攻撃]", pHost), mMoveStepKeeper(pMoveStepKeeper) {
    mActions = new Actions;
    mActions->pSign = "AttackSign";
    mActions->pAttack = "Attack";
    mActions->pSecondSign = al::isExistAction(pHost, "AttackSecondSign") ? "AttackSecondSign" : "AttackSign";
    mActions->pSecondAttack = al::isExistAction(pHost, "AttackSecond") ? "AttackSecond" : "Attack";
    mActions->pWait = al::isExistAction(pHost, "AttackWait") ? "AttackWait" : "Wait";
    mActions->pEnd = "Wait";
    mActions->pTurn = "Turn";
    mTowerActions = new Actions;
    mTowerActions->pSign = al::isExistAction(pHost, "TowerAttackSign") ? "TowerAttackSign" : mActions->pSign;
    mTowerActions->pAttack = al::isExistAction(pHost, "TowerAttack") ? "TowerAttack" : mActions->pAttack;
    mTowerActions->pSecondSign = al::isExistAction(pHost, "TowerAttackSecondSign") ? "TowerAttackSecondSign" : mActions->pSecondSign;
    mTowerActions->pSecondAttack = al::isExistAction(pHost, "TowerAttackSecond") ? "TowerAttackSecond" : mActions->pSecondAttack;
    mTowerActions->pWait = al::isExistAction(pHost, "TowerWait") ? "TowerWait" : mActions->pWait;
    mTowerActions->pEnd = al::isExistAction(pHost, "TowerWait") ? "TowerWait" : mActions->pEnd;
    mTowerActions->pTurn = al::isExistAction(pHost, "TowerTurn") ? "TowerTurn" : mActions->pTurn;
    initNerve(&NrvBrosStateAttackWait, 0);
}

/** @brief Resets the attack count and chooses whether to turn first. */
void BrosStateAttack::appear() {
    mAttacksLeft = 2;
    if (!rc::isTurnToDefaultDirectionCurrent(mHostActor, mMoveStepKeeper) &&
        (rc::isFaceToDefaultDirectionCurrent(mHostActor, mMoveStepKeeper) || isFacingPlayer(mHostActor, 30.0f))) {
        al::setNerve(this, &NrvBrosStateAttackSign);
    } else { al::setNerve(this, &NrvBrosStateAttackTurn); }
    al::ActorStateBase::appear();
}

/** @brief Plays the anticipation animation before an attack. */
void BrosStateAttack::exeSign() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentStepKuribo(mMoveStepKeeper)) {
            if (mAttacksLeft % 2 == 0) { al::startAction(mHostActor, mTowerActions->pSign); }
            else { al::startAction(mHostActor, mTowerActions->pSecondSign); }
        } else {
            if (mAttacksLeft % 2 == 0) { al::startAction(mHostActor, mActions->pSign); }
            else { al::startAction(mHostActor, mActions->pSecondSign); }
        }
    }
    if (al::isActionEnd(mHostActor)) { al::setNerve(this, &NrvBrosStateAttackAttack); }
}

/** @brief Performs an attack, then waits, turns, or ends the sequence. */
void BrosStateAttack::exeAttack() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentStepKuribo(mMoveStepKeeper)) {
            if (mAttacksLeft % 2 == 0) { al::startAction(mHostActor, mTowerActions->pAttack); }
            else { al::startAction(mHostActor, mTowerActions->pSecondAttack); }
        } else {
            if (mAttacksLeft % 2 == 0) { al::startAction(mHostActor, mActions->pAttack); }
            else { al::startAction(mHostActor, mActions->pSecondAttack); }
        }
    }
    if (al::isActionEnd(mHostActor)) {
        if (--mAttacksLeft <= 0) { al::setNerve(this, &NrvBrosStateAttackEnd); return; }
        if (rc::isFaceToDefaultDirectionCurrent(mHostActor, mMoveStepKeeper)) {
            al::setNerve(this, &NrvBrosStateAttackWait);
            return;
        }
        if (isFacingPlayer(mHostActor, 90.0f)) { al::setNerve(this, &NrvBrosStateAttackWait); }
        else { al::setNerve(this, &NrvBrosStateAttackTurn); }
    }
}

/** @brief Faces the target during the short pause between attacks. */
void BrosStateAttack::exeWait() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentStepKuribo(mMoveStepKeeper)) { al::startAction(mHostActor, mTowerActions->pWait); }
        else { al::startAction(mHostActor, mActions->pWait); }
    }
    rc::turnToTargetOrDefaultDirectionCurrent(mHostActor, mMoveStepKeeper, 8.0f);
    if (al::isStep(this, 10)) { al::setNerve(this, &NrvBrosStateAttackSign); }
}

/** @brief Turns over the animation duration toward the player or fixed platform direction. */
void BrosStateAttack::exeTurn() {
    if (al::isFirstStep(this)) {
        const char* pAction = (rc::isCurrentStepKuribo(mMoveStepKeeper) ? mTowerActions : mActions)->pTurn;
        al::startAction(mHostActor, pAction);
        sead::Vector3f front;
        sead::Vector3f targetDir;
        if (rc::isCurrentStepPoseFixed(mMoveStepKeeper)) {
            al::calcQuatFront(&targetDir, rc::getCurrentKeyQuat(mMoveStepKeeper));
        } else { targetDir = al::findNearestPlayerPos(mHostActor) - al::getTrans(mHostActor); }
        sead::Vector3f gravity(al::getGravity(mHostActor));
        al::calcFrontDir(&front, mHostActor);
        al::verticalizeVec(&front, gravity, front);
        al::verticalizeVec(&targetDir, gravity, targetDir);
        const f32 angle = al::calcAngleOnPlaneDegree(front, targetDir, gravity);
        mTurnDegree = angle / al::getActionFrameMax(mHostActor, pAction);
    }
    if (rc::isCurrentStepPoseFixed(mMoveStepKeeper)) {
        sead::Vector3f front;
        al::calcQuatFront(&front, rc::getCurrentKeyQuat(mMoveStepKeeper));
        al::turnToDirection(mHostActor, front, mTurnDegree);
    } else { al::turnToTarget(mHostActor, al::findNearestPlayerPos(mHostActor), mTurnDegree); }
    if (al::isActionEnd(mHostActor)) { al::setNerve(this, &NrvBrosStateAttackSign); }
}

/** @brief Waits briefly after the final attack before ending the state. */
void BrosStateAttack::exeEnd() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentStepKuribo(mMoveStepKeeper)) { al::startAction(mHostActor, mTowerActions->pEnd); }
        else { al::startAction(mHostActor, mActions->pEnd); }
    }
    if (al::isStep(this, 30)) { kill(); }
}

/** @brief Tests the first frame of an attack.
 * @return Whether the attack nerve is on frame zero.
 */
bool BrosStateAttack::isAttackStep() const {
    if (al::isNerve(this, &NrvBrosStateAttackAttack)) {
        if (al::isStep(this, 0)) { return true; }
    }
    return false;
}

/** @brief Tests entry into the anticipation state.
 * @return Whether anticipation is new or on its first frame.
 */
bool BrosStateAttack::isSignStep() const {
    if (al::isNerve(this, &NrvBrosStateAttackSign)) {
        if (al::isFirstStep(this)) { return true; }
        if (al::isNewNerve(this)) { return true; }
    }
    return false;
}

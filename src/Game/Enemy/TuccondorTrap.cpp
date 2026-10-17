#include "Enemy/TuccondorTrap.hpp"
#include "Enemy/TuccondorGlass.hpp"
#include "Enemy/TuccondorGlassBlower.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(TuccondorTrap, Wait)
NERVE_DECL(TuccondorTrap, SupportFreeze)
NERVE_DECL(TuccondorTrap, Attack)
NERVE_DECL(TuccondorTrap, AttackWait)
NERVE_DECL(TuccondorTrap, AttackEnd)
NERVE_DECL(TuccondorTrap, AttackSign)
NERVES_MAKE_STRUCT(TuccondorTrap, Wait, SupportFreeze)
NERVES_MAKE_NOSTRUCT(TuccondorTrap, Attack, AttackWait, AttackEnd, AttackSign)
ActorStateSupportFreezeParam sFreezeParam(true, 15, false, true, 120, sead::Vector3f(0.0f, 150.0f, 0.0f));
}
/** @brief Creates a Conkdor trap. @param pName Actor name. */
TuccondorTrap::TuccondorTrap(const char* pName) : TuccondorBase(pName) {}
/** @brief Initializes collision attachment, glasses, and support states. @param rInfo Placement information. */
void TuccondorTrap::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    const char* pModelName = "Tuccondor";
    alPlacementFunction::tryGetModelName(&pModelName, rInfo);
    al::initActorWithArchiveNameWithPlacementInfo(this, rInfo, pModelName, nullptr);
    mConnector = al::createMtxConnector(this);
    mFreeze = new ActorStateSupportFreeze(this, &sFreezeParam);
    al::initNerve(this, &NrvTuccondorTrap.Wait, 5);
    al::initNerveState(this, mFreeze, &NrvTuccondorTrap.SupportFreeze, "\u30d5\u30ea\u30fc\u30ba");
    initNerveState();
    mGlass = new TuccondorGlass("\u30b5\u30f3\u30b0\u30e9\u30b9");
    mGlass->init(rInfo);
    mGlass->makeActorDead();
    mGlassBlower = new TuccondorGlassBlower(this, mGlass);
    al::tryGetArg(&mShortAttack, rInfo, "IsTuccondorAttackShort");
    al::trySetShadowLength(this, rInfo, nullptr);
    makeActorAppeared();
}
/** @brief Attaches the trap to the surface beneath it. */
void TuccondorTrap::initAfterPlacement() {
    if (mConnector) {
        al::offCollide(this);
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
    }
}
/** @brief Updates glasses and follows the attached surface. */
void TuccondorTrap::control() {
    mGlassBlower->update();
    if (mConnector && !isNerveDead()) al::connectPoseQT(this, mConnector);
}
/** @brief Applies head, neck, and body contact behavior. @param pSelf Own sensor. @param pOther Contact sensor. */
void TuccondorTrap::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isNerveDead()) return;
    if (al::isSensorEnemyBody(pOther) && (al::isSensorName(pSelf, "AttackHead") || al::isSensorName(pSelf, "AttackBody") || isHitNeckSensor(pSelf, pOther, _150))) al::sendMsgPush(pOther, pSelf);
    if (al::isSensorPlayer(pOther) && al::isSensorName(pSelf, "AttackBody")) al::sendMsgPush(pOther, pSelf);
    if (al::isNerve(this, &NrvTuccondorTrapAttack)) {
        if (al::isSensorName(pSelf, "AttackHead") && (rc::sendMsgTuccondorAttack(pOther, pSelf) || al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf))) return;
        if (isHitNeckSensor(pSelf, pOther, _150)) { al::sendMsgEnemyAttack(pOther, pSelf); return; }
    } else if (!(GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && al::isSensorPlessie(pOther)) && (al::isSensorName(pSelf, "AttackHead") || isHitNeckSensor(pSelf, pOther, _150))) { al::sendMsgPush(pOther, pSelf); return; }
    if (al::isSensorName(pSelf, "AttackBody")) al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
}
/** @brief Handles knockback, trampling, and laser attacks. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool TuccondorTrap::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (isNerveDead()) return false;
    if (al::isNerve(this, &NrvTuccondorTrap.Wait) && al::isSensorPlayer(pOther) && al::isSensorName(pSelf, "AttackSearch")) {
        al::setNerve(this, &NrvTuccondorTrapAttackSign);
        return false;
    }
    if (al::isSensorName(pSelf, "AttackSearch")) return false;
    if (!al::isSensorEnemyBody(pSelf) && !isHitNeckSensor(pSelf, pOther, _154)) return false;
    if (tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf)) { rc::addScoreCombo(this, pOther, pMsg, 100.0f); return true; }
    if (al::isNerve(this, &NrvTuccondorTrapAttack) || (al::isNerve(this, &NrvTuccondorTrapAttackEnd) && al::isGreaterStep(this, 30)) || al::isSensorName(pSelf, "Body")) return false;
    bool standing = al::isNerve(this, &NrvTuccondorTrap.Wait);
    if ((!al::isMsgPlayerTrample(pMsg) || rc::getPlayerVelocity(pOther).y < 0.0f) && tryRequestPressDown(pMsg, pOther, pSelf, standing)) { rc::addScoreCombo(this, pOther, pMsg, 100.0f); return true; }
    if (al::isMsgLaserAttack(pMsg)) { al::startHitReactionDeath(this); kill(); return true; }
    return false;
}
/** @brief Routes touches to body freeze or glasses. @param pMsg Message. @param pPointer Touch pointer. @param pTarget Touch target. @return Whether handled. */
bool TuccondorTrap::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (isNerveDead()) return false;
    if (al::isEqualString(pTarget->getName(), "Body")) {
        if (!mFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) return false;
        if (!al::isNerve(this, &NrvTuccondorTrap.SupportFreeze)) al::setNerve(this, &NrvTuccondorTrap.SupportFreeze);
        return true;
    }
    if (al::isEqualString(pTarget->getName(), "Head") || al::isEqualString(pTarget->getName(), "Beak")) return mGlassBlower->tryRequestGlassBlow(pMsg, pPointer, pTarget);
    return false;
}
/** @brief Waits for a player to enter the search sensor. */
void TuccondorTrap::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
/** @brief Pauses briefly before striking. */
void TuccondorTrap::exeAttackSign() {
    if (al::isFirstStep(this)) al::startAction(this, mShortAttack ? "AttackShortTrap" : "AttackTrap");
    if (al::isGreaterStep(this, 10)) al::setNerve(this, &NrvTuccondorTrapAttack); }
/** @brief Strikes with the beak. */
void TuccondorTrap::exeAttack() {
    if (al::isFirstStep(this)) al::setVelocityZero(this);
    if (al::isStep(this, 32)) requestHitReactionAttack();
    if (al::isActionEnd(this)) al::setNerve(this, &NrvTuccondorTrapAttackWait);
}
/** @brief Holds the beak down briefly. */
void TuccondorTrap::exeAttackWait() {
    if (al::isFirstStep(this)) al::startAction(this, mShortAttack ? "AttackShortWait" : "AttackWait");
    if (al::isGreaterStep(this, 20)) al::setNerve(this, &NrvTuccondorTrapAttackEnd);
}
/** @brief Raises the head before resuming tracking. */
void TuccondorTrap::exeAttackEnd() {
    if (al::isFirstStep(this)) al::startAction(this, mShortAttack ? "AttackShortEnd" : "AttackEnd");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvTuccondorTrap.Wait);
}
/** @brief Updates the support freeze state. */
void TuccondorTrap::exeSupportFreeze() { if (al::updateNerveState(this)) al::setNerve(this, &NrvTuccondorTrap.Wait); }

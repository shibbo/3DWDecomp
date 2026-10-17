#include "Enemy/TuccondorAround.hpp"
#include "Enemy/TuccondorGlass.hpp"
#include "Enemy/TuccondorGlassBlower.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(TuccondorAround, Wait)
NERVE_DECL(TuccondorAround, SupportFreeze)
NERVE_DECL(TuccondorAround, Attack)
NERVE_DECL(TuccondorAround, AttackWait)
NERVE_DECL(TuccondorAround, AttackEnd)
NERVE_DECL(TuccondorAround, AttackSign)
NERVES_MAKE_STRUCT(TuccondorAround, Wait, SupportFreeze)
NERVES_MAKE_NOSTRUCT(TuccondorAround, Attack, AttackWait, AttackEnd, AttackSign)
ActorStateSupportFreezeParam sFreezeParam(true, 15, false, true, 120, sead::Vector3f(0.0f, 150.0f, 0.0f));
}
/** @brief Creates a rail-targeting Conkdor. @param pName Actor name. */
TuccondorAround::TuccondorAround(const char* pName) : TuccondorBase(pName) {}
/** @brief Initializes the target rail, glasses, and support states. @param rInfo Placement information. */
void TuccondorAround::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTFSV(this);
    const char* pModelName = "Tuccondor";
    alPlacementFunction::tryGetModelName(&pModelName, rInfo);
    al::initActorWithArchiveName(this, rInfo, pModelName, nullptr);
    al::invalidateHitSensor(this, "AttackSearch");
    if (!al::isExistRail(this)) { makeActorDead(); return; }
    al::tryGetArg(&mTargetSpeed, rInfo, "TargetMoveSpeed");
    al::tryGetArg(&mTargetOffset, rInfo, "TargetRailCoordOffset");
    al::tryGetArg(&mAttackWaitTime, rInfo, "AttackWaitTime");
    sead::Vector3f target;
    f32 coord = 0.0f;
    if (al::tryGetLinksTrans(&target, rInfo, "TuccondorAroundTarget")) coord = al::calcNearestRailCoord(this, target);
    al::moveRailLoop(this, coord);
    mGlass = new TuccondorGlass("\u30b5\u30f3\u30b0\u30e9\u30b9");
    mGlass->init(rInfo);
    mGlass->makeActorDead();
    mGlassBlower = new TuccondorGlassBlower(this, mGlass);
    mFreeze = new ActorStateSupportFreeze(this, &sFreezeParam);
    al::initNerve(this, &NrvTuccondorAround.Wait, 5);
    al::initNerveState(this, mFreeze, &NrvTuccondorAround.SupportFreeze, "\u30d5\u30ea\u30fc\u30ba");
    initNerveState();
    makeActorAppeared();
}
/** @brief Advances the target and turns toward it between attacks. */
void TuccondorAround::control() {
    mGlassBlower->update();
    al::moveRailLoop(this, mTargetSpeed);
    if (al::isNerve(this, &NrvTuccondorAroundAttack) || al::isNerve(this, &NrvTuccondorAroundAttackWait) || al::isNerve(this, &NrvTuccondorAroundAttackEnd) || isNerveDead() || al::isNerve(this, &NrvTuccondorAround.SupportFreeze)) return;
    f32 coord = al::getRailCoord(this) + mTargetOffset;
    if (coord < 0.0f) coord += al::getRailTotalLength(this);
    sead::Vector3f target;
    al::calcRailPosAtCoord(&target, this, coord);
    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), target, 5.0f);
}
/** @brief Applies head, neck, and body contact behavior. @param pSelf Own sensor. @param pOther Contact sensor. */
void TuccondorAround::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isNerveDead()) return;
    if (al::isSensorEnemyBody(pOther) && (al::isSensorName(pSelf, "AttackHead") || al::isSensorName(pSelf, "AttackBody") || isHitNeckSensor(pSelf, pOther, _150))) al::sendMsgPush(pOther, pSelf);
    if (al::isSensorPlayer(pOther) && al::isSensorName(pSelf, "AttackBody")) al::sendMsgPush(pOther, pSelf);
    bool attacking = al::isNerve(this, &NrvTuccondorAroundAttack);
    bool head = al::isSensorName(pSelf, "AttackHead");
    if (attacking) {
        if (head && (rc::sendMsgTuccondorAttack(pOther, pSelf) || al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf))) return;
        if (isHitNeckSensor(pSelf, pOther, _150)) { al::sendMsgEnemyAttack(pOther, pSelf); return; }
    } else if (head || isHitNeckSensor(pSelf, pOther, _150)) { al::sendMsgPush(pOther, pSelf); return; }
    if (al::isSensorName(pSelf, "AttackBody")) al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
}
/** @brief Handles knockback, trampling, and laser attacks. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool TuccondorAround::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (isNerveDead() || al::isSensorName(pSelf, "AttackSearch")) return false;
    if (!al::isSensorEnemyBody(pSelf) && !isHitNeckSensor(pSelf, pOther, _154)) return false;
    if (tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf)) { rc::addScoreCombo(this, pOther, pMsg, 100.0f); return true; }
    if (al::isNerve(this, &NrvTuccondorAroundAttack) || (al::isNerve(this, &NrvTuccondorAroundAttackEnd) && al::isGreaterStep(this, 30)) || al::isSensorName(pSelf, "Body")) return false;
    bool standing = al::isNerve(this, &NrvTuccondorAround.Wait);
    if ((!al::isMsgPlayerTrample(pMsg) || rc::getPlayerVelocity(pOther).y < 0.0f) && tryRequestPressDown(pMsg, pOther, pSelf, standing)) { rc::addScoreCombo(this, pOther, pMsg, 100.0f); return true; }
    if (al::isMsgLaserAttack(pMsg)) { al::startHitReactionDeath(this); kill(); return true; }
    return false;
}
/** @brief Routes touches to body freeze or glasses. @param pMsg Message. @param pPointer Touch pointer. @param pTarget Touch target. @return Whether handled. */
bool TuccondorAround::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (isNerveDead()) return false;
    if (al::isEqualString(pTarget->getName(), "Body")) {
        if (!mFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) return false;
        if (!al::isNerve(this, &NrvTuccondorAround.SupportFreeze)) al::setNerve(this, &NrvTuccondorAround.SupportFreeze);
        return true;
    }
    if (al::isEqualString(pTarget->getName(), "Head") || al::isEqualString(pTarget->getName(), "Beak")) return mGlassBlower->tryRequestGlassBlow(pMsg, pPointer, pTarget);
    return false;
}
/** @brief Tracks the rail before preparing an attack. */
void TuccondorAround::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Walk");
    if (al::isGreaterStep(this, 30)) al::setNerve(this, &NrvTuccondorAroundAttackSign);
}
/** @brief Pauses briefly before striking. */
void TuccondorAround::exeAttackSign() { if (al::isGreaterStep(this, 10)) al::setNerve(this, &NrvTuccondorAroundAttack); }
/** @brief Strikes with the beak. */
void TuccondorAround::exeAttack() {
    if (al::isFirstStep(this)) al::startAction(this, "Attack");
    if (al::isStep(this, 12)) requestHitReactionAttack();
    if (al::isActionEnd(this)) al::setNerve(this, &NrvTuccondorAroundAttackWait);
}
/** @brief Holds the beak down for the configured duration. */
void TuccondorAround::exeAttackWait() {
    if (al::isFirstStep(this)) al::startAction(this, "AttackWait");
    if (al::isGreaterStep(this, mAttackWaitTime)) al::setNerve(this, &NrvTuccondorAroundAttackEnd);
}
/** @brief Raises the head before resuming tracking. */
void TuccondorAround::exeAttackEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "AttackEnd");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvTuccondorAround.Wait);
}
/** @brief Updates the support freeze state. */
void TuccondorAround::exeSupportFreeze() { if (al::updateNerveState(this)) al::setNerve(this, &NrvTuccondorAround.Wait); }

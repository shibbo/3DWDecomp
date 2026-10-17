#include "Enemy/Fugumannen.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Fugumannen, Accelerate)
NERVE_DECL(Fugumannen, SwimBlowDown)
NERVE_DECL(Fugumannen, SupportFreeze)
NERVE_DECL(Fugumannen, Turn)
NERVE_DECL(Fugumannen, Brake)
NERVE_DECL(Fugumannen, SwimWait)
NERVES_MAKE_STRUCT(Fugumannen, Accelerate, SwimBlowDown, SupportFreeze)
NERVES_MAKE_NOSTRUCT(Fugumannen, Turn, Brake, SwimWait)
}
/** @brief Creates a rail-swimming Porcupuffer. @param pName Actor name. */
Fugumannen::Fugumannen(const char* pName) : LiveActor(pName) {}
/** @brief Initializes rail swimming and auxiliary states. @param rInfo Placement information. */
void Fugumannen::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::tryGetArg(&mSwimSpeed, rInfo, "SwimSpeed");
    if (!al::isExistRail(this)) { makeActorDead(); return; }
    al::initNerve(this, &NrvFugumannen.Accelerate, 2);
    al::setSyncRailToStart(this);
    al::startAction(this, "SwimWait");
    calcBrakeDistance();
    al::offCollide(this);
    mBlowDownParam = new EnemyStateBlowDownParam(true);
    mBlowDown = new EnemyStateBlowDown(this, mBlowDownParam);
    al::initNerveState(this, mBlowDown, &NrvFugumannen.SwimBlowDown, "state:BlowDown");
    mFreezeParam = new ActorStateSupportFreezeParam(true, 15, false, true, 120, {0.0f, 150.0f, 0.0f});
    mFreeze = new ActorStateSupportFreeze(this, mFreezeParam);
    al::initNerveState(this, mFreeze, &NrvFugumannen.SupportFreeze, "state:SupportFreeze");
    makeActorAppeared();
}
/** @brief Computes stopping distance for thirty frames of braking. */
void Fugumannen::calcBrakeDistance() { mBrakeDistance = (mSwimSpeed * 30.0f) * 0.5f; }
/** @brief Restores swimming after an attack animation and refreshes braking distance. */
void Fugumannen::control() {
    if (al::isActionPlaying(this, "AttackSuccess") && al::isActionEnd(this)) al::startAction(this, "SwimWait");
    calcBrakeDistance();
}
/** @brief Attacks players while active. @param pSelf Attacking sensor. @param pOther Player sensor. */
void Fugumannen::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvFugumannen.SwimBlowDown)) return;
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) && al::sendMsgEnemyAttack(pOther, pSelf) &&
        !al::isActionPlaying(this, "AttackSuccess")) al::startAction(this, "AttackSuccess");
}
/** @brief Requests defeat from attacks on the body.
 * @param pMsg Incoming message. @param pOther Attacking sensor. @param pSelf Receiving sensor.
 * @return Whether defeat started. */
bool Fugumannen::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvFugumannen.SwimBlowDown)) return false;
    if (al::isSensorEnemyBody(pSelf) && EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mBlowDown, &NrvFugumannen.SwimBlowDown, false)) {
        al::onCollide(this);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        return true;
    }
    return false;
}
/** @brief Routes touch input to the support-freeze state.
 * @param pMsg Touch message. @param pPointer Touch pointer. @param pTarget Selected target.
 * @return Whether the freeze state accepted the input. */
bool Fugumannen::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvFugumannen.SwimBlowDown)) return false;
    if (mFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvFugumannen.SupportFreeze)) al::setNerve(this, &NrvFugumannen.SupportFreeze);
        return true;
    }
    return false;
}
/** @brief Swims at full speed and starts braking near a rail endpoint. */
void Fugumannen::exeSwimWait() {
    al::moveSyncRailTurn(this, mSwimSpeed);
    sead::Vector3f dir = al::getRailDir(this);
    if (!al::isRailGoingToEnd(this)) dir = -dir;
    al::turnQuatFrontToDirDegreeH(this, dir, 2.6f);
    if (al::isRailReachedEdge(this)) al::setNerve(this, &NrvFugumannenTurn);
    else if (al::isRailReachedNearGoal(this, mBrakeDistance)) al::setNerve(this, &NrvFugumannenBrake);
}
/** @brief Updates rail movement and orientation during turn. */
void Fugumannen::exeTurn() {
    sead::Vector3f dir = al::getRailDir(this);
    if (!al::isRailGoingToEnd(this)) dir = -dir;
    al::turnQuatFrontToDirDegreeH(this, dir, 2.6f);
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, this);
    if ((dir.y > 0.0f ? dir.y : -dir.y) > 0.95f && al::isNearAngleDegree(up, sead::Vector3f::ey, 30.0f) ) {
        if (dir.y > 0.0f) al::rotateQuatXDirDegree(this, -0.6f);
        else if (dir.y < 0.0f) al::rotateQuatXDirDegree(this, 0.6f);
    }
    if (al::isGreaterEqualStep(this, 20)) al::setNerve(this, &NrvFugumannen.Accelerate);
}
/** @brief Updates rail movement and orientation during accelerate. */
void Fugumannen::exeAccelerate() {
    al::moveSyncRailTurn(this, al::lerpValue(al::getNerveStep(this) / 30.0f, 0.0f, mSwimSpeed));
    sead::Vector3f dir = al::getRailDir(this);
    if (!al::isRailGoingToEnd(this)) dir = -dir;
    al::turnQuatFrontToDirDegreeH(this, dir, 2.6f);
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, this);
    if ((dir.y > 0.0f ? dir.y : -dir.y) > 0.95f && al::isNearAngleDegree(up, sead::Vector3f::ey, 30.0f) ) {
        if (dir.y > 0.0f) al::rotateQuatXDirDegree(this, -0.6f);
        else if (dir.y < 0.0f) al::rotateQuatXDirDegree(this, 0.6f);
    }
    if (al::isGreaterEqualStep(this, 29)) al::setNerve(this, &NrvFugumannenSwimWait);
}
/** @brief Updates rail movement and orientation during brake. */
void Fugumannen::exeBrake() {
    f32 speed = al::lerpValue(al::getNerveStep(this) / 30.0f, mSwimSpeed, 0.0f);
    if (!al::isRailReachedEdge(this)) al::moveSyncRailTurn(this, speed);
    sead::Vector3f dir = al::getRailDir(this);
    if (!al::isRailReachedEdge(this)) dir = -dir;
    if (!al::isRailGoingToEnd(this)) dir = -dir;
    al::turnQuatFrontToDirDegreeH(this, dir, 2.6f);
    if ((dir.y > 0.0f ? dir.y : -dir.y) > 0.95f ) {
        if (dir.y > 0.0f) al::rotateQuatXDirDegree(this, -0.6f);
        else if (dir.y < 0.0f) al::rotateQuatXDirDegree(this, 0.6f);
    }
    if (al::isGreaterEqualStep(this, 29)) al::setNerve(this, &NrvFugumannenTurn);
}
/** @brief Drops the reward and removes the enemy after its defeat state ends. */
void Fugumannen::exeSwimBlowDown() {
    if (al::updateNerveState(this)) { al::appearItem(this); al::startHitReactionDeath(this); kill(); }
}
/** @brief Resumes swimming after support freezing ends. */
void Fugumannen::exeSupportFreeze() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvFugumannenSwimWait);
}

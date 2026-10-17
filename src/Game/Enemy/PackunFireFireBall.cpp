#include "Enemy/PackunFireFireBall.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(PackunFireFireBall, Move)
NERVE_DECL(PackunFireFireBall, RouteDokan)
NERVES_MAKE_STRUCT(PackunFireFireBall, Move, RouteDokan)
}
/** @brief Creates a fireball for a Fire Piranha Plant. @param pHost Firing actor. */
PackunFireFireBall::PackunFireFireBall(al::LiveActor* pHost) : LiveActor("\u30d5\u30a1\u30a4\u30a2\u30d1\u30c3\u30af\u30f3\u30d5\u30a1\u30a4\u30a2\u30dc\u30fc\u30eb"), mHost(pHost) { getName(); }
/** @brief Initializes the model and pipe travel state. @param rInfo Host placement information. */
void PackunFireFireBall::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "PackunFireFireBall", nullptr);
    al::initNerve(this, &NrvPackunFireFireBall.Move, 1);
    mRouteState = new ActorStateRouteDokanMove(this, rInfo);
    al::initNerveState(this, mRouteState, &NrvPackunFireFireBall.RouteDokan, "[state]\u30eb\u30fc\u30c8\u571f\u7ba1\u79fb\u52d5");
    makeActorDead();
}
/** @brief Enters pipes or sends fire damage to eligible targets.
 * @param pSelf Fireball sensor. @param pOther Target sensor. */
void PackunFireFireBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvPackunFireFireBall.Move) && mRouteState->tryStart(pSelf, pOther)) {
        al::offCollide(this);
        al::setNerve(this, &NrvPackunFireFireBall.RouteDokan);
        return;
    }
    if (!al::isSensorEnemyAttack(pSelf)) return;
    if (!(al::isSensorEnemy(pOther) || al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther) ||
          al::isSensorHostName(pOther, "DoorKey") ||
          (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
           (al::isSensorKickKoura(pOther) || al::isSensorMapObj(pOther))))) return;
    if (al::getSensorHost(pOther) == mHost && !mCanHitHost) return;
    bool hit;
    if (al::isNerve(this, &NrvPackunFireFireBall.RouteDokan)) hit = al::sendMsgEnemyRouteDokanFire(pOther, pSelf);
    else hit = al::sendMsgEnemyAttackFire(pOther, pSelf);
    if (hit) {
        if (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) al::startHitReaction(this, "\u30d7\u30ec\u30a4\u30e4\u30fc\u30d2\u30c3\u30c8");
        else al::startHitReactionDeath(this);
        kill();
    }
}
/** @brief Handles eating and attacks that extinguish the fireball.
 * @param pMsg Incoming message. @param pOther Attacking sensor. @param pSelf Receiving sensor.
 * @return Whether the message was accepted. */
bool PackunFireFireBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgPackunEatStart(pMsg)) return true;
    if (rc::isMsgPackunEat(pMsg)) kill();
    bool extinguish = al::isMsgGoalKill(pMsg);
    if (!extinguish) {
        if (al::isNerve(this, &NrvPackunFireFireBall.RouteDokan))
            extinguish = EnemyStateUtil::isMsgRouteDokanAttack(pMsg) && !al::isMsgEnemyRouteDokanAttack(pMsg);
        else extinguish = al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg) ||
                          al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgKeyThrow(pMsg);
        if (!extinguish) extinguish = al::isMsgNekoAttack(pMsg);
    }
    if (extinguish) { al::startHitReactionDeath(this); kill(); return true; }
    return false;
}
/** @brief Deflects a free-flying fireball along a touch slide.
 * @param pMsg Touch message. @param pPointer Touch pointer. @param pTarget Selected target.
 * @return Whether the fireball was deflected. */
bool PackunFireFireBall::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssist(pMsg)) return false;
    if (!al::isNerve(this, &NrvPackunFireFireBall.Move)) return false;
    if (mIsDeflected) return false;
    sead::Vector3f dir = sead::Vector3f::zero;
    const al::LiveActor* pTouchActor = DrcFunction::tryFindDrcTouchActor(this, pPointer);
    if (!pTouchActor) return false;
    if (!rc::tryCalcTouchPointerSlideDirOnWorldByPointer(&dir, pTouchActor)) return false;
    dir.y = 0.0f;
    al::normalizeOrZero(&dir);
    if (al::isNearZero(dir, 0.001f)) return false;
    al::setVelocityToDirection(this, dir, 8.2f);
    al::makeQuatFrontUp(al::getQuatPtr(this), dir, sead::Vector3f::ey);
    mIsDeflected = true;
    mCanHitHost = true;
    return true;
}
/** @brief Removes a live fireball when its switch turns off. */
void PackunFireFireBall::killBySwitch() {
    if (al::isDead(this)) return;
    al::startHitReaction(this, "\u81ea\u7136\u6d88\u6ec5");
    kill();
}
/** @brief Sets the launch position, orientation, and initial size.
 * @param pMtx Launch matrix. @param rQuat Launch orientation. @param offset Forward launch offset. */
void PackunFireFireBall::setParam(const sead::Matrix34f* pMtx, const sead::Quatf& rQuat, f32 offset) {
    sead::Vector3f pos;
    pMtx->getTranslation(pos);
    al::setTrans(this, pos);
    al::setQuat(this, rQuat);
    sead::Vector3f front = sead::Vector3f::zero;
    al::calcQuatFront(&front, this);
    al::setVelocityToDirection(this, front, 8.2f);
    al::setTrans(this, front * offset + pos);
    al::setScale(this, 0.1f, 0.1f, 0.1f);
    mMoveStep = 0;
    mCollisionDelay = 0;
    mIsDeflected = false;
    mCanHitHost = false;
}
/** @brief Activates the fireball and starts its travel sound. */
void PackunFireFireBall::appear() {
    if (al::isAlive(this)) kill();
    makeActorAppeared();
    al::startHitReactionAppear(this);
    al::invalidateClipping(this);
    al::onCollide(this);
    al::setNerve(this, &NrvPackunFireFireBall.Move);
    al::startSe(this, "Moving", nullptr);
}
/** @brief Grows the fireball and advances its free-flight lifetime. */
void PackunFireFireBall::exeMove() {
    if (mMoveStep < 10) {
        f32 scale = mMoveStep * 0.9f / 10.0f + 0.1f;
        al::setScale(this, scale, scale, scale);
    }
    if (mCollisionDelay > 0 && al::isNoCollide(this)) {
        if (--mCollisionDelay <= 0) al::onCollide(this);
    }
    tryKill(mMoveStep);
    ++mMoveStep;
}
/** @brief Checks wall collisions and lifetime expiry. @param step Lifetime counter. @return Whether the fireball died. */
bool PackunFireFireBall::tryKill(s32 step) {
    if (al::isCollided(this)) {
        al::HitSensor* pWall = al::tryGetCollidedWallSensor(this);
        if (pWall) al::sendMsgEnemyAttackFire(pWall, al::getHitSensor(this, "Attack"));
        al::startHitReactionDeath(this);
        kill();
        return true;
    }
    if (step >= 250) {
        al::startHitReaction(this, "\u81ea\u7136\u6d88\u6ec5");
        kill();
        return true;
    }
    return false;
}
/** @brief Follows the pipe and restores free flight after exiting. */
void PackunFireFireBall::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        mRouteState->setMoveSpeed(12.0f);
        mRouteState->setEndSpeed(8.2f);
        al::setScaleAll(this, 1.0f);
    }
    if (al::isIntervalStep(this, 3, 0) && tryKill(++mMoveStep)) return;
    sead::Quatf* pQuat = al::getQuatPtr(this);
    al::turnQuatZDirRadian(pQuat, *pQuat, mRouteState->getMoveDirection(), 3.1415927f);
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvPackunFireFireBall.Move);
        mCollisionDelay = 10;
        mCanHitHost = true;
    }
}


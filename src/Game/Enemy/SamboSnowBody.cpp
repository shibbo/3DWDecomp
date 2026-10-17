#include "Enemy/SamboSnowBody.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "MapObj/BallSnow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(SamboSnowBody, Wait)
NERVE_DECL(SamboSnowBody, SupportFreeze)
NERVE_DECL(SamboSnowBody, BlowDown)
NERVE_DECL(SamboSnowBody, FireDown)
NERVES_MAKE_STRUCT(SamboSnowBody, Wait, SupportFreeze)
NERVES_MAKE_NOSTRUCT(SamboSnowBody, BlowDown, FireDown)
ActorStateSupportFreezeParam sFreezeParam(true, 15, false, false, 120, sead::Vector3f(0.0f, 150.0f, 0.0f));
}
/** @brief Creates a snow Pokey body. @param pName Actor name. */
SamboSnowBody::SamboSnowBody(const char* pName) : al::LiveActor(pName) {}
/** @brief Creates the released snowball and freeze state. @param rInfo Placement information. */
void SamboSnowBody::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "SamboSnowBody", nullptr);
    mFreeze = new ActorStateSupportFreeze(this, &sFreezeParam);
    al::initNerve(this, &NrvSamboSnowBody.Wait, 1);
    al::initNerveState(this, mFreeze, &NrvSamboSnowBody.SupportFreeze, "[state]\u30d5\u30ea\u30fc\u30ba");
    mSnowBall = new BallSnow("\u96ea\u7389");
    al::initCreateActorNoPlacementInfo(mSnowBall, rInfo);
    mSnowBall->makeActorDead();
    al::invalidateClipping(this);
    al::offCollide(this);
    makeActorAppeared();
}
/** @brief Removes the body and released snowball. @param flag Completion flag. */
void SamboSnowBody::killComplete(bool flag) {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && mSnowBall->isPlayerHold()) mSnowBall->requestPlayerRelease();
    mSnowBall->makeActorDead();
    kill();
}
/** @brief Breaks apart inside an ink limit sphere. */
void SamboSnowBody::control() {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && InkUtil::isInInkLimitSphere(this) && !al::isNerve(this, &NrvSamboSnowBodyBlowDown)) requestBlowDown();
}
/** @brief Begins the fall using the current velocity. */
void SamboSnowBody::requestBlowDown() {
    al::invalidateHitSensors(this);
    mBlowVelocity = al::getVelocity(this);
    al::setNerve(this, &NrvSamboSnowBodyBlowDown);
}
/** @brief Restores the stacked body. */
void SamboSnowBody::reappear() {
    al::startAction(this, "Wait");
    makeActorAppeared();
    mSnowBall->makeActorDead();
    al::setNerve(this, &NrvSamboSnowBody.Wait);
    al::validateHitSensors(this);
}
/** @brief Pushes contacts and attacks players. @param pSelf Own sensor. @param pOther Contact sensor. */
void SamboSnowBody::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvSamboSnowBodyBlowDown)) return;
    if (al::isSensorEnemyBody(pOther) || al::isSensorNpc(pOther)) { al::sendMsgPush(pOther, pSelf); return; }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (al::isSensorHostName(pOther, "\u30b3\u30a6\u30e9") || al::isSensorHostName(pOther, "BallNeko")) al::sendMsgPushStrong(pOther, pSelf);
        if (al::isSensorName(pSelf, "Attack") && (al::isSensorHostName(pOther, "\u96ea\u7389") || al::isSensorHostName(pOther, "Spinner"))) al::sendMsgPush(pOther, pSelf);
    }
    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) && al::isSensorName(pSelf, "Attack")) {
        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) *mHeadIsRequestAttack = true;
    }
}
/** @brief Handles fire and knockback attacks. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool SamboSnowBody::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvSamboSnowBodyBlowDown) || !*mHeadIsStacked) return false;
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvSamboSnowBodyFireDown);
        *mHeadIsStacked = false;
        return true;
    }
    if (EnemyStateUtil::isMsgBlowDown(pMsg) && !al::isMsgPlayerCooperationHipDrop(pMsg)) {
        al::setVelocitySeparateHV(this, al::getTrans(this) - al::getSensorPos(pOther), 8.0f, 30.0f);
        *mHeadIsStacked = false;
        al::invalidateClipping(this);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        if (al::isMsgExplosion(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg)) *mHeadIsRequestBlowDown = true;
        mBlowVelocity = al::getVelocity(this);
        al::setNerve(this, &NrvSamboSnowBodyBlowDown);
        return true;
    }
    return false;
}
/** @brief Handles touch knockback and freeze requests. @param pMsg Message. @param pPointer Touch pointer. @param pTarget Touch target. @return Whether handled. */
bool SamboSnowBody::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvSamboSnowBodyBlowDown) || !*mHeadIsStacked) return false;
    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::setVelocitySeparateHV(this, al::getTrans(this) - al::getHitScreenPointTargetPos(pPointer), 8.0f, 30.0f);
        mBlowVelocity = al::getVelocity(this);
        *mHeadIsStacked = false;
        al::invalidateClipping(this);
        al::setNerve(this, &NrvSamboSnowBodyBlowDown);
        return true;
    }
    if (al::isMsgTouchAssist(pMsg)) {
        *mHeadSupportFreezeActor = DrcFunction::tryFindDrcTouchActor(this, pPointer);
        return true;
    }
    return false;
}
/** @brief Checks for a defeated body. @return Whether falling or melting. */
bool SamboSnowBody::isAttacked() { return al::isNerve(this, &NrvSamboSnowBodyBlowDown) || al::isNerve(this, &NrvSamboSnowBodyFireDown); }
/** @brief Freezes the body on behalf of its head. @param pActor Touching actor. */
void SamboSnowBody::requestSupportFreeze(const al::LiveActor* pActor) {
    if (al::isNerve(this, &NrvSamboSnowBody.SupportFreeze)) return;
    mFreeze->forceSetTouchActor(const_cast<al::LiveActor*>(pActor));
    al::setNerve(this, &NrvSamboSnowBody.SupportFreeze);
}
/** @brief Ends a shared freeze. */
void SamboSnowBody::requestEndSupportFreeze() {
    if (al::isNerve(this, &NrvSamboSnowBody.SupportFreeze)) al::setNerve(this, &NrvSamboSnowBody.Wait);
}
/** @brief Waits for the head to move the body. */
void SamboSnowBody::exeWait() {}
/** @brief Falls and turns into a throwable snowball on landing. */
void SamboSnowBody::exeBlowDown() {
    if (al::isFirstStep(this)) { al::onCollide(this); al::startAction(this, "BlowDown"); al::setVelocity(this, mBlowVelocity); }
    al::addVelocityToGravity(this, 1.5f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::emitEffect(this, "Change", nullptr);
        al::startSe(this, "PgLand", nullptr);
        mSnowBall->makeActorAppeared();
        al::setTrans(mSnowBall, al::getTrans(this) + sead::Vector3f::ey * 50.0f);
        al::resetPosition(mSnowBall, false);
        kill();
    }
    if (al::isGreaterEqualStep(this, 120)) { al::emitEffect(this, "Die", nullptr); kill(); }
}
/** @brief Melts after a fireball hit. */
void SamboSnowBody::exeFireDown() {
    if (al::isFirstStep(this)) al::startAction(this, "FireDown");
    if (al::isActionEnd(this)) kill();
}
/** @brief Updates the shared freeze state. */
void SamboSnowBody::exeSupportFreeze() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvSamboSnowBody.Wait);
}

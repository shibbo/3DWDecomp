#include "MapObj/ThrowMapParts.hpp"
#include "MapObj/BallStateThrow.hpp"
#include "MapObj/BallStateFall.hpp"
#include "MapObj/BallStateFallParam.hpp"
#include "MapObj/ItemStateGigaPlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Player.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_ACTION_IMPL(ThrowMapParts, Wait)
NERVE_ACTION_IMPL(ThrowMapParts, PullOutSuccess)
NERVE_ACTION_IMPL(ThrowMapParts, PullOutMiss)
NERVE_ACTION_IMPL(ThrowMapParts, PullOutMissWait)
NERVE_ACTION_IMPL(ThrowMapParts, PlayerHold)
NERVE_ACTION_IMPL(ThrowMapParts, Throw)
NERVE_ACTION_IMPL(ThrowMapParts, Fall)
NERVE_ACTION_IMPL(ThrowMapParts, Stuck)
NERVE_ACTION_IMPL(ThrowMapParts, Destroy)
alNerveFunction::NerveActionCollector sNerveCollector;
ThrowMapPartsNrvWait NrvThrowMapPartsWait;
ThrowMapPartsNrvPullOutSuccess NrvThrowMapPartsPullOutSuccess;
ThrowMapPartsNrvPullOutMiss NrvThrowMapPartsPullOutMiss;
ThrowMapPartsNrvPullOutMissWait NrvThrowMapPartsPullOutMissWait;
ThrowMapPartsNrvPlayerHold NrvThrowMapPartsPlayerHold;
ThrowMapPartsNrvThrow NrvThrowMapPartsThrow;
ThrowMapPartsNrvFall NrvThrowMapPartsFall;
ThrowMapPartsNrvStuck NrvThrowMapPartsStuck;
ThrowMapPartsNrvDestroy NrvThrowMapPartsDestroy;
BallStateFallParam sFallParam(28.0f, 11.0f, 0.6f, 0.3f, 0.1f, 50.0f, 0.2f, 15.0f, 0.5f, 0.95f, 0.65f, 0.5f);
BallStateThrowParam sThrowParam(600.0f, 450.0f, 10.0f, 16.0f, 0.5f, 0.996f, 0.65f, 1.25f, 0, true, false);
ItemStatePlayerHoldParam sHoldParam({10.0f, 0.0f, 40.0f}, {5.0f, 0.0f, 35.0f}, {5.0f, 0.0f, 25.0f}, {25.0f, 0.0f, 45.0f}, {5.0f, 0.0f, 25.0f}, {30.0f, 0.0f, 45.0f}, {30.0f, 0.0f, 45.0f}, {30.0f, 0.0f, 35.0f}, {35.0f, 0.0f, 45.0f}, {40.0f, 0.0f, 30.0f}, {0.0f, 0.0f, 30.0f});
}
ThrowMapParts::ThrowMapParts(const char* name) : al::LiveActor(name) {}
ThrowMapParts::~ThrowMapParts() = default;
void ThrowMapParts::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &sNerveCollector, 4);
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, info, nullptr, 0);
    mThrowState = new BallStateThrow(this, &sThrowParam);
    mFallState = new BallStateFall(this, &sFallParam);
    mHoldState = new ItemStateGigaPlayerHold(this, &sHoldParam, false, false);
    al::initNerveState(this, mThrowState, &NrvThrowMapPartsThrow, "Throw");
    al::initNerveState(this, mFallState, &NrvThrowMapPartsFall, "Fall");
    al::initNerveState(this, mHoldState, &NrvThrowMapPartsPlayerHold, "PlayerHold");
    mHoldState->initColliderControl();
    makeActorAppeared();
    al::invalidateClipping(this);
    al::offCollide(this);
    mInitialMtx = *getBaseMtx();
}
void ThrowMapParts::appear() { al::LiveActor::appear(); }
void ThrowMapParts::kill() { al::LiveActor::kill(); }
void ThrowMapParts::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if (al::isNerve(this, &NrvThrowMapPartsThrow) &&
        (al::isSensorEnemyBody(other) || al::isSensorEnemyType(other)) && al::sendMsgBallAttack(other, self, nullptr)) {
        mStuckSensor = other;
        mStuckMtx = al::getHitSensorFollowMtx(other);
        sead::Matrix34f inverse;
        inverse.setInverse(*mStuckMtx);
        mStuckOffset = inverse * *getBaseMtx();
        al::startNerveAction(this, "Stuck");
    }
}
bool ThrowMapParts::isEnableHold(al::HitSensor* sensor) {
    if (!mHolderSensor || al::getSensorHost(sensor) != al::getSensorHost(mHolderSensor)) return true;
    return al::isNerve(this, &NrvThrowMapPartsWait);
}
bool ThrowMapParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor* self) {
    if (isEnableHold(other) && mHoldState->tryStartCarryFront(msg, other, false)) {
        mHolderSensor = other;
        al::startNerveAction(this, "PlayerHold");
        return true;
    }
    if (al::isNerve(this, &NrvThrowMapPartsPlayerHold) && mHoldState->receiveMsg(msg, other, self)) {
        if (al::isMsgPlayerRelease(msg) || al::isMsgPlayerReleaseDamage(msg) || al::isMsgPlayerReleaseDead(msg)) {
            al::startNerveAction(this, "Throw");
            al::offCollide(this);
            mThrowState->setThrowParam(mHolderSensor, &sThrowParam, nullptr);
        } else if (al::isMsgWarpStart(msg) || al::isMsgHoldCancel(msg)) al::startNerveAction(this, "Fall");
        return true;
    }
    if (al::isMsgBindEnd(msg)) { al::startNerveAction(this, "Destroy"); return true; }
    return false;
}
void ThrowMapParts::control() {
    if (mStuckSensor) {
        sead::Matrix34f pose;
        pose.setMul(*mStuckMtx, mStuckOffset);
        al::setTrans(this, pose.getTranslation());
        sead::Quatf quat;
        pose.toQuat(quat);
        al::setQuat(this, quat);
    }
}
void ThrowMapParts::updateCollider() {
    if (mHoldState->isDead()) al::LiveActor::updateCollider();
    else mHoldState->updateCollider(al::getHitSensor(this, "Body"));
}
void ThrowMapParts::requestRelease(al::HitSensor*, BallStateThrowParam* param, const char* action) {
    rc::requestPlayerRelease(mHolderSensor);
    mThrowState->setThrowParam(mHolderSensor, param, nullptr);
    al::startNerveAction(this, action);
    al::offCollide(this);
}
void ThrowMapParts::exeWait() {}
void ThrowMapParts::exePullOutSuccess() { al::startNerveAction(this, "PlayerHold"); }
void ThrowMapParts::exePullOutMiss() { if (al::isActionEnd(this)) al::startNerveAction(this, "PullOutMissWait"); }
void ThrowMapParts::exePullOutMissWait() {}
void ThrowMapParts::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::invalidateCollisionParts(this);
    } else if (al::isStep(this, 3)) al::onCollide(this);
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvThrowMapPartsFall); return; }
    if (mHolderSensor && al::isSensorPlayer(mHolderSensor)) {
        auto* player = static_cast<PlayerActor*>(al::getSensorHost(mHolderSensor));
        if (player && player->getPlayer() && player->getPlayer()->getGigaDirector() && !player->getPlayerGigaDirector()->isFullScale())
            requestRelease(mHolderSensor, &sThrowParam, "Throw");
    }
}
void ThrowMapParts::exeThrow() {
    if (al::isFirstStep(this)) al::invalidateClipping(this);
    if (al::isStep(this, 3)) al::onCollide(this);
    if (al::updateNerveState(this)) al::startNerveAction(this, "Fall");
}
void ThrowMapParts::exeFall() { if (al::updateNerveState(this)) al::startNerveAction(this, "Destroy"); }
void ThrowMapParts::exeStuck() { if (al::isFirstStep(this)) al::offCollide(this); }
void ThrowMapParts::exeDestroy() {
    mStuckSensor = nullptr;
    sead::Quatf quat;
    mInitialMtx.toQuat(quat);
    al::resetPosition(this, mInitialMtx.getTranslation(), false);
    al::setQuat(this, quat);
    al::startNerveAction(this, "Wait");
    al::setVelocity(this, sead::Vector3f::zero);
    al::validateCollisionParts(this);
    al::offCollide(this);
}

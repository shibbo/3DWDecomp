#include "MapObj/LavaGeyser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
namespace {
float smoothGeyserRate(float t) {
    float t2 = t * t;
    float t3 = t * t2;
    return 3.0f * t2 - 2.0f * t3;
}
NERVE_DECL(LavaGeyser, Ready);
NERVE_DECL(LavaGeyser, Delay);
NERVE_DECL(LavaGeyser, SwitchOffStart);
NERVE_DECL(LavaGeyser, Rise);
NERVE_DECL(LavaGeyser, KeepRising);
NERVE_DECL(LavaGeyser, FallDown);
NERVE_DECL(LavaGeyser, Wait);
NERVES_MAKE_STRUCT(LavaGeyser, Ready, Delay, SwitchOffStart)
NERVES_MAKE_NOSTRUCT(LavaGeyser, Rise, KeepRising, FallDown, Wait)
}
LavaGeyser::LavaGeyser(const char* name) : al::LiveActor(name) {}
LavaGeyser::~LavaGeyser() {}
void LavaGeyser::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    mStartPos = al::getTrans(this);
    if (!al::tryGetLinksTrans(&mEndPos, info, "MoveNext")) {
        makeActorDead();
        return;
    }
    mClippingCenter = (mStartPos + mEndPos) * 0.5f;
    al::setClippingInfo(this, sead::Mathf::abs(mClippingCenter.y - mStartPos.y), &mClippingCenter);
    al::tryGetArg(&mWaitTime, info, "WaitTime");
    al::tryGetArg(&mDelayTime, info, "DelayTime");
    al::tryGetArg(&mSearchDistanceUp, info, "SearchDistanceUp");
    al::tryGetArg(&mSearchDistanceDown, info, "SearchDistanceDown");
    al::initNerve(this, &NrvLavaGeyser.Ready, 0);
    if (mDelayTime > 0) al::setNerve(this, &NrvLavaGeyser.Delay);
    if (al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &LavaGeyser::start)))
        al::setNerve(this, &NrvLavaGeyser.SwitchOffStart);
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalTransController(this, &mEffectTrans, "SplashJoint");
    makeActorAppeared();
}
void LavaGeyser::start() {
    if (mDelayTime > 0) al::setNerve(this, &NrvLavaGeyser.Delay);
    else al::setNerve(this, &NrvLavaGeyser.Ready);
}
void LavaGeyser::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorPlayer(receiver) && al::isHitCylinderSensor(receiver, sender, sead::Vector3f::ey, 160.0f))
        al::sendMsgEnemyAttack(receiver, sender);
}
bool LavaGeyser::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangBreak(msg))
        return al::isHitCylinderSensor(sender, receiver, sead::Vector3f::ey, 160.0f);
    return false;
}
bool LavaGeyser::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    return al::isMsgTouchAssistBurn(msg);
}
void LavaGeyser::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "DownWait");
    if (mWaitTime - 1 < 0 || al::isGreaterEqualStep(this, mWaitTime - 1))
        al::setNerve(this, &NrvLavaGeyser.Ready);
}
void LavaGeyser::exeReady() {
    if (al::isFirstStep(this)) al::startAction(this, "UpSign");
    if (al::isGreaterEqualStep(this, 59)) {
        al::setNerve(this, &NrvLavaGeyserRise);
        return;
    }
    calcEffectTrans();
}
void LavaGeyser::calcEffectTrans() {
    if (al::isNearZero(mSearchDistanceUp - mSearchDistanceDown, 0.001f)) {
        const sead::Vector3f& current = al::getTrans(this);
        mEffectTrans.y = mStartPos.y - current.y + mSearchDistanceDown;
        return;
    }
    sead::Vector3f hitPos(0.0f, 0.0f, 0.0f);
    al::Triangle triangle;
    sead::Vector3f start(mStartPos);
    start.y += mSearchDistanceUp;
    bool hit = alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle, start, -sead::Vector3f::ey * (mSearchDistanceUp - mSearchDistanceDown), nullptr, nullptr);
    const sead::Vector3f& current = al::getTrans(this);
    if (hit) mEffectTrans.y = hitPos.y - current.y;
    else mEffectTrans.y = mStartPos.y - current.y + mSearchDistanceDown;
}
void LavaGeyser::exeRise() {
    if (al::isFirstStep(this)) al::startAction(this, "Up");
    if (al::isGreaterEqualStep(this, 59)) {
        al::setTrans(this, mEndPos);
        al::setNerve(this, &NrvLavaGeyserKeepRising);
        return;
    }
    sead::Vector3f position(0.0f, 0.0f, 0.0f);
    al::lerpVec(&position, mStartPos, mEndPos, smoothGeyserRate(float(al::getNerveStep(this)) / 60.0f));
    al::setTrans(this, position);
    calcEffectTrans();
}
void LavaGeyser::exeKeepRising() {
    if (al::isFirstStep(this)) al::startAction(this, "UpWait");
    if (al::isGreaterEqualStep(this, 59)) {
        al::setNerve(this, &NrvLavaGeyserFallDown);
        return;
    }
    calcEffectTrans();
}
void LavaGeyser::exeFallDown() {
    if (al::isFirstStep(this)) al::startAction(this, "Down");
    if (al::isGreaterEqualStep(this, 59)) {
        al::setTrans(this, mStartPos);
        al::setNerve(this, &NrvLavaGeyserWait);
        return;
    }
    sead::Vector3f position(0.0f, 0.0f, 0.0f);
    al::lerpVec(&position, mStartPos, mEndPos, smoothGeyserRate(float(al::getNerveStep(this)) / -60.0f + 1.0f));
    al::setTrans(this, position);
    calcEffectTrans();
}
void LavaGeyser::exeSwitchOffStart() {}
void LavaGeyser::exeDelay() {
    if (mDelayTime - 1 < 0 || al::isGreaterEqualStep(this, mDelayTime - 1))
        al::setNerve(this, &NrvLavaGeyser.Ready);
}

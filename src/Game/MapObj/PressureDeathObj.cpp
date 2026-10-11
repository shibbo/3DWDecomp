#include "MapObj/PressureDeathObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_ACTION_IMPL(PressureDeathObj, Move);
NERVE_ACTION_IMPL(PressureDeathObj, Delay);
NERVE_ACTION_IMPL(PressureDeathObj, Wait);
NERVE_ACTION_IMPL(PressureDeathObj, Stop);
NERVE_ACTIONS_MAKE_STRUCT(PressureDeathObj, Move, Delay, Wait, Stop)
sead::Vector3f sPressureSizeA(350.0f, 100.0f, 200.0f);
sead::Vector3f sPressureSizeB(200.0f, 350.0f, 200.0f);
sead::Vector3f sPressureSizeDefault(1000.0f, 1000.0f, 1000.0f);
float absValue(float v) { return v > 0.0f ? v : -v; }
}
PressureDeathObj::PressureDeathObj(const char* name) : al::LiveActor(name) {}
PressureDeathObj::~PressureDeathObj() {}
void PressureDeathObj::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvPressureDeathObj.collector, 0);
    al::initMapPartsActor(this, info, nullptr, 0);
    mKeyPose = al::createKeyPoseKeeper(info);
    const char* name = nullptr;
    al::tryGetObjectName(&name, info);
    if (al::isEqualString(name, "ChorobonTowerPressureDeathObjA")) mPressureHalfSize.set(sPressureSizeA);
    else if (al::isEqualString(name, "ChorobonTowerPressureDeathObjB")) mPressureHalfSize.set(sPressureSizeB);
    else mPressureHalfSize.set(sPressureSizeDefault);
    float radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPose, 0.0f);
    sead::Vector3f scale(1.0f, 1.0f, 1.0f);
    al::tryGetScale(&scale, info);
    radius += al::getClippingRadius(this) * (scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z));
    al::setClippingInfo(this, radius, &mClippingCenter);
    bool hasDelay = al::tryGetArg(&mDelayTime, info, "DelayTime");
    if (al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &PressureDeathObj::start))) al::startNerveAction(this, "Stop");
    else if (hasDelay && mDelayTime > 0) al::startNerveAction(this, "Delay");
    makeActorAppeared();
}
void PressureDeathObj::start() {
    if (mDelayTime > 0) al::startNerveAction(this, "Delay");
    else al::startNerveAction(this, "Move");
}
void PressureDeathObj::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorMapObj(sender)) return;
    if (al::isSensorPlayer(receiver) && rc::isPlayerDeadOrBubble(al::getSensorHost(receiver))) return;
    if (isInsidePressureArea(receiver)) al::sendMsgPressureDeath(receiver, sender);
}
bool PressureDeathObj::isInsidePressureArea(const al::HitSensor* sensor) {
    sead::Vector3f side = sead::Vector3f::ex;
    sead::Vector3f up = sead::Vector3f::ey;
    sead::Vector3f front = sead::Vector3f::ez;
    sead::Vector3f offset = al::getSensorPos(sensor) - al::getTrans(this);
    float x = absValue(side.dot(offset));
    float y = absValue(up.dot(offset));
    float z = absValue(front.dot(offset));
    return x <= mPressureHalfSize.x && y <= mPressureHalfSize.y && z <= mPressureHalfSize.z;
}
void PressureDeathObj::exeMove() {
    if (al::isFirstStep(this)) {
        int time = al::calcKeyMoveSpeedByTime(mKeyPose);
        if (time > 0) mMoveTime = time;
        else {
            float speed = al::calcKeyMoveSpeed(mKeyPose);
            if (speed > 0.0f) {
                int frames = al::calcDistanceNextKeyTrans(mKeyPose) / speed;
                mMoveTime = frames < 0 ? 1 : frames;
            }
        }
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPose, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPose, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::startHitReaction(this, mKeyPose->isGoingToEnd() ? "停止" : "停止[戻る]");
        al::nextKeyPose(mKeyPose);
        if (al::isStop(mKeyPose)) al::startNerveAction(this, "Stop");
        else al::startNerveAction(this, "Wait");
    }
}
void PressureDeathObj::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayTime)) al::startNerveAction(this, "Move");
}
void PressureDeathObj::exeWait() {
    if (al::isFirstStep(this)) {
        mWaitTime = al::calcKeyMoveWaitTime(mKeyPose);
        if (mWaitTime < 0) { al::startNerveAction(this, "Wait"); return; }
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) al::startNerveAction(this, "Move");
}
void PressureDeathObj::exeStop() {}

#include "MapObj/LiftMikeSlide.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
NERVE_ACTION_IMPL(LiftMikeSlide, Wait);
NERVE_ACTION_IMPL(LiftMikeSlide, Move);
NERVE_ACTION_IMPL(LiftMikeSlide, Hold);
NERVE_ACTION_IMPL(LiftMikeSlide, Back);
NERVE_ACTIONS_MAKE_STRUCT(LiftMikeSlide, Wait, Move, Hold, Back)
float absValue(float v) { return v > 0.0f ? v : -v; }
float endpointDot(const sead::Vector3f& pos, const sead::Vector3f& origin, const sead::Vector3f& target) {
    sead::Vector3f offset = pos;
    offset -= origin;
    sead::Vector3f direction = target;
    direction -= origin;
    return offset.dot(direction);
}
}
LiftMikeSlide::LiftMikeSlide(const char* name) : al::LiveActor(name) {}
LiftMikeSlide::~LiftMikeSlide() {}
void LiftMikeSlide::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvLiftMikeSlide.collector, 0);
    al::initActorWithArchiveName(this, info, "LiftMikeSlide", nullptr);
    mKeyPose = al::createKeyPoseKeeper(info);
    float radius;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPose, 0.0f);
    radius += al::getClippingRadius(this);
    al::setClippingInfo(this, radius, &mClippingCenter);
    mBaseTrans.set(al::getTrans(this));
    makeActorAppeared();
}
void LiftMikeSlide::makeActorAppeared() { al::LiveActor::makeActorAppeared(); }
void LiftMikeSlide::control() {
    al::LiveActor::control();
    float rate = mSpeed * 10.0f / 45.0f + 0.2f;
    if (absValue(rate) < absValue(mAnimRate)) rate = mAnimRate * 0.98f + rate * 0.02f;
    mAnimRate = rate;
    al::setSklAnimFrameRate(this, rate, 0);
}
void LiftMikeSlide::exeWait() {
    if (al::isMicBreathInputOn(this)) al::startNerveAction(this, "Move");
}
void LiftMikeSlide::exeMove() {
    bool isBlowing = al::isMicBreathInputOn(this);
    if (isBlowing) mSpeed += 4.0f;
    else mSpeed += -0.9f;
    updateMove();
    if (mSpeed <= 0.0f && !al::isMicBreathInputOn(this)) {
        mSpeed = 0.0f;
        al::startNerveAction(this, "Hold");
    }
}
void LiftMikeSlide::updateMove() {
    if (mSpeed > 45.0f) mSpeed = 45.0f;
    else if (mSpeed < -25.0f) mSpeed = -25.0f;
    sead::Vector3f dir(0.0f, 0.0f, 0.0f);
    sead::Vector3CalcCommon<float>::sub(dir, al::getNextKeyTrans(mKeyPose), al::getCurrentKeyTrans(mKeyPose));
    al::normalize(&dir);
    dir *= mSpeed;
    sead::Vector3f pos = al::getTrans(this);
    pos += dir;
    const sead::Vector3f& next = al::getNextKeyTrans(mKeyPose);
    const sead::Vector3f& current = al::getCurrentKeyTrans(mKeyPose);
    if (endpointDot(pos, next, current) < 0.0f) {
        pos = al::getNextKeyTrans(mKeyPose);
        mSpeed = 0.0f;
        al::startNerveAction(this, "Hold");
    }
    const sead::Vector3f& start = al::getCurrentKeyTrans(mKeyPose);
    const sead::Vector3f& end = al::getNextKeyTrans(mKeyPose);
    if (endpointDot(pos, start, end) < 0.0f) {
        pos = al::getCurrentKeyTrans(mKeyPose);
        mSpeed = 0.0f;
        al::startNerveAction(this, "Wait");
    }
    *al::getTransPtr(this) = pos;
    float rate = absValue(mSpeed / 45.0f);
    float pitch = rate * 0.7f + 0.5f;
    float volume = rate * 2.0f + 0.1f;
    al::holdSeSetPitchVolumeByName(this, "LiftUp", pitch, volume);
}
void LiftMikeSlide::exeHold() {
    if (al::isGreaterEqualStep(this, 90)) al::startNerveAction(this, "Back");
    if (al::isMicBreathInputOn(this)) al::startNerveAction(this, "Move");
}
void LiftMikeSlide::exeBack() {
    if (mSpeed > 0.0f) mSpeed = 0.0f;
    mSpeed += -0.5f;
    updateMove();
    if (al::isMicBreathInputOn(this)) al::startNerveAction(this, "Move");
}

#include "MapObj/TestIshiokaTrapWall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
    NERVE_DECL(TestIshiokaTrapWall, Stop);
    NERVE_DECL(TestIshiokaTrapWall, Delay);
    NERVE_DECL(TestIshiokaTrapWall, Wait);
    NERVE_DECL(TestIshiokaTrapWall, Move);
    NERVES_MAKE_NOSTRUCT(TestIshiokaTrapWall, Stop, Delay, Wait, Move)
}
TestIshiokaTrapWall::TestIshiokaTrapWall(const char* name) : al::LiveActor(name) {}
void TestIshiokaTrapWall::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(info);
    float radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoseKeeper, 0.0f);
    sead::Vector3f scale(1.0f, 1.0f, 1.0f);
    al::tryGetScale(&scale, info);
    radius += al::getClippingRadius(this) * (scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z));
    al::setClippingInfo(this, radius, &mClippingCenter);
    bool hasDelay = al::tryGetArg(&mDelayFrames, info, "DelayTime");
    if (al::listenStageSwitchOnStart(this, al::Functor(this, &TestIshiokaTrapWall::start))) al::initNerve(this, &NrvTestIshiokaTrapWallStop, 0);
    else if (hasDelay && mDelayFrames >= 1) al::initNerve(this, &NrvTestIshiokaTrapWallDelay, 0);
    else al::initNerve(this, &NrvTestIshiokaTrapWallWait, 0);
    makeActorAppeared();
}
void TestIshiokaTrapWall::start() { al::setNerve(this, &NrvTestIshiokaTrapWallMove); }
void TestIshiokaTrapWall::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorMapObj(sender)) return;
    if (al::isSensorPlayer(receiver) && rc::isPlayerDeadOrBubble(al::getSensorHost(receiver))) return;
    sead::Vector3f pos = al::getSensorPos(receiver);
    const sead::Matrix34f* mtx = getBaseMtx();
    sead::Vector3f side = mtx->getBase(0);
    sead::Vector3f up;
    mtx->getBase(up, 1);
    sead::Vector3f front = mtx->getBase(2);
    sead::Vector3f offset = pos - mtx->getTranslation();
    float x = side.dot(offset);
    if (x < -400.0f || x > 400.0f) return;
    float y = up.dot(offset);
    if (y < -400.0f || y > 400.0f) return;
    float z = front.dot(offset);
    float halfRadius = al::getSensorRadius(receiver) * 0.5f;
    if (z > halfRadius || z < -1600.0f - halfRadius) return;
    al::sendMsgPressureDeath(receiver, sender);
}
void TestIshiokaTrapWall::exeMove() {
    if (al::isFirstStep(this)) {
        int frames = al::calcKeyMoveSpeedByTime(mKeyPoseKeeper);
        if (frames >= 1) mMoveFrames = frames;
        else {
            float speed = al::calcKeyMoveSpeed(mKeyPoseKeeper);
            if (speed > 0.0f) {
                int duration = al::calcDistanceNextKeyTrans(mKeyPoseKeeper) / speed;
                mMoveFrames = duration < 0 ? 1 : duration;
            }
        }
    }
    float rate = al::calcNerveRate(this, mMoveFrames);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    if (al::isGreaterEqualStep(this, mMoveFrames)) {
        al::nextKeyPose(mKeyPoseKeeper);
        if (al::isStop(mKeyPoseKeeper)) al::setNerve(this, &NrvTestIshiokaTrapWallStop);
        else al::setNerve(this, &NrvTestIshiokaTrapWallWait);
    }
}
void TestIshiokaTrapWall::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayFrames)) al::setNerve(this, &NrvTestIshiokaTrapWallMove);
}
void TestIshiokaTrapWall::exeWait() {
    if (al::isFirstStep(this)) {
        mWaitFrames = al::calcKeyMoveWaitTime(mKeyPoseKeeper);
        if (mWaitFrames < 0) { al::setNerve(this, &NrvTestIshiokaTrapWallMove); return; }
    }
    if (al::isGreaterEqualStep(this, mWaitFrames)) al::setNerve(this, &NrvTestIshiokaTrapWallMove);
}
void TestIshiokaTrapWall::exeStop() {}

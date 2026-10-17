#include "MapObj/WoodLogBridge.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(WoodLogBridgeParts, Wait);
NERVE_DECL(WoodLogBridgeParts, HipDrop);
NERVE_DECL(WoodLogBridgeParts, Ride);
NERVES_MAKE_NOSTRUCT(WoodLogBridgeParts, Wait)
NERVES_MAKE_STRUCT(WoodLogBridgeParts, HipDrop, Ride)
}
WoodLogBridgeParts::WoodLogBridgeParts(const char* name) : al::LiveActor(name) {}
WoodLogBridgeParts::~WoodLogBridgeParts() {}
void WoodLogBridgeParts::setInitPos(const sead::Vector3f& pos) {
    al::setTrans(this, pos);
    mInitPos = pos;
}
WoodLogBridge::WoodLogBridge(const char* name) : al::LiveActor(name) {}
WoodLogBridge::~WoodLogBridge() {}
void WoodLogBridge::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, info);
    al::initExecutorWatchObj(this, info);
    al::tryGetArg(&mCount, info, "PartsNum");
    mParts = new WoodLogBridgeParts*[mCount];
    int i = 0;
    do {
        mParts[i] = new WoodLogBridgeParts("丸太");
        mParts[i]->init(info);
        WoodLogBridgeParts* part = mParts[i];
        const sead::Vector3f& trans = al::getTrans(this);
        const sead::Vector3f& front = al::getFront(this);
        part->setInitPos(front * 100.0f * i + trans);
        int count = mCount;
        int halfCount = count / 2;
        mParts[i]->setMaxSink((i < halfCount ? float(i) : float(count - 1 - i)) * 30.0f);
        mParts[i]->getName();
        ++i;
    } while (i < mCount);
    mParts[0]->mIsFixed = true;
    mParts[mCount - 1]->mIsFixed = true;
    mHeights = new float[mCount];
    for (int j = 0; j < mCount; ++j) mHeights[j] = 0.0f;
    makeActorAppeared();
}
void WoodLogBridge::control() {
    for (int i = 0; i < mCount; ++i) mHeights[i] = mParts[i]->mSink;
    for (int i = 1; i < mCount - 1; ++i) mParts[i]->mSink = (mHeights[i - 1] + mHeights[i + 1]) * 0.5f;
}
void WoodLogBridgeParts::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "WoodLogBridgeParts", nullptr);
    al::initNerve(this, &NrvWoodLogBridgePartsWait, 0);
    makeActorAppeared();
}
void WoodLogBridgeParts::control() {
    if (mSink < -mMaxSink) mSink = -mMaxSink;
    al::setTrans(this, mSink * sead::Vector3f::ey + mInitPos);
}
bool WoodLogBridgeParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (mIsFixed) return false;
    if (al::isMsgPlayerFloorTouch(msg) && !al::isNerve(this, &NrvWoodLogBridgeParts.HipDrop)) {
        const sead::Vector3f sensorPos = al::getSensorPos(sender);
        sead::Vector3f offset = sensorPos - al::getTrans(this);
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        sead::Vector3f parallel;
        al::parallelizeVec(&parallel, front, offset);
        if (parallel.length() < 50.0f) {
            al::setNerve(this, &NrvWoodLogBridgeParts.Ride);
            return true;
        }
    } else if (al::isMsgPlayerHipDropAll(msg)) {
        al::setNerve(this, &NrvWoodLogBridgeParts.HipDrop);
        return true;
    }
    return false;
}
bool WoodLogBridgeParts::isPlayerRide() const {
    return al::isNerve(this, &NrvWoodLogBridgeParts.Ride) || al::isNerve(this, &NrvWoodLogBridgeParts.HipDrop);
}
void WoodLogBridgeParts::exeWait() {}
void WoodLogBridgeParts::exeRide() {
    if (mSink > -100.0f) mSink = -100.0f;
    if (mSink < -mMaxSink) mSink = -mMaxSink;
    if (al::isGreaterEqualStep(this, 1)) al::setNerve(this, &NrvWoodLogBridgePartsWait);
}
void WoodLogBridgeParts::exeHipDrop() {
    mSink = -250.0f;
    if (mMaxSink < 250.0f) mSink = -mMaxSink;
    if (al::isGreaterEqualStep(this, 25)) al::setNerve(this, &NrvWoodLogBridgePartsWait);
}

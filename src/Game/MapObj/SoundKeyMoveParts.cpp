#include "MapObj/SoundKeyMoveParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
namespace {
    NERVE_ACTION_IMPL(SoundKeyMoveParts, Delay);
    NERVE_ACTION_IMPL(SoundKeyMoveParts, Wait);
    NERVE_ACTION_IMPL(SoundKeyMoveParts, MoveSign);
    NERVE_ACTION_IMPL(SoundKeyMoveParts, Move);
    NERVE_ACTION_IMPL(SoundKeyMoveParts, StopSign);
    NERVE_ACTION_IMPL(SoundKeyMoveParts, Stop);
    NERVE_ACTIONS_MAKE_STRUCT(SoundKeyMoveParts, Delay, Wait, MoveSign, Move, StopSign, Stop)
}
SoundKeyMoveParts::SoundKeyMoveParts(const char* name) : al::LiveActor(name) {}
SoundKeyMoveParts::~SoundKeyMoveParts() {}
void SoundKeyMoveParts::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvSoundKeyMoveParts.collector, 0);
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, info, nullptr, 0);
    mKeyPoses = al::createKeyPoseKeeper(info);
    float radius;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoses, 0.0f);
    radius = al::getClippingRadius(this) + radius;
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::tryGetArg(&mDelayTime, info, "DelayTime");
    if (mDelayTime > 0) al::startNerveAction(this, "Delay");
    makeActorAppeared();
}
void SoundKeyMoveParts::changeBgmTrackVolumeByRate(const al::IUseAudioKeeper* user, float rate) {
    const char* const situations[] = {"MoveRateL0", "MoveRateL1", "MoveRateL2", "MoveRateL3", "MoveRateL4"};
    int level = int(rate * 4.0f);
    if (mBgmLevel != level) al::changeBgmSituation(user, situations[level]);
    mBgmLevel = level;
}
void SoundKeyMoveParts::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayTime - 1)) al::startNerveAction(this, "Wait");
}
void SoundKeyMoveParts::exeWait() {
    if (al::isFirstStep(this)) {
        int waitTime = al::calcKeyMoveWaitTime(mKeyPoses);
        if (waitTime >= 0) mWaitTime = waitTime;
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) {
        bool playSign = false;
        al::tryGetArg(&playSign, al::getCurrentKeyPlacementInfo(mKeyPoses), "IsPlaySign");
        if (playSign && al::isExistAction(this, "MoveKeySign")) al::startNerveAction(this, "MoveSign");
        else al::startNerveAction(this, "Move");
    }
}
void SoundKeyMoveParts::exeMoveSign() {
    if (al::isFirstStep(this)) al::startAction(this, "MoveKeySign");
    if (al::isActionEnd(this)) al::startNerveAction(this, "Move");
}
void SoundKeyMoveParts::exeMove() {
    if (al::isFirstStep(this)) {
        if (al::isExistAction(this, "MoveLoop")) al::tryStartActionIfNotPlaying(this, "MoveLoop");
        mMoveTime = al::calcKeyMoveMoveTime(mKeyPoses);
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoses, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoses, rate);
    changeBgmTrackVolumeByRate(this, mKeyPoses->isGoingToEnd() ? 1.0f - rate : rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoses);
        if (al::isStop(mKeyPoses)) {
            if (al::isExistAction(this, "StopSign")) al::startNerveAction(this, "StopSign");
            else al::startNerveAction(this, "Stop");
        } else al::startNerveAction(this, "Wait");
    }
}
void SoundKeyMoveParts::exeStopSign() {
    if (al::isFirstStep(this)) al::startAction(this, "StopSign");
    if (al::isActionEnd(this)) al::tryStartAction(this, "Stop");
}
void SoundKeyMoveParts::exeStop() {
    if (al::isFirstStep(this) && al::isInvalidClipping(this)) al::validateClipping(this);
}

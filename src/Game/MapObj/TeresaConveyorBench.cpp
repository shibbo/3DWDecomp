#include "MapObj/TeresaConveyorBench.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
    NERVE_DECL(TeresaConveyorBench, StandBy);
    NERVE_DECL(TeresaConveyorBench, MoveSign);
    NERVE_DECL(TeresaConveyorBench, Move);
    NERVE_DECL(TeresaConveyorBench, Stop);
    NERVE_DECL(TeresaConveyorBench, Wait);
    NERVES_MAKE_NOSTRUCT(TeresaConveyorBench, StandBy, MoveSign, Move, Stop, Wait)
}
TeresaConveyorBench::TeresaConveyorBench(const char* pName) : al::LiveActor(pName) {}
TeresaConveyorBench::~TeresaConveyorBench() {}
void TeresaConveyorBench::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTeresaConveyorBenchStandBy, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    float radius;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoseKeeper, 0.0f);
    radius += al::getClippingRadius(this);
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::listenStageSwitchOnStart(this, al::Functor(this, &TeresaConveyorBench::start));
    makeActorAppeared();
}
void TeresaConveyorBench::start() {
    if (!al::isNerve(this, &NrvTeresaConveyorBenchStandBy) || al::getKeyPoseCount(mKeyPoseKeeper) < 2)
        return;
    al::setNerve(this, &NrvTeresaConveyorBenchMoveSign);
}
bool TeresaConveyorBench::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgFloorTouch(pMsg)) {
        start();
        return true;
    }
    return false;
}
void TeresaConveyorBench::exeStandBy() {}
void TeresaConveyorBench::exeMoveSign() {
    if (al::isFirstStep(this))
        al::startAction(this, "MoveKeySign");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvTeresaConveyorBenchMove);
}
void TeresaConveyorBench::exeWait() {
    if (al::isFirstStep(this)) {
        int waitTime = al::calcKeyMoveWaitTime(mKeyPoseKeeper);
        if (waitTime >= 0)
            mWaitTime = waitTime;
    }
    if (al::isGreaterEqualStep(this, mWaitTime))
        al::setNerve(this, &NrvTeresaConveyorBenchMove);
}
void TeresaConveyorBench::exeMove() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "MoveLoop");
        mMoveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper);
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoseKeeper);
        if (al::isStop(mKeyPoseKeeper))
            al::setNerve(this, &NrvTeresaConveyorBenchStop);
        else
            al::setNerve(this, &NrvTeresaConveyorBenchWait);
    }
}
void TeresaConveyorBench::exeStop() {}

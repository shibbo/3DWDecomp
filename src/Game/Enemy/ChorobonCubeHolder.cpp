#include "Enemy/ChorobonCubeHolder.hpp"
#include "Enemy/ChorobonCube.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Movement/PoseHistoryPath.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(ChorobonCubeHolder, Move)
NERVE_DECL(ChorobonCubeHolder, Stop)
NERVE_DECL(ChorobonCubeHolder, Delay)
NERVE_DECL(ChorobonCubeHolder, Wait)
NERVES_MAKE_NOSTRUCT(ChorobonCubeHolder, Move, Stop, Delay, Wait)
}

/** @brief Creates the chain and its pose history. @param pName Actor name. */
ChorobonCubeHolder::ChorobonCubeHolder(const char* pName)
    : al::LiveActor(pName), mPath(new al::PoseHistoryPath(128)) {}

/** @brief Creates the trailing cubes and initializes movement. @param rInfo Placement and scene information. */
void ChorobonCubeHolder::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvChorobonCubeHolderMove, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    f32 radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoseKeeper, 0.0f);
    radius += al::getClippingRadius(this);
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::tryGetArg(&mCubeCount, rInfo, "CubeNum");
    if (mCubeCount <= 0) {
        makeActorDead();
        return;
    }
    --mCubeCount;
    mCubes = new ChorobonCube*[mCubeCount];
    for (s32 i = 0; i < mCubeCount; ++i) {
        mCubes[i] = new ChorobonCube("\u30ad\u30e5\u30fc\u30d6\u30c1\u30e7\u30ed\u30dc\u30f3[\u5b50\u4f9b]");
        al::initCreateActorWithPlacementInfo(mCubes[i], rInfo);
        al::setClippingInfo(mCubes[i], radius, &mClippingCenter);
    }
    al::tryGetArg(&mDelayTime, rInfo, "DelayTime");
    if (al::listenStageSwitchOnStart(this, al::Functor(this, &ChorobonCubeHolder::start))) {
        al::setNerve(this, &NrvChorobonCubeHolderStop);
    } else if (mDelayTime > 0) {
        al::setNerve(this, &NrvChorobonCubeHolderDelay);
    }
    al::trySyncStageSwitchKill(this);
    makeActorAppeared();
}

/** @brief Starts the chain after its configured delay. */
void ChorobonCubeHolder::start() {
    if (mDelayTime > 0) { al::setNerve(this, &NrvChorobonCubeHolderDelay); }
    else { al::setNerve(this, &NrvChorobonCubeHolderMove); }
}

/** @brief Kills the chain and every trailing cube. */
void ChorobonCubeHolder::kill() {
    al::LiveActor::kill();
    for (s32 i = 0; i < mCubeCount; ++i) { mCubes[i]->kill(); }
}

/** @brief Moves the leader and samples its path for trailing cubes. */
void ChorobonCubeHolder::exeMove() {
    if (al::isFirstStep(this)) { mMoveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper); }
    al::holdSeWithParam(this, "Wait", 1.0f, nullptr);
    f32 rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    if (mCubeCount > 0) {
        sead::Quatf quat;
        al::calcQuat(&quat, this);
        mPath->addHistory(quat, al::getTrans(this), 30.0f);
        for (s32 i = 0; i < mCubeCount; ++i) {
            mPath->calcPoseAndTrans(al::getQuatPtr(mCubes[i]), al::getTransPtr(mCubes[i]), mCubeDistance * (i + 1));
        }
    }
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoseKeeper);
        if (al::isStop(mKeyPoseKeeper)) { al::setNerve(this, &NrvChorobonCubeHolderStop); }
        else { al::setNerve(this, &NrvChorobonCubeHolderWait); }
    }
}

/** @brief Waits before starting the chain. */
void ChorobonCubeHolder::exeDelay() {
    al::holdSeWithParam(this, "Wait", 1.0f, nullptr);
    if (al::isGreaterEqualStep(this, mDelayTime)) { al::setNerve(this, &NrvChorobonCubeHolderMove); }
}

/** @brief Waits at a key pose. */
void ChorobonCubeHolder::exeWait() {
    if (al::isFirstStep(this)) { mWaitTime = al::calcKeyMoveWaitTime(mKeyPoseKeeper); }
    al::holdSeWithParam(this, "Wait", 1.0f, nullptr);
    if (al::isGreaterEqualStep(this, mWaitTime)) { al::setNerve(this, &NrvChorobonCubeHolderMove); }
}

/** @brief Keeps the chain stopped. */
void ChorobonCubeHolder::exeStop() {}

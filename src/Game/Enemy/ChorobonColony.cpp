#include "Enemy/ChorobonColony.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(ChorobonColony, Wait)
NERVE_DECL(ChorobonColony, Stop)
NERVE_DECL(ChorobonColony, Delay)
NERVE_DECL(ChorobonColony, Move)
NERVES_MAKE_STRUCT(ChorobonColony, Wait, Stop, Delay, Move)
}

/** @brief Creates a moving colony. @param pName Actor name. */
ChorobonColony::ChorobonColony(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes movement keys and stage switches. @param rInfo Placement information. */
void ChorobonColony::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvChorobonColony.Wait, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    al::tryGetArg(&mDelayTime, rInfo, "DelayTime");
    al::listenStageSwitchOnKill(this, al::Functor(this, &ChorobonColony::killBySwitch));
    if (al::listenStageSwitchOnStart(this, al::Functor(this, &ChorobonColony::start))) {
        al::setNerve(this, &NrvChorobonColony.Stop);
        makeActorDead();
    } else {
        if (mDelayTime > 0)
            al::setNerve(this, &NrvChorobonColony.Delay);
        else
            al::setNerve(this, &NrvChorobonColony.Move);
        makeActorAppeared();
    }
}

/** @brief Removes a live colony when its kill switch activates. */
void ChorobonColony::killBySwitch() {
    if (al::isAlive(this))
        kill();
}

/** @brief Appears and begins the configured initial delay or movement. */
void ChorobonColony::start() {
    appear();
    if (mDelayTime > 0)
        al::setNerve(this, &NrvChorobonColony.Delay);
    else
        al::setNerve(this, &NrvChorobonColony.Move);
}

/** @brief Enables the colony and starts its idle animation. */
void ChorobonColony::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::startAction(this, "Wait");
}

/** @brief Interpolates between movement keys and advances to the next key. */
void ChorobonColony::exeMove() {
    if (al::isFirstStep(this))
        mMoveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper);
    f32 rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoseKeeper);
        al::setNerve(this, al::isStop(mKeyPoseKeeper) ? static_cast<const al::Nerve*>(&NrvChorobonColony.Stop)
                                                   : &NrvChorobonColony.Wait);
    }
}

/** @brief Waits for the initial movement delay to expire. */
void ChorobonColony::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayTime))
        al::setNerve(this, &NrvChorobonColony.Move);
}

/** @brief Waits at the current movement key. */
void ChorobonColony::exeWait() {
    if (al::isFirstStep(this)) {
        s32 time = al::calcKeyMoveWaitTime(mKeyPoseKeeper);
        mWaitTime = time < 0 ? 0 : time;
    }
    if (al::isGreaterEqualStep(this, mWaitTime))
        al::setNerve(this, &NrvChorobonColony.Move);
}

/** @brief Remains stationary after the final movement key. */
void ChorobonColony::exeStop() {}

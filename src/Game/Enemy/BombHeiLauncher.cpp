#include "Enemy/BombHeiLauncher.hpp"
#include "Enemy/BombHei.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(BombHeiLauncher, LaunchReady)
NERVE_DECL(BombHeiLauncher, WaitStart)
NERVE_DECL(BombHeiLauncher, Wait)
NERVE_DECL(BombHeiLauncher, LaunchInterval)
NERVE_DECL(BombHeiLauncher, Launch)
NERVES_MAKE_NOSTRUCT(BombHeiLauncher, LaunchReady, WaitStart, Wait, LaunchInterval, Launch)
}

/** @brief Creates an empty Bob-omb launcher. @param pName Actor name. */
BombHeiLauncher::BombHeiLauncher(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates the pool and connects the start switch. @param rInfo Placement information. */
void BombHeiLauncher::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvBombHeiLauncherLaunchReady, 0);
    s32 count = 3;
    al::tryGetArg(&count, rInfo, "NumMax");
    mBombs.allocBuffer(count, nullptr);
    for (s32 i = 0; i < mBombs.capacity(); ++i) {
        auto* pBomb = new BombHei("ボムへい");
        al::initCreateActorWithPlacementInfo(pBomb, rInfo);
        pBomb->makeActorDead();
        mBombs.pushBack(pBomb);
    }
    if (al::listenStageSwitchOnStart(this, al::Functor(this, &BombHeiLauncher::start)))
        al::setNerve(this, &NrvBombHeiLauncherWaitStart);
    makeActorAppeared();
}

/** @brief Begins the launch cycle. */
void BombHeiLauncher::start() { al::setNerve(this, &NrvBombHeiLauncherLaunchReady); }

/** @brief Pushes actors touching the launcher. @param pSelf Own sensor. @param pOther Contact sensor. */
void BombHeiLauncher::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    al::sendMsgPush(pOther, pSelf);
}

/** @brief Responds to player fireballs. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool BombHeiLauncher::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }
    return false;
}

/** @brief Plays the idle animation while waiting for the switch. */
void BombHeiLauncher::exeWaitStart() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}

/** @brief Waits until a pooled Bob-omb becomes available. */
void BombHeiLauncher::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    if (tryGetDeadBombHei())
        al::setNerve(this, &NrvBombHeiLauncherLaunchInterval);
}

/** @brief Finds an inactive Bob-omb whose carryable bomb is also gone. @return Available Bob-omb, or null. */
BombHei* BombHeiLauncher::tryGetDeadBombHei() const {
    for (s32 i = 0; i < mBombs.size(); ++i)
        if (mBombs[i]->isDeadWithBomb())
            return mBombs[i];
    return nullptr;
}

/** @brief Waits three seconds before preparing another launch. */
void BombHeiLauncher::exeLaunchInterval() {
    if (al::isStep(this, 180))
        al::setNerve(this, &NrvBombHeiLauncherLaunchReady);
}

/** @brief Positions a Bob-omb and waits for the launch cue. */
void BombHeiLauncher::exeLaunchReady() {
    if (al::isFirstStep(this)) {
        mLaunchBomb = getDeadBombHei();
        mLaunchBomb->appearLaunchReady(this);
        al::startAction(this, "Wait");
    }
    if (al::isStep(this, 60))
        al::setNerve(this, &NrvBombHeiLauncherLaunch);
}

/** @brief Gets the next available Bob-omb. @return Available Bob-omb, or null. */
BombHei* BombHeiLauncher::getDeadBombHei() const { return tryGetDeadBombHei(); }

/** @brief Launches the prepared Bob-omb and returns to waiting. */
void BombHeiLauncher::exeLaunch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Launch");
        mLaunchBomb->launch();
        mLaunchBomb = nullptr;
    }
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvBombHeiLauncherWait);
}

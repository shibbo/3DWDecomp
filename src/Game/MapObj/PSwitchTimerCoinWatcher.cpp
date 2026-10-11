#include "MapObj/PSwitchTimerCoinWatcher.hpp"
#include "MapObj/TimerCoinHolder.hpp"
#include "MapObj/TrampleSwitch.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
    NERVE_DECL(PSwitchTimerCoinWatcher, Wait);
    NERVE_DECL(PSwitchTimerCoinWatcher, WaitToInstantReset);
    NERVE_DECL(PSwitchTimerCoinWatcher, WaitToReset);
    NERVE_DECL(PSwitchTimerCoinWatcher, Success);
    NERVES_MAKE_NOSTRUCT(PSwitchTimerCoinWatcher, Wait, WaitToInstantReset, WaitToReset, Success)
}
PSwitchTimerCoinWatcher::PSwitchTimerCoinWatcher(const char* name) : al::LiveActor(name) {}
PSwitchTimerCoinWatcher::~PSwitchTimerCoinWatcher() {}
void PSwitchTimerCoinWatcher::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initStageSwitch(this, info);
    al::initExecutorWatchObj(this, info);
    mHasViewGroup = al::calcLinkChildNum(info, "ViewGroup") > 0;
    if (mHasViewGroup) al::initActorClipping(this, info);
    al::initNerve(this, &NrvPSwitchTimerCoinWatcherWait, 0);
    int count = al::calcLinkChildNum(info, "WatchTimerCoin");
    al::tryGetArg(&mResetDelay, info, "SwitchOnDelayStep");
    mCoinHolders.tryAllocBuffer(count, nullptr);
    ProjectActorFactory factory;
    for (int i = 0; i < count; ++i) {
        auto* holder = static_cast<TimerCoinHolder*>(al::createLinksActorFromFactory(factory, info, "WatchTimerCoin", i));
        mCoinHolders[i] = holder;
        al::invalidateClipping(holder);
    }
    al::calcLinkChildNum(info, "SourceSwitch");
    mSwitch = static_cast<TrampleSwitch*>(al::createLinksActorFromFactory(factory, info, "SourceSwitch", 0));
    al::listenStageSwitchOnAppear(this, al::Functor(this, &PSwitchTimerCoinWatcher::switchAppear));
    makeActorDead();
}
void PSwitchTimerCoinWatcher::switchAppear() {
    if (al::isClipped(this)) {
        al::invalidateClipping(this);
        al::LiveActor::appear();
        al::setNerve(this, &NrvPSwitchTimerCoinWatcherWaitToInstantReset);
    } else {
        appear();
        al::invalidateClipping(this);
    }
}
void PSwitchTimerCoinWatcher::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvPSwitchTimerCoinWatcherWait);
}
void PSwitchTimerCoinWatcher::startClipped() {
    for (int i = 0; i < mCoinHolders.size(); ++i) mCoinHolders[i]->forceReset();
    mSwitch->resetSwitch();
    al::LiveActor::startClipped();
}
void PSwitchTimerCoinWatcher::endClipped() {
    al::LiveActor::endClipped();
    kill();
}
void PSwitchTimerCoinWatcher::exeWait() {
    if (al::isFirstStep(this)) al::validateClipping(this);
    int complete = 0;
    int dead = 0;
    for (int i = 0; i < mCoinHolders.size(); ++i) {
        TimerCoinHolder* holder = mCoinHolders[i];
        if (al::isDead(holder)) {
            ++dead;
            complete += holder->isComplete();
        }
    }
    if (dead == mCoinHolders.size()) {
        if (complete == dead) {
            al::setNerve(this, &NrvPSwitchTimerCoinWatcherSuccess);
            kill();
        } else al::setNerve(this, &NrvPSwitchTimerCoinWatcherWaitToReset);
    }
}
void PSwitchTimerCoinWatcher::exeSuccess() {}
void PSwitchTimerCoinWatcher::exeWaitToReset() {
    if (al::isGreaterEqualStep(this, mResetDelay)) {
        mSwitch->resetSwitch();
        kill();
    }
}
void PSwitchTimerCoinWatcher::exeWaitToInstantReset() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mCoinHolders.size(); ++i) mCoinHolders[i]->forceReset();
    }
    if (al::isGreaterEqualStep(this, 20)) {
        mSwitch->resetSwitch();
        kill();
    }
}

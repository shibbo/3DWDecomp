#include "MapObj/SwitchClippingDirector.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Scene/ProjectActorFactory.hpp"

SwitchClippingDirector::SwitchClippingDirector(const char* pName) : al::LiveActor(pName) {
}

void SwitchClippingDirector::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    int count = al::calcLinkChildNum(rInfo, "ClippingTarget");
    mTargets.allocBuffer(count > 1 ? count : 1, nullptr);
    ProjectActorFactory factory;
    for (int i = 0; i < count; ++i) {
        auto* target = new Target;
        target->actor = al::createLinksActorFromFactory(factory, rInfo, "ClippingTarget", i);
        target->wasDead = false;
        mTargets.pushBack(target);
    }

    al::listenStageSwitchOn(this, "SwitchKill", al::Functor(this, &SwitchClippingDirector::kill));
    if (al::listenStageSwitchOn(this, "SwitchAppear", al::Functor(this, &SwitchClippingDirector::appear))) {
        kill();
    } else {
        appear();
    }
}

void SwitchClippingDirector::appear() {
    al::LiveActor::appear();
    int count = mTargets.size();
    for (int i = 0; i < count; ++i) {
        auto* target = mTargets(i);
        if (!target->wasDead) {
            target->actor->makeActorAppeared();
        }
    }
}

// Preserve each target's prior state so an already dead actor is not revived.
void SwitchClippingDirector::kill() {
    al::LiveActor::kill();
    int count = mTargets.size();
    for (int i = 0; i < count; ++i) {
        mTargets(i)->wasDead = al::isDead(mTargets(i)->actor);
        mTargets(i)->actor->makeActorDead();
    }
}

SwitchClippingDirector::~SwitchClippingDirector() {
}

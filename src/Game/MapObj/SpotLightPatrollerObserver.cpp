#include "MapObj/SpotLightPatrollerObserver.hpp"
#include "MapObj/SpotLightPatroller.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(SpotLightPatrollerObserver, Patrol);
    NERVE_DECL(SpotLightPatrollerObserver, Alert);
    NERVES_MAKE_NOSTRUCT(SpotLightPatrollerObserver, Patrol, Alert)
}
SpotLightPatrollerObserver::SpotLightPatrollerObserver(const char* pName) : al::LiveActor(pName) {}
SpotLightPatrollerObserver::~SpotLightPatrollerObserver() {}
void SpotLightPatrollerObserver::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorMapObjMovement(this, rInfo);
    mPatrollerCount = al::calcLinkChildNum(rInfo, "SpotLightPatroller");
    mPatrollers = new SpotLightPatroller*[mPatrollerCount];
    for (int i = 0; i < mPatrollerCount; ++i) {
        mPatrollers[i] = new SpotLightPatroller("監視スポットライト");
        al::initLinksActor(mPatrollers[i], rInfo, "SpotLightPatroller", i);
        mPatrollers[i]->setObserved(true);
    }
    al::initNerve(this, &NrvSpotLightPatrollerObserverPatrol, 0);
    makeActorAppeared();
}
void SpotLightPatrollerObserver::exePatrol() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mPatrollerCount; ++i)
            mPatrollers[i]->setPatrol();
    }
    for (int i = 0; i < mPatrollerCount; ++i) {
        if (mPatrollers[i]->isPlayerWatch()) {
            al::setNerve(this, &NrvSpotLightPatrollerObserverAlert);
            return;
        }
    }
}
void SpotLightPatrollerObserver::exeAlert() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mPatrollerCount; ++i)
            mPatrollers[i]->setAlert();
    }
    for (int i = 0; i < mPatrollerCount; ++i) {
        if (mPatrollers[i]->isPlayerWatch())
            return;
    }
    if (al::isGreaterEqualStep(this, 30))
        al::setNerve(this, &NrvSpotLightPatrollerObserverPatrol);
}

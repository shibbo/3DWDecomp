#include "Enemy/KuriboClimbSyncObj.hpp"
#include "Enemy/KuriboClimbRail.hpp"
#include "Library/ActorUtil.hpp"

/** @brief Creates the synchronized climbing enemy group. @param pName Actor name. */
KuriboClimbSyncObj::KuriboClimbSyncObj(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates the linked climbing enemies. @param rInfo Placement information. */
void KuriboClimbSyncObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    s32 count = al::calcLinkChildNum(rInfo, "ChildKuriboClimb");
    mActors.allocBuffer(count, nullptr);
    for (s32 i = 0; i < count; ++i) {
        auto* pActor = new KuriboClimbRail("\u30cd\u30b3\u30af\u30ea\u30dc\u30fc");
        al::initLinksActor(pActor, rInfo, "ChildKuriboClimb", i);
        pActor->setBodyAttackController(this);
        mActors.pushBack(pActor);
    }
    makeActorAppeared();
}

/** @brief Starts a synchronized attack when every living member is ready. */
void KuriboClimbSyncObj::control() {
    bool allDead = true;
    bool allReady = true;
    for (s32 i = 0; i < mActors.size(); ++i) {
        if (!al::isDead(mActors[i])) {
            allReady &= mActors[i]->isEnableBodyAttack();
            allDead = false;
        }
    }
    if (allDead) {
        kill();
        return;
    }
    if (allReady) {
        for (s32 i = 0; i < mActors.size(); ++i) {
            if (!al::isDead(mActors[i]))
                mActors[i]->startBodyAttack();
        }
    }
}

#include "MapObj/CollisionSearchObj.hpp"
#include <prim/seadDelegate.h>
#include "Library/ActorUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Collision/CollisionUtil.hpp"

CollisionSearchObj::CollisionSearchObj(const char* pName) : al::LiveActor(pName) {}
CollisionSearchObj::~CollisionSearchObj() {}

void CollisionSearchObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::invalidateClipping(this);
    al::listenStageSwitchOnAppear(this, al::Functor(this, &CollisionSearchObj::appear));
    al::listenStageSwitchOnKill(this, al::Functor(this, &CollisionSearchObj::kill));
    mParts.allocBuffer(100, nullptr);
    makeActorDead();
}

void CollisionSearchObj::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    sead::Delegate1<CollisionSearchObj, al::CollisionParts*> delegate(this, &CollisionSearchObj::gatherCollisionParts);
    alCollisionUtil::searchCollisionParts(this, al::getTrans(this), mSearchRadius, delegate);
}

void CollisionSearchObj::gatherCollisionParts(al::CollisionParts* pParts) {
    mParts.pushBack(pParts);
}

void CollisionSearchObj::appear() {
    al::LiveActor::appear();
    alCollisionUtil::validateCollisionPartsPtrArray(this, &mParts);
}

void CollisionSearchObj::kill() {
    al::LiveActor::kill();
    alCollisionUtil::invalidateCollisionPartsPtrArray(this);
}

void CollisionSearchObj::control() {
    mParts.clear();
    sead::Delegate1<CollisionSearchObj, al::CollisionParts*> delegate(this, &CollisionSearchObj::gatherCollisionParts);
    alCollisionUtil::searchCollisionParts(this, al::getTrans(this), mSearchRadius, delegate);
}

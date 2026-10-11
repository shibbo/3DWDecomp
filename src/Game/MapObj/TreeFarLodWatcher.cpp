#include "MapObj/TreeFarLodWatcher.hpp"
#include "MapObj/Tree.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Util/AreaObjUtil.hpp"

TreeFarLodWatcher::TreeFarLodWatcher(const char* name) : al::LiveActor(name) {}
TreeFarLodWatcher::~TreeFarLodWatcher() {}
void TreeFarLodWatcher::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initExecutorWatchObj(this, info);
    mIslandId = info.mPlacementInfo->_28;
    int count = al::calcLinkChildNum(info, "WatchTreeLod");
    ProjectActorFactory factory;
    mTrees.allocBuffer(count, nullptr);
    mRespawnedTrees.allocBuffer(count, nullptr);
    for (int i = 0; i < count; ++i) {
        auto* tree = static_cast<Tree*>(al::createLinksActorFromFactory(factory, info, "WatchTreeLod", i));
        tree->setRespawnByWatcher();
        mTrees.pushBack(tree);
    }
    makeActorAppeared();
}
bool TreeFarLodWatcher::isPlayerOnDifferentIsland() const {
    auto* group = rc::tryFindAreaObjGroup(this, rc::AreaObjType::IslandArea);
    if (!group) return false;
    sead::Vector3f pos = al::getTrans(al::tryFindNearestPlayerActor(this));
    auto* area = group->getInVolumeAreaObj(pos);
    if (!area) return false;
    return mIslandId != area->getPlacementInfo()._28;
}
void TreeFarLodWatcher::respawnTrees() {
    for (int i = 0; i < mTrees.size(); ++i) {
        if (al::isDead(mTrees[i])) {
            mRespawnAlpha = 0.0f;
            mTrees[i]->forceRespawn();
            Tree* tree = mTrees.unsafeAt(i);
            tree->mGlobalAlphaLastFrame = mRespawnAlpha;
            mRespawnedTrees.pushBack(mTrees[i]);
        }
    }
}
void TreeFarLodWatcher::control() {
    if (mRespawnPending) {
        if (isPlayerOnDifferentIsland()) {
            mRespawnPending = false;
            respawnTrees();
        }
    } else if (mRespawnAlpha < 1.0f) {
        mRespawnAlpha = sead::Mathf::min(1.0f, mRespawnAlpha + 1.0f / 60.0f);
        for (int i = 0; i < mRespawnedTrees.size(); ++i)
            mRespawnedTrees.unsafeAt(i)->mGlobalAlphaLastFrame = mRespawnAlpha;
    } else {
        if (mFarLodPending) {
            for (int i = 0; i < mTrees.size(); ++i) mTrees.unsafeAt(i)->startFarLod();
            al::LiveActor::startFarLod();
            mFarLodPending = false;
        }
        mRespawnedTrees.clear();
    }
}
void TreeFarLodWatcher::startFarLod() {
    mFarLodPending = true;
    for (int i = 0; i < mTrees.size(); ++i) {
        if (al::isDead(mTrees[i])) {
            mRespawnPending = true;
            break;
        }
    }
}
void TreeFarLodWatcher::endFarLod() {
    mFarLodPending = false;
    mRespawnPending = false;
    for (int i = 0; i < mTrees.size(); ++i) mTrees.unsafeAt(i)->endFarLod();
    al::LiveActor::endFarLod();
}
void TreeFarLodWatcher::startClipped() {
    mRespawnAlpha = 1.0f;
    for (int i = 0; i < mRespawnedTrees.size(); ++i)
        mRespawnedTrees.unsafeAt(i)->mGlobalAlphaLastFrame = mRespawnAlpha;
    mRespawnedTrees.clear();
    if (mFarLodPending && !mRespawnPending) {
        for (int i = 0; i < mTrees.size(); ++i) mTrees.unsafeAt(i)->startFarLod();
        al::LiveActor::startFarLod();
        mFarLodPending = false;
    }
    al::LiveActor::startClipped();
}

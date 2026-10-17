#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class Tree;
class TreeFarLodWatcher : public al::LiveActor {
public:
    explicit TreeFarLodWatcher(const char*);
    ~TreeFarLodWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void startFarLod() override;
    void endFarLod() override;
    void startClipped() override;
    bool isPlayerOnDifferentIsland() const;
    void respawnTrees();
private:
    sead::PtrArray<Tree> mTrees;
    sead::PtrArray<Tree> mRespawnedTrees;
    bool mFarLodPending = false;
    bool mRespawnPending = false;
    float mRespawnAlpha = 0.0f;
    int mIslandId = -1;
};
static_assert(sizeof(TreeFarLodWatcher) == 0x178);

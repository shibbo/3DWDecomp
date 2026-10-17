#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "container/seadPtrArray.h"
class GhostPlayer;
class GhostPlayerLoaderBase;
class GhostPlayerPlayer : public al::LiveActor {
public:
    explicit GhostPlayerPlayer(const char* pName);
    ~GhostPlayerPlayer() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void update();
    bool tryStartPlay();
    void tryStartFromObj(const char* pId);
    bool tryEndPlay();
    bool waitForStartFromWarpObj();
    void stopAndHideGhostPlayerAll();
    void restartGhostPlayerAll();

private:
    GhostPlayerLoaderBase* mLoader;
    sead::FixedPtrArray<GhostPlayer, 3> mPlayers;
    sead::FixedPtrArray<GhostPlayer, 3> mActivePlayers;
};
static_assert(sizeof(GhostPlayerPlayer) == 0x1a0);

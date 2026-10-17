#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class GhostPlayer;
class GhostPlayerLoaderBase;

/** @brief Coordinates up to three replay actors alongside the human players. */
class GhostPlayerPlayer : public al::LiveActor {
public:
    GhostPlayerPlayer(const char*);
    void init(const al::ActorInitInfo&) override;
    void update();
    bool tryStartPlay();
    void tryStartFromObj(const char*);
    bool tryEndPlay();
    bool waitForStartFromWarpObj();
    void stopAndHideGhostPlayerAll();
    void restartGhostPlayerAll();
private:
    GhostPlayerLoaderBase* mLoader = nullptr;
    sead::FixedPtrArray<GhostPlayer, 3> mGhosts;
    sead::FixedPtrArray<GhostPlayer, 3> mActiveGhosts;
};
static_assert(sizeof(GhostPlayerPlayer) == 0x1a0);

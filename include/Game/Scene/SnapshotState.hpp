#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al {
class Scene;
}
namespace rc {
class StampDirector;
}
class DrcAssistDirectorList;
class PlayerAliveWatcher;
class SnapshotLayout;
class SnapshotState : public al::NerveStateBase {
public:
    SnapshotState(SnapshotLayout* layout, al::Scene* scene, DrcAssistDirectorList* assist,
                  rc::StampDirector* stamps, PlayerAliveWatcher* players);
    ~SnapshotState() override = default;
    void init() override;
    void appear() override;
    void kill() override;
    void exeWait();
    void exeFinish();
    void updateGraphics();

private:
    al::Scene* mScene;
    SnapshotLayout* mLayout;
    DrcAssistDirectorList* mAssist;
    rc::StampDirector* mStamps;
    PlayerAliveWatcher* mPlayers;
    s32 mDefaultFilter = 0;
};
static_assert(sizeof(SnapshotState) == 0x48);

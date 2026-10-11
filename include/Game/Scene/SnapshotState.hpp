#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class Scene;
}  // namespace al

namespace rc {
class StampDirector;
}  // namespace rc

class DrcAssistDirectorList;
class PlayerAliveWatcher;
class SnapshotLayout;

/**
 * @brief Scene state of the snapshot mode.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class SnapshotState : public al::NerveStateBase {
public:
    SnapshotState(SnapshotLayout* pLayout, al::Scene* pScene,
                  DrcAssistDirectorList* pDrcAssistDirectorList, rc::StampDirector* pStampDirector,
                  PlayerAliveWatcher* pPlayerAliveWatcher);

private:
    u8 _pad[0x48 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(SnapshotState) == 0x48);

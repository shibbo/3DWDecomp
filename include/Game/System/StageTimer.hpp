#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class StageDataHolder;

/** @brief Stage countdown interface used by the stage-entry timer event. */
class StageTimer : public al::LayoutActor, public al::ISceneObj {
public:
    StageTimer(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
               al::LayoutActor* pParent, StageDataHolder* pStageDataHolder,
               const char* pStageName);

    bool tryStartHurryUp();
    bool isCountDown() const;
    bool isStop() const;
    void requestStop(const void*);
    void requestRestart(const void*);
    s32 calcDisplayCount() const;
    al::LayoutActor* getTimeUpLayout() const;
    void deactivate();
    void setTimerCheckpoint();
    void clearStage();
    bool isTimeUp() const;
    void startDemo();
    void endDemo();

private:
    u8 _130[0x1a0 - 0x130];  // TODO: layout not decompiled yet.
};

static_assert(sizeof(StageTimer) == 0x1a0);

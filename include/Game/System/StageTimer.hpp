#pragma once

#include <container/seadPtrArray.h>

#include "Layout/TextBoxTextInfo.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class LayoutInitInfo;
class SimpleLayoutAppearWait;
}  // namespace al

class StageDataHolder;

/** @brief Stage countdown timer shown in the stage HUD ("TxtTimer" panes). */
class StageTimer : public al::LayoutActor, public al::ISceneObj {
public:
    StageTimer(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
               al::LayoutActor* pParent, StageDataHolder* pStageDataHolder,
               const char* pStageName);

    bool isCountDown() const;
    bool isStop() const;
    bool isTimeUp() const;
    void setTimerCheckpoint();
    void setTimerCountClearStage(s32 count);
    s32 calcDisplayCount() const;
    void clearStage();
    void deactivate();
    void tryStartHurryUp();
    void startHurryUp();
    void requestStop(const void* pRequester);
    void requestRestart(const void* pRequester);
    void startDemo();
    void endDemo();
    al::LayoutActor* getTimeUpLayout() const;
    void endHurryUp();
    void tryRevertVolumeForSeLikeBgm();

    void exeCountDown();
    void exeTimeUpDelay();
    void exeTimeUp();
    void exeClearStage();
    void exeStop();
    void exeDeactive();

    void control() override;

    /**
     * @brief Gets the scene object name.
     * @return "ステージタイマー" (stage timer).
     */
    const char* getSceneObjName() const override { return "ステージタイマー"; }

private:
    /** @brief Opaque type of the objects that stopped the timer. */
    struct StopRequester;
    typedef sead::PtrArray<const StopRequester> StopRequesterArray;

    al::LayoutActor* mParent = nullptr;                    // 0x130
    StageDataHolder* mStageDataHolder = nullptr;           // 0x138
    bool mIsHurryUp = false;                               // 0x140
    const char* mSeLikeBgmName = nullptr;                  // 0x148
    bool mIsDemo = false;                                  // 0x150
    al::SimpleLayoutAppearWait* mTimeUpLayout = nullptr;   // 0x158
    s32 mPrevTimerFrame = 0;                               // 0x160
    StopRequesterArray mStopRequesters;                    // 0x168
    bool mIsLongTimer = false;                             // 0x178
    fix::TextBoxTextInfo mTimerTextInfo;                   // 0x180
};

static_assert(sizeof(StageTimer) == 0x1a0);

#pragma once

#include <container/seadPtrArray.h>

#include "Library/Scene/Scene.hpp"

namespace sead {
class Event;
class Viewport;
}  // namespace sead

namespace al {
class ActorInitInfo;
class Nerve;
class PadRumbleDirector;
class ScreenCaptureExecutor;
class StageInfo;
class WipeSimple;
}  // namespace al

class CourseSelectLayoutKiosk;
class GameDataFile;
class GameDataHolder;
class PlayerActor;
class PlayerEntry;
class PlayerRetargettingSelector;
class RCSControlGuideBar;
class RCS_SaveDataLayout;
class StageWipeKeeper;
class TitleLogo;
class WindowConfirm;

/**
 * Title screen scene: plays a recorded title demo in the background and runs the title menu
 * (file select, character select, Luigi Bros, kiosk course select...).
 */
class TitleScene : public al::Scene {
public:
    TitleScene(StageWipeKeeper* pStageWipeKeeper, s32 demoId);
    ~TitleScene() override;

    void init(const al::SceneInitInfo& rInfo) override;
    const char* getDemoStageName() const;
    void initPlacement(const al::ActorInitInfo& rInfo);
    void initPadReplayData();
    void appear() override;
    void setAppearWait();
    void setTitleNerve(bool isUnused);
    void appearFromBoot();
    void appearFromLoad();
    void kill() override;
    void control() override;
    bool isTitleDraw3D() const;
    void update3D();
    void drawMain_() const override;
    void drawSub_() const override;
    bool isReturnToTopMenu() const;
    bool isAudioReady();

    void exeLoad();
    void exeAppearWait();
    void decidePlayerPlacement();
    void startReplay();
    void exeTitle();
    void pauseReplay();
    void exeNewGameWarning();
    void endTitle();
    void exeConfirmLuigi();
    void exeFileSelect();
    void exeDeleteFile();
    void exePlayerSelect();
    bool isReplayActive() const;
    void exeReturnPlayerSelect();
    void exeWipeCloseTopMenu();
    void exeWipeClose();
    void exeWipeCloseLuigiBros();
    void exeWipeOpen();
    void exeRestart();
    void exeWaitTextFade();
    void exeCourseSelect();

    bool isAllPlayerDecideTrigger() const;
    bool isReadyShow() const;
    bool isRestart() const;
    bool isRestartWhiteFade() const;

    void initPlacementPlayer(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             PlayerRetargettingSelector* pSelector);
    void initPlacementSky(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo);
    void initPlacementObject(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                             const char* pListName);

    /**
     * Gets whether the Luigi Bros mini game was chosen on the title screen.
     * @return true if Luigi Bros was started.
     */
    bool isLuigiBros() const { return mIsLuigiBros; }

private:
    GameDataHolder* mGameDataHolder = nullptr;
    GameDataFile* mTitleDemoFile = nullptr;
    StageWipeKeeper* mStageWipeKeeper;
    sead::Viewport* mMainViewport = nullptr;
    sead::Viewport* mSubViewport = nullptr;
    al::ScreenCaptureExecutor* mScreenCaptureExecutor;
    TitleLogo* mTitleLogo = nullptr;
    RCSControlGuideBar* mControlGuideBar;
    RCS_SaveDataLayout* mSaveDataLayout = nullptr;
    PlayerEntry* mPlayerEntry = nullptr;
    WindowConfirm* mWindowConfirm = nullptr;
    CourseSelectLayoutKiosk* mCourseSelectLayout;
    al::WipeSimple* mWipe = nullptr;
    const al::Nerve* mWipeOpenNextNerve = nullptr;
    bool mIsLuigiBros = false;
    bool* mCameraTitleFlag = nullptr;
    void* _168 = nullptr;
    void* _170 = nullptr;
    sead::FixedPtrArray<PlayerActor, 8> mPlayers;
    s32 mPlayerNum = 1;
    s32 mDemoId;
    al::PadRumbleDirector* mPadRumbleDirector = nullptr;
    bool mIsPlayerChangeDemo = false;
    bool mIsAppearWaitSkip = false;
    bool mIsAppearFromBoot = false;
    bool mIsAppearFromLoad = false;
    bool mIsStartBgm = false;
    s32 mLastPlayingFileId = 0;
    sead::Event* mAudioReadyEvent;
};

static_assert(sizeof(TitleScene) == 0x1f0);

#pragma once

#include "Layout/TopMenu.hpp"
#include "Library/Scene/Scene.hpp"

namespace al {
class ScreenCaptureExecutor;
}  // namespace al
namespace sead {
class Viewport;
}  // namespace sead
class GameDataHolder;
class RCS_SaveDataLayout;
class RCSControlGuideBar;
class StageWipeKeeper;
class WindowConfirm;

/**
 * @brief Menu the top menu scene is in, as seen by the product sequence.
 */
enum TopMenuSceneMode : s32 {
    TopMenuSceneMode_TopMenu = 0,          ///< Choosing between the two games.
    TopMenuSceneMode_SingleModeTitle = 2,  ///< In the Bowser's Fury title menu.
};

/**
 * @brief Scene of the top menu choosing between Super Mario 3D World and Bowser's Fury.
 */
class TopMenuScene : public al::Scene {
public:
    TopMenuScene(StageWipeKeeper* pStageWipeKeeper);
    ~TopMenuScene() override;
    void init(const al::SceneInitInfo& rInfo) override;
    virtual void appear(TopMenuAppearState state, bool isFirstAppear);
    void control() override;
    void drawMain_() const override;
    bool isSelectAny();
    void startSingleModeTitleMenu();
    void cancelSingleModeTitleMenu();
    void cancelSingleModeTitleMenuAudio(s32 fadeFrames);
    void exeInit();
    void exeInitError();
    void exeLoadSaveData();
    void exeWriteSaveData();
    void exeTopMenu();
    void exeTopMenuEnd();
    void exeSingleModeTitleAppear();
    void exeSingleModeTitle();
    void exeNewGameWarning();
    void exeFileSelect();
    void exeDeleteFile();
    void exeEndFade();
    void exeWaitTextFade();

private:
    GameDataHolder* mGameDataHolder = nullptr;
    sead::Viewport* mViewport = nullptr;
    TopMenuSceneMode mMode = TopMenuSceneMode_TopMenu;
    TopMenu* mTopMenu = nullptr;
    RCSControlGuideBar* mControlGuideBar = nullptr;
    StageWipeKeeper* mStageWipeKeeper;
    WindowConfirm* mWindowConfirm = nullptr;
    RCS_SaveDataLayout* mSaveDataLayout = nullptr;
    al::ScreenCaptureExecutor* mScreenCaptureExecutor;
    bool mIsSaveInit = true;
    bool mIsSaveDataLoaded = false;
    bool mIsFirstAppear = true;
    bool mIsStartSingleMode;
    TopMenuAppearState mAppearState = TopMenuAppearState_3DWorld;
};
static_assert(sizeof(TopMenuScene) == 0x138);

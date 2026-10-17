#pragma once

#include <basis/seadTypes.h>
#include "Layout/TextBoxTextInfo.hpp"
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class CameraDirector;
class CameraDirector_RS;
class GamePadSystem;
class LayoutInitInfo;
class PlayerHolder;
class ScreenCaptureExecutor;
class WipeSimple;
}  // namespace al
class ButtonGroup;
class ControlGuide;
class GameDataHolder;
class OptionsMenu;
class PlayerAliveWatcher;
class RCSControlGuideBar;
class RCS_SaveDataLayout;
class SimpleMenuLayout;
class WindowConfirm;

/**
 * @brief In-stage pause menu of both Super Mario 3D World and Bowser's Fury.
 *
 * Hosts the main button list (continue, restart, return to map, controller, data, guide, options,
 * quit and the Bowser's Fury assist mode) together with the sub-menus it opens.
 */
class PauseMenu : public al::LayoutActor {
public:
    PauseMenu(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
              const PlayerAliveWatcher* pPlayerAliveWatcher, al::CameraDirector* pCameraDirector,
              al::PlayerHolder* pPlayerHolder, al::GamePadSystem* pGamePadSystem,
              al::ScreenCaptureExecutor* pScreenCaptureExecutor);
    PauseMenu(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
              const PlayerAliveWatcher* pPlayerAliveWatcher, al::CameraDirector_RS* pCameraDirector,
              al::PlayerHolder* pPlayerHolder, al::GamePadSystem* pGamePadSystem,
              al::ScreenCaptureExecutor* pScreenCaptureExecutor, al::WipeSimple* pWipe);

    void control() override;
    void appear(s32 port);
    void appearKoopaJrOptions();
    void forceExit();
    void end();
    void decideBack();

    void exeAppear();
    void exeWait();
    void exeDeciding();
    void exeEnd();
    void exeFadeAssistMode();
    void exeHandleAssistMode();
    void exeOptions();
    void exeKoopaJrOptions();
    void exeSaveDataMenu();
    void exeDataManagement();
    void exeWaitLoad();
    void exeGuide();
    void exeConfirm();
    void exeEndAssistMode();

    bool isDecideBack() const;
    bool isDecideMap() const;
    bool isDecideReenterStage() const;
    bool isDecideQuit() const;
    bool isDecideGameChange() const;
    bool isEndLoad() const;
    bool isWait() const;
    bool isEnd() const;
    bool isEndAssistMode() const;

    /**
     * Gets whether the menu was closed because its controller was disconnected.
     * @return true if the menu was force exited.
     */
    bool isForceExit() const { return mIsForceExit; }
    void setPauseMenuHeader(s32 islandNo);

    /** @brief The menu is only opened through appear(s32), which knows the controller port. */
    void appear() override {}

private:
    GameDataHolder* mGameDataHolder;
    const PlayerAliveWatcher* mPlayerAliveWatcher;
    al::PlayerHolder* mPlayerHolder;
    s32 mPort = -1;
    ButtonGroup* mButtonGroup = nullptr;
    WindowConfirm* mWindowConfirm = nullptr;
    SimpleMenuLayout* mSaveDataMenu = nullptr;
    RCS_SaveDataLayout* mSaveDataLayout = nullptr;
    OptionsMenu* mOptionsMenu = nullptr;
    ControlGuide* mControlGuide = nullptr;
    RCSControlGuideBar* mGuideBar = nullptr;
    al::LayoutActor* mMenuFrameParts = nullptr;
    al::WipeSimple* mWipe;
    al::GamePadSystem* mGamePadSystem;
    al::ScreenCaptureExecutor* mScreenCaptureExecutor;
    bool mIsExitDisabled = false;
    bool mIsSingleMode;
    bool mIsForceExit = false;
    const char* mSaveDataMenuDecision;
    const char* mLastSubMenuButtonName;
    fix::TextBoxTextInfo mCourseNameTextInfo;
};
static_assert(sizeof(PauseMenu) == 0x1d8);

namespace PauseMenuFunction {
void validateButtonGroup(ButtonGroup* pButtonGroup, bool isExitDisabled, bool isSingleMode);
}  // namespace PauseMenuFunction

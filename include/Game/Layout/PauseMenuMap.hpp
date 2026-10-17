#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class CameraDirector;
class LayoutInitInfo;
class PlayerHolder;
class ScreenCaptureExecutor;
}  // namespace al

class ControlGuide;
class CourseSelectDirector;
class ButtonGroup;
class GameDataHolder;
class OptionsMenu;
class RCSControlGuideBar;
class RCS_SaveDataLayout;
class SimpleMenuLayout;
class WindowConfirm;

/**
 * @brief Pause menu of the course select map.
 *
 * Hosts the main button list (continue, controller, data, guide, options, quit and leaving a
 * multiplayer session) together with the sub-menus it opens.
 */
class PauseMenuMap : public al::LayoutActor {
public:
    PauseMenuMap(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                 CourseSelectDirector* pDirector, al::CameraDirector* pCameraDirector,
                 al::PlayerHolder* pPlayerHolder, RCSControlGuideBar* pGuideBar,
                 al::ScreenCaptureExecutor* pScreenCaptureExecutor);

    void appear(s32 port, s32 worldId);
    bool isEnableLeaveUser();
    void setButtonTargets();
    bool isEnableControllerChange(s32 port) const;
    void decideBack();
    void forceExit();
    void end();
    void updateWorldStageString();

    void exeAppear();
    void exeWait();
    void exeDeciding();
    void exeEnd();
    void exeOptions();
    void exeSaveDataMenu();
    void exeDataManagement();
    void exeWaitLoad();
    void exeGuide();
    void exeControllerChange();

    bool isDecideBack() const;
    bool isDecideGoToTitle() const;
    bool isDecideLeaveGame() const;
    bool isDecideLoad() const;
    bool isWait() const;
    bool isEnd() const;
    bool isEndGoToTitle() const;
    bool isControllerChange() const;
    void setControlGuideBar(RCSControlGuideBar* pGuideBar);

    /** @brief The menu is only opened through appear(s32, s32), which knows the port. */
    void appear() override {}

    /**
     * Gets the pad port the menu is operated with.
     * @return The pad port.
     */
    s32 getPort() const { return mPort; }

private:
    CourseSelectDirector* mDirector;
    GameDataHolder* mGameDataHolder;
    s32 mPort = -1;
    s32 mUserId = -1;
    s32 mWorldId = 0;
    ButtonGroup* mButtonGroup = nullptr;
    bool mIsEnableLeaveUser = false;
    bool mIsEnableControllerChange = true;
    WindowConfirm* mWindowConfirm = nullptr;
    SimpleMenuLayout* mSaveDataMenu = nullptr;
    RCS_SaveDataLayout* mSaveDataLayout = nullptr;
    OptionsMenu* mOptionsMenu = nullptr;
    ControlGuide* mControlGuide = nullptr;
    RCSControlGuideBar* mGuideBar;
    al::LayoutActor* mMenuFrameParts = nullptr;
    const char* mSaveDataMenuDecision = nullptr;
    const char* mLastSubMenuButtonName = nullptr;
    al::ScreenCaptureExecutor* mScreenCaptureExecutor;
};

static_assert(sizeof(PauseMenuMap) == 0x1a8);

#include "Layout/PauseMenuMap.hpp"

#include <attributes.h>

#include "CourseSelect/CourseSelectDirector.hpp"
#include "Layout/ButtonCursorParts.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/ControlGuide.hpp"
#include "Layout/CursorTarget.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/RCS_SaveDataLayout.hpp"
#include "Layout/SimpleMenuLayout.hpp"
#include "Layout/Switch/OptionsMenu.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/PlayerUtil.hpp"

/// Declares a nerve of the map pause menu whose state function may differ from the nerve's name.
#define PAUSE_MENU_MAP_NERVE_DECL(Action, Func)                                                    \
    class PauseMenuMapNrv##Action : public al::Nerve {                                             \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<PauseMenuMap>())->exe##Func();                                     \
        }                                                                                          \
    };

namespace {
PAUSE_MENU_MAP_NERVE_DECL(End, End)
PAUSE_MENU_MAP_NERVE_DECL(Appear, Appear)
PAUSE_MENU_MAP_NERVE_DECL(Deciding, Deciding)
PAUSE_MENU_MAP_NERVE_DECL(WaitLoad, WaitLoad)
PAUSE_MENU_MAP_NERVE_DECL(EndLoad, End)
PAUSE_MENU_MAP_NERVE_DECL(Wait, Wait)
PAUSE_MENU_MAP_NERVE_DECL(EndGoToTitle, End)
PAUSE_MENU_MAP_NERVE_DECL(ControllerChange, ControllerChange)
PAUSE_MENU_MAP_NERVE_DECL(SaveDataMenu, SaveDataMenu)
PAUSE_MENU_MAP_NERVE_DECL(Guide, Guide)
PAUSE_MENU_MAP_NERVE_DECL(Options, Options)
PAUSE_MENU_MAP_NERVE_DECL(DataManagement, DataManagement)
PAUSE_MENU_MAP_NERVE_DECL(SaveDataMenuBack, SaveDataMenu)

NERVES_MAKE_NOSTRUCT(PauseMenuMap, End, Appear, Deciding, WaitLoad, EndLoad, Wait, EndGoToTitle,
                     ControllerChange, SaveDataMenu, Guide, Options, DataManagement,
                     SaveDataMenuBack)

/// Button names of the map pause menu.
constexpr const char* cButtonContinue = "つづける";
constexpr const char* cButtonController = "Controller";
constexpr const char* cButtonData = "Data";
constexpr const char* cButtonGuide = "Guide";
constexpr const char* cButtonOptions = "Options";
constexpr const char* cButtonQuit = "Quit";
constexpr const char* cButtonLeaveGame = "LeaveGame";

/// Message file and button label of the controller change confirmation window.
constexpr const char* cConfirmFileName = "WindowConfirmPauseMenu";
constexpr const char* cConfirmButtonName = "Rボタン";

/// Minimum number of active users for one of them to be able to leave the session.
constexpr s32 cLeaveUserNumMin = 2;

/**
 * @brief Access the cursor layout of a button group.
 * @param pButtonGroup Button group owning the cursor.
 * @return The button cursor parts.
 */
inline ButtonCursorParts* getCursorParts(const ButtonGroup* pButtonGroup) {
    return static_cast<ButtonCursorParts*>(pButtonGroup->getCursor());
}

/**
 * @brief Grey out a button and stop it from being selected.
 * @param pButton Button to disable.
 */
inline void disableButton(CursorTarget* pButton) {
    pButton->disable();
    pButton->invalidate();
}

/**
 * @brief Grey out a button of a group and stop it from being selected.
 * @param pButtonGroup Button group owning the button.
 * @param pName Name of the button to disable.
 */
inline void disableButton(ButtonGroup* pButtonGroup, const char* pName) {
    pButtonGroup->getButton(pName)->disable();
    pButtonGroup->getButton(pName)->invalidate();
}

/**
 * @brief Hide the cursor of a button group and invalidate the buttons that remember the cursor.
 * @param pButtonGroup Button group to deactivate.
 */
inline void deactivateButtonGroup(ButtonGroup* pButtonGroup) {
    pButtonGroup->hideCursor();
    pButtonGroup->getButton(cButtonController)->invalidate();
    pButtonGroup->getButton(cButtonOptions)->invalidate();
}

/**
 * @brief Check whether a sub-menu button was selected; those are remembered for the vertical
 *        cursor moves.
 * @param pName Name of the selected button.
 * @return True for the controller, data, guide and options buttons.
 */
inline bool isSubMenuButton(const char* pName) {
    return al::isEqualString(pName, cButtonController) || al::isEqualString(pName, cButtonData) ||
           al::isEqualString(pName, cButtonGuide) || al::isEqualString(pName, cButtonOptions);
}

/**
 * @brief Reactivates the map pause menu buttons, disabling the ones that can't be used right now.
 * @param pButtonGroup Buttons of the pause menu.
 * @param isNetwork True when the network account is set up (unused).
 * @param isEnableControllerChange True when the controller button can be used.
 * @param isEnableLeaveUser True when the leave game button can be used.
 */
NOINLINE void validateButtonGroup(ButtonGroup* pButtonGroup, bool isNetwork,
                                  bool isEnableControllerChange, bool isEnableLeaveUser) {
    getCursorParts(pButtonGroup)->reset();
    pButtonGroup->showCursor();
    pButtonGroup->validate();

    if (!isEnableLeaveUser) {
        disableButton(pButtonGroup, cButtonLeaveGame);
    }

    if (!isEnableControllerChange) {
        disableButton(pButtonGroup, cButtonController);
    }
}

/**
 * @brief Checks whether the network account is set up.
 * @param accessor Game data.
 * @return True when the network settings were opened and an account is linked.
 */
inline bool isNetworkReady(GameDataHolderAccessor accessor) {
    return GameDataFlagFunction::isAlreadyOpenNetworkSetting(accessor) &&
           GameDataFunction::isNetworkAccount(accessor);
}
}  // namespace

/**
 * @brief Creates the map pause menu.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data of the running session.
 * @param pDirector Course select director of the map.
 * @param pCameraDirector Camera director adjusted by the options.
 * @param pPlayerHolder Players of the scene (unused).
 * @param pGuideBar Guide bar shown below the menu.
 * @param pScreenCaptureExecutor Screen capture shown behind the confirmation window.
 */
PauseMenuMap::PauseMenuMap(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                           CourseSelectDirector* pDirector, al::CameraDirector* pCameraDirector,
                           al::PlayerHolder* pPlayerHolder, RCSControlGuideBar* pGuideBar,
                           al::ScreenCaptureExecutor* pScreenCaptureExecutor)
    : al::LayoutActor("ポーズメニュー"), mDirector(pDirector), mGameDataHolder(pGameDataHolder),
      mGuideBar(pGuideBar), mScreenCaptureExecutor(pScreenCaptureExecutor) {
    al::initLayoutActor(this, rInfo, "PauseMenu", nullptr);
    initNerve(&NrvPauseMenuMapEnd, 0);
    mButtonGroup = new ButtonGroup(rInfo, this, "PauseMenu", "Map", false);
    mOptionsMenu = new OptionsMenu(rInfo, pCameraDirector, false);
    mSaveDataMenu = new SimpleMenuLayout("RCS_SaveDataManage", rInfo, 4, nullptr);
    mSaveDataLayout = new RCS_SaveDataLayout(rInfo, pGameDataHolder, mScreenCaptureExecutor,
                                             mGuideBar, true);
    mControlGuide = new ControlGuide(rInfo, false);
    mWindowConfirm = new WindowConfirm(WindowConfirmType_Double, rInfo, "PauseMenu", false);
    mMenuFrameParts = new al::LayoutActor("RCS_MenuFrameParts");
    al::initLayoutPartsActor(mMenuFrameParts, this, rInfo, "ParFrame", nullptr);
    mSaveDataLayout->setPauseMenuLayout(mMenuFrameParts);
}

/**
 * @brief Opens the pause menu.
 * @param port Controller port that paused the game.
 * @param worldId World the map shows.
 */
void PauseMenuMap::appear(s32 port, s32 worldId) {
    al::startFreezeAction(this, "Appear", 0.0f, nullptr);
    mPort = port;
    mButtonGroup->setPort(port);
    mUserId = rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(mGameDataHolder), mPort);
    mWorldId = worldId;
    mIsEnableLeaveUser = isEnableLeaveUser();

    if (mIsEnableLeaveUser) {
        al::startAction(this, "SetPauseMenuMapMultiplayer", "PauseMenuType");
        mButtonGroup->getButton(cButtonLeaveGame)->enable();
        mButtonGroup->getButton(cButtonLeaveGame)->validate();
    } else {
        al::startAction(this, "SetPauseMenuMap", "PauseMenuType");
        mButtonGroup->getButton(cButtonLeaveGame)->disable();
        mButtonGroup->getButton(cButtonLeaveGame)->invalidate();
    }

    setButtonTargets();
    al::LayoutActor::appear();

    if (mGuideBar != nullptr) {
        mGuideBar->appearWithMessage(RCSControlGuideBar::GuideBarMsgType_PauseMenuMap, mPort);
    }

    mIsEnableControllerChange = isEnableControllerChange(mPort);
    al::setNerve(this, &NrvPauseMenuMapAppear);
    mLastSubMenuButtonName = mIsEnableControllerChange ? cButtonController : cButtonData;
}

/**
 * @brief Checks whether the user who opened the menu may leave the multiplayer session.
 * @return True when several users play and the opening user stands on the ground.
 */
bool PauseMenuMap::isEnableLeaveUser() {
    al::LiveActor* player =
        rc::tryFindAlivePlayerActorFirstByUserId(mDirector->getMainPlayer(), mUserId);

    if (rc::getActiveControlUserNum(GameDataHolderAccessor(mGameDataHolder)) < cLeaveUserNumMin ||
        mDirector->isPlayPuppeterDemo(mUserId)) {
        return false;
    }

    return rc::isPlayerOnGround(player);
}

/** @brief Sets up the cursor destinations of the buttons for the available buttons. */
void PauseMenuMap::setButtonTargets() {
    if (!mIsEnableControllerChange) {
        disableButton(mButtonGroup->getButton(cButtonController));
    }

    const char* sideButton = mIsEnableControllerChange ? cButtonController : cButtonOptions;
    mButtonGroup->resetDestination(cButtonData, mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit,
                                   nullptr, sideButton, nullptr, false);
    mButtonGroup->resetDestination(cButtonQuit, cButtonContinue,
                                   mIsEnableLeaveUser ? cButtonLeaveGame : mLastSubMenuButtonName,
                                   nullptr, nullptr, false);
    mButtonGroup->resetDestination(cButtonController,
                                   mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit, nullptr,
                                   nullptr, nullptr, false);
    mButtonGroup->resetDestination(cButtonData, mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit,
                                   nullptr, nullptr, nullptr, false);
    mButtonGroup->resetDestination(cButtonGuide,
                                   mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit, nullptr,
                                   nullptr, nullptr, false);
    mButtonGroup->resetDestination(cButtonOptions,
                                   mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit, nullptr,
                                   nullptr, nullptr, false);
}

/**
 * @brief Checks whether the controllers may be reassigned.
 * @param port Controller port that operates the menu.
 * @return False while the user of the port plays a puppeteer demo.
 */
bool PauseMenuMap::isEnableControllerChange(s32 port) const {
    if (mDirector == nullptr) {
        return true;
    }

    return !mDirector->isPlayPuppeterDemo(rc::calcControlUserIdByPortNum(port));
}

/** @brief Confirms the continue button as if it had been pressed. */
void PauseMenuMap::decideBack() {
    mButtonGroup->decide(cButtonContinue);
}

/** @brief Closes whatever sub-menu is open, or the whole menu, without animations. */
void PauseMenuMap::forceExit() {
    if (al::isNerve(this, &NrvPauseMenuMapAppear) || al::isNerve(this, &NrvPauseMenuMapDeciding) ||
        al::isNerve(this, &NrvPauseMenuMapEnd) || al::isNerve(this, &NrvPauseMenuMapWaitLoad) ||
        al::isNerve(this, &NrvPauseMenuMapEndLoad)) {
        return;
    }

    if (mOptionsMenu->isAlive()) {
        if (!mOptionsMenu->isEnding()) {
            mOptionsMenu->forceExit();
            al::startAction(this, "AppearSubMenu", "Main");
            al::startAction(mMenuFrameParts, "End_Options", nullptr);
            mButtonGroup->reset();
        }

        return;
    }

    if (mControlGuide->isAlive()) {
        if (!mControlGuide->isEnding()) {
            mControlGuide->requestEnd();
        }

        return;
    }

    if (mWindowConfirm->isAlive()) {
        if (!mWindowConfirm->isEnding()) {
            mWindowConfirm->forceExit();
        }

        return;
    }

    if (mSaveDataLayout->isAlive()) {
        if (!mSaveDataLayout->isEnding()) {
            mSaveDataLayout->forceExit();
        }

        return;
    }

    if (mSaveDataMenu->isAlive()) {
        if (!mSaveDataMenu->isEnding()) {
            mSaveDataMenu->end();
        }

        return;
    }

    if (mIsEnableLeaveUser) {
        mDirector->leaveUser(mUserId);
    }

    al::setNerve(this, &NrvPauseMenuMapEnd);
}

/** @brief Closes the menu. */
void PauseMenuMap::end() {
    al::setNerve(this, &NrvPauseMenuMapEnd);
}

/** @brief Shows the world and the course the cursor of the map is on in the header. */
void PauseMenuMap::updateWorldStageString() {
    s32 courseId = rc::getSelectMiniatureCourseId(mDirector);

    if (GameDataFunction::isInvalidCourseId(courseId) ||
        GameDataFunction::isStageNoCourseName(GameDataHolderAccessor(mGameDataHolder), courseId)) {
        rc::setPaneWorldString(this, this, "TxtWorld", "PauseMenu", "PauseMenuMap_World", mWorldId,
                               nullptr, true);
        al::setPaneString(this, "TxtCourseName", reinterpret_cast<const char16_t*>(""));
        return;
    }

    s32 worldId;
    s32 stageId;
    GameDataFunction::calcWorldAndStageId(this, &worldId, &stageId, courseId);
    rc::setPaneWorldStageString(this, this, "TxtWorld", "PauseMenu", "PauseMenuMap_WorldStage",
                                worldId, stageId, mGameDataHolder, nullptr, true);
    rc::setPaneStageNameString(this, this, "TxtCourseName", mGameDataHolder, courseId);
}

/** @brief Plays the opening animation and shows the player character of the menu's user. */
void PauseMenuMap::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startSe(this, "Appear");
        mButtonGroup->reset();
        getCursorParts(mButtonGroup)->hide();
        updateWorldStageString();
        const char* characterName = rc::getPlayerCharacterName(
            rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder), mUserId));
        al::startAction(mMenuFrameParts, characterName, nullptr);

        if (mGuideBar != nullptr) {
            mGuideBar->setCharacter(characterName);
        }
    }

    if (al::isActionEnd(this, nullptr)) {
        mButtonGroup->select(cButtonContinue);
        validateButtonGroup(mButtonGroup, isNetworkReady(GameDataHolderAccessor(mGameDataHolder)),
                            mIsEnableControllerChange, mIsEnableLeaveUser);
        rc::getActiveInputPortList(GameDataHolderAccessor(mGameDataHolder));
        rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder), mUserId);
        al::setNerve(this, &NrvPauseMenuMapWait);
    }
}

/** @brief Moves the cursor and waits for a button to be confirmed. */
void PauseMenuMap::exeWait() {
    al::isFirstStep(this);

    if (al::isLessStep(this, al::getActionFrameMax(mButtonGroup->getCursor(), "Appear", "Main"))) {
        return;
    }

    mButtonGroup->update();

    if (mButtonGroup->isInputLocked()) {
        return;
    }

    if (rc::isPadTriggerUiUpByPort(mPort)) {
        if (mButtonGroup->isSelect(cButtonContinue)) {
            mButtonGroup->resetDestination(cButtonContinue, mLastSubMenuButtonName, nullptr,
                                           nullptr, nullptr, false);
        }

        mButtonGroup->tryMove(ButtonGroup::Direction_Up);
    }

    if (rc::isPadTriggerUiDownByPort(mPort)) {
        if (mButtonGroup->isSelect(mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit)) {
            mButtonGroup->resetDestination(mIsEnableLeaveUser ? cButtonLeaveGame : cButtonQuit,
                                           nullptr, mLastSubMenuButtonName, nullptr, nullptr,
                                           false);
        }

        mButtonGroup->tryMove(ButtonGroup::Direction_Down);
    }

    if (rc::isPadTriggerUiLeftByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Left);
        const char* name = mButtonGroup->getSelectedButtonName();

        if (name != nullptr && isSubMenuButton(name)) {
            mLastSubMenuButtonName = mButtonGroup->getSelectedButtonName();
        }
    }

    if (rc::isPadTriggerUiRightByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Right);
        const char* name = mButtonGroup->getSelectedButtonName();

        if (name != nullptr && isSubMenuButton(name)) {
            mLastSubMenuButtonName = mButtonGroup->getSelectedButtonName();
        }
    }

    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        if (mButtonGroup->isSelect(cButtonContinue)) {
            mButtonGroup->decide(cButtonContinue);
        } else if (mButtonGroup->isSelect(cButtonQuit)) {
            mButtonGroup->decide(cButtonQuit);
        } else if (mButtonGroup->isSelect(cButtonController)) {
            mButtonGroup->decide(cButtonController);
        } else if (mButtonGroup->isSelect(cButtonData)) {
            mButtonGroup->decide(cButtonData);
        } else if (mButtonGroup->isSelect(cButtonGuide)) {
            mButtonGroup->decide(cButtonGuide);
        } else if (mButtonGroup->isSelect(cButtonOptions)) {
            mButtonGroup->decide(cButtonOptions);
        } else if (mButtonGroup->isSelect(cButtonLeaveGame)) {
            mButtonGroup->decide(cButtonLeaveGame);
        }
    } else if (rc::isPadTriggerUiCancelByPort(mPort)) {
        mButtonGroup->decide(cButtonContinue);
    }

    if (mButtonGroup->isDecideAny()) {
        mButtonGroup->invalidate();
        mButtonGroup->hideCursor();
        al::setNerve(this, &NrvPauseMenuMapDeciding);
    }
}

/** @brief Waits for the confirmed button's animation and opens what it stands for. */
void PauseMenuMap::exeDeciding() {
    if (!mButtonGroup->isDecideEndAny()) {
        return;
    }

    if (mButtonGroup->isDecideEnd(cButtonContinue)) {
        al::startSe(this, "Return");
        al::setNerve(this, &NrvPauseMenuMapEnd);
        return;
    }

    if (mButtonGroup->isDecideEnd(cButtonQuit)) {
        al::startSe(this, "Return");
        al::setNerve(this, &NrvPauseMenuMapEndGoToTitle);
        return;
    }

    if (mButtonGroup->isDecideEnd(cButtonController)) {
        al::setNerve(this, &NrvPauseMenuMapControllerChange);
        return;
    }

    if (mButtonGroup->isDecideEnd(cButtonData)) {
        al::setNerve(this, &NrvPauseMenuMapSaveDataMenu);
    }

    if (mButtonGroup->isDecideEnd(cButtonGuide)) {
        al::setNerve(this, &NrvPauseMenuMapGuide);
    } else if (mButtonGroup->isDecideEnd(cButtonOptions)) {
        al::setNerve(this, &NrvPauseMenuMapOptions);
    } else if (mButtonGroup->isDecideEnd(cButtonLeaveGame)) {
        mDirector->leaveUser(mUserId);
        al::startSe(this, "Return");
        al::setNerve(this, &NrvPauseMenuMapEnd);
    }
}

/** @brief Plays the closing animation (going to the title skips it). */
void PauseMenuMap::exeEnd() {
    if (al::isFirstStep(this)) {
        getCursorParts(mButtonGroup)->hide();

        if (!al::isNerve(this, &NrvPauseMenuMapEndGoToTitle)) {
            al::startAction(this, "End", nullptr);

            if (mGuideBar != nullptr) {
                mGuideBar->end();
            }
        }
    }
}

/** @brief Shows the options sub-menu until it is closed. */
void PauseMenuMap::exeOptions() {
    if (al::isFirstStep(this)) {
        deactivateButtonGroup(mButtonGroup);
        al::startAction(mMenuFrameParts, "Appear_Options", nullptr);
        al::startAction(this, "EndSubMenu", "Main");
        mOptionsMenu->appear(mPort);
    }

    if (mOptionsMenu->isAlive()) {
        return;
    }

    mButtonGroup->reset();
    al::startAction(this, "AppearSubMenu", "Main");
    al::startAction(mMenuFrameParts, "End_Options", nullptr);
    mButtonGroup->select(cButtonOptions);
    validateButtonGroup(mButtonGroup, isNetworkReady(GameDataHolderAccessor(mGameDataHolder)),
                        mIsEnableControllerChange, mIsEnableLeaveUser);
    al::setNerve(this, &NrvPauseMenuMapWait);
}

/** @brief Shows the save data sub-menu (save, load, delete). */
void PauseMenuMap::exeSaveDataMenu() {
    if (al::isFirstStep(this)) {
        deactivateButtonGroup(mButtonGroup);

        if (al::isNerve(this, &NrvPauseMenuMapSaveDataMenu)) {
            mSaveDataMenu->appear(mPort);
            al::startSe(mSaveDataMenu, "Appear");
            al::startAction(this, "EndSubMenu", "Main");
        }
    }

    if (mSaveDataMenu->isDecideEnd()) {
        mSaveDataMenuDecision = mSaveDataMenu->getDecidedButtonName();

        if (al::isEqualString(mSaveDataMenuDecision, "Save")) {
            mSaveDataLayout->appearSave(mPort, mWorldId);
        } else if (al::isEqualString(mSaveDataMenuDecision, "Load")) {
            mSaveDataLayout->appearLoad(mPort);
        } else {
            mSaveDataLayout->appearDelete(mPort, true);
        }

        al::startAction(mSaveDataMenu, "End", "Main");
        al::setNerve(this, &NrvPauseMenuMapDataManagement);
        return;
    }

    if (mSaveDataMenu->isDecided()) {
        al::startSe(mSaveDataLayout, "Appear");
    }

    if (mSaveDataMenu->isEnding()) {
        al::startSe(mSaveDataMenu, "Back");
        mSaveDataMenu->end();
        mButtonGroup->reset();
        mButtonGroup->select(cButtonData);
        al::startAction(this, "AppearSubMenu", "Main");
        validateButtonGroup(mButtonGroup, isNetworkReady(GameDataHolderAccessor(mGameDataHolder)),
                            mIsEnableControllerChange, mIsEnableLeaveUser);
        al::setNerve(this, &NrvPauseMenuMapWait);
    }
}

/** @brief Shows the save file layout until it is closed or a file has to be loaded. */
void PauseMenuMap::exeDataManagement() {
    if (al::isFirstStep(this)) {
        al::startAction(mMenuFrameParts, "Appear_SaveFile", nullptr);
    }

    if (mSaveDataLayout->isLoading()) {
        al::setNerve(this, &NrvPauseMenuMapWaitLoad);
        return;
    }

    if (mSaveDataLayout->isAlive()) {
        return;
    }

    mSaveDataMenu->reset(mSaveDataMenuDecision);
    al::startAction(mSaveDataMenu, "Appear", "Main");
    al::startAction(mMenuFrameParts, "End_SaveFile", nullptr);
    al::setNerve(this, &NrvPauseMenuMapSaveDataMenuBack);
}

/** @brief Waits until the chosen save file is ready to be loaded. */
void PauseMenuMap::exeWaitLoad() {
    if (mSaveDataLayout->isNeedLoad()) {
        al::setNerve(this, &NrvPauseMenuMapEndLoad);
    }
}

/** @brief Shows the control guide until it is closed. */
void PauseMenuMap::exeGuide() {
    if (al::isFirstStep(this)) {
        deactivateButtonGroup(mButtonGroup);
        mControlGuide->appearWithPort(mPort);

        if (mGuideBar != nullptr) {
            mGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_ControlGuide, mPort, true);
        }
    }

    if (!mControlGuide->isEnding()) {
        return;
    }

    if (mGuideBar != nullptr) {
        mGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_PauseMenuMap, mPort, true);
    }

    mButtonGroup->reset();
    mButtonGroup->select(cButtonGuide);
    validateButtonGroup(mButtonGroup, isNetworkReady(GameDataHolderAccessor(mGameDataHolder)),
                        mIsEnableControllerChange, mIsEnableLeaveUser);
    al::startAction(this, "AppearSubMenu", nullptr);
    al::setNerve(this, &NrvPauseMenuMapWait);
}

/** @brief Asks for confirmation of a controller change and opens the controller applet. */
void PauseMenuMap::exeControllerChange() {
    if (al::isFirstStep(this)) {
        mWindowConfirm->appearWithSystemMessage(cConfirmFileName, "ChangeControls", mPort,
                                                cConfirmButtonName);
        mScreenCaptureExecutor->requestCapture(true, 2, true);
    }

    if (al::isStep(this, 2) && mGuideBar != nullptr) {
        mGuideBar->hide();
    }

    if (mWindowConfirm->isDecideLeft() && mButtonGroup->isDecideAny()) {
        mButtonGroup->reset();
        mButtonGroup->select(cButtonController);
    }

    if (mWindowConfirm->isDecideLeftEnd()) {
        validateButtonGroup(mButtonGroup, isNetworkReady(GameDataHolderAccessor(mGameDataHolder)),
                            mIsEnableControllerChange, mIsEnableLeaveUser);

        if (mGuideBar != nullptr) {
            mGuideBar->show();
        }

        mScreenCaptureExecutor->offDraw(2);
        mScreenCaptureExecutor->onDraw(1, true);
        al::setNerve(this, &NrvPauseMenuMapWait);
        return;
    }

    if (mWindowConfirm->isDecideRight()) {
        mScreenCaptureExecutor->offDraw(2);
        mScreenCaptureExecutor->onDraw(1, true);
        mButtonGroup->reset();

        if (mGuideBar != nullptr) {
            mGuideBar->show();
        }

        al::startFreezeAction(
            mButtonGroup->getButton(cButtonController), "Select",
            al::getActionFrameMax(mButtonGroup->getSelectedButton(), "Select", "Main"), nullptr);
    }

    if (mWindowConfirm->isDecideRightEnd() && !mWindowConfirm->isAlive()) {
        rc::forceControllerApplet();
        mButtonGroup->reset();
        mButtonGroup->select(cButtonController);
        validateButtonGroup(mButtonGroup, isNetworkReady(GameDataHolderAccessor(mGameDataHolder)),
                            mIsEnableControllerChange, mIsEnableLeaveUser);
        al::setNerve(this, &NrvPauseMenuMapWait);
    }
}

/**
 * @brief Checks whether the menu was closed with the continue button.
 * @return True when continue was confirmed.
 */
bool PauseMenuMap::isDecideBack() const {
    return mButtonGroup->isDecide(cButtonContinue);
}

/**
 * @brief Checks whether going back to the title was chosen.
 * @return True when quit was confirmed.
 */
bool PauseMenuMap::isDecideGoToTitle() const {
    return mButtonGroup->isDecide(cButtonQuit);
}

/**
 * @brief Checks whether the user leaves the multiplayer session.
 * @return True when leave game was confirmed.
 */
bool PauseMenuMap::isDecideLeaveGame() const {
    return mButtonGroup->isDecide(cButtonLeaveGame);
}

/**
 * @brief Checks whether a save file has to be loaded.
 * @return True after a save file was chosen to be loaded.
 */
bool PauseMenuMap::isDecideLoad() const {
    return al::isNerve(this, &NrvPauseMenuMapEndLoad);
}

/**
 * @brief Checks whether the menu is idle and waits for input.
 * @return True while the main buttons are waiting.
 */
bool PauseMenuMap::isWait() const {
    return al::isNerve(this, &NrvPauseMenuMapWait) && getCursorParts(mButtonGroup)->isWait();
}

/**
 * @brief Checks whether the closing animation finished.
 * @return True once the menu has closed.
 */
bool PauseMenuMap::isEnd() const {
    return al::isNerve(this, &NrvPauseMenuMapEnd) && al::isGreaterStep(this, 2) &&
           al::isActionEnd(this, nullptr);
}

/**
 * @brief Checks whether the menu finished closing to go back to the title.
 * @return True a moment after quit was confirmed.
 */
bool PauseMenuMap::isEndGoToTitle() const {
    return al::isNerve(this, &NrvPauseMenuMapEndGoToTitle) && al::isGreaterStep(this, 2);
}

/**
 * @brief Checks whether the controller change confirmation is open.
 * @return True while the controller change is handled.
 */
bool PauseMenuMap::isControllerChange() const {
    return al::isNerve(this, &NrvPauseMenuMapControllerChange);
}

/**
 * @brief Sets the guide bar shown below the menu and its save data layout.
 * @param pGuideBar Guide bar to use.
 */
void PauseMenuMap::setControlGuideBar(RCSControlGuideBar* pGuideBar) {
    mGuideBar = pGuideBar;
    mSaveDataLayout->setGuideBar(pGuideBar);
}

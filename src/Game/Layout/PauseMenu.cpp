#include "Layout/PauseMenu.hpp"

#include <prim/seadSafeString.h>
#include "Layout/ButtonCursorParts.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/ControlGuide.hpp"
#include "Layout/CursorTarget.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/RCS_SaveDataLayout.hpp"
#include "Layout/SimpleMenuLayout.hpp"
#include "Layout/Switch/OptionsMenu.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Message/MessageSystem.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/IslandDataList.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/PlayerUtil.hpp"

/// Declares a nerve of the pause menu whose state function may differ from the nerve's name.
#define PAUSE_MENU_NERVE_DECL(Action, Func)                                                        \
    class PauseMenuNrv##Action : public al::Nerve {                                                \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<PauseMenu>())->exe##Func();                                        \
        }                                                                                          \
    };

namespace {
PAUSE_MENU_NERVE_DECL(End, End)
PAUSE_MENU_NERVE_DECL(Appear, Appear)
PAUSE_MENU_NERVE_DECL(KoopaJrOptions, KoopaJrOptions)
PAUSE_MENU_NERVE_DECL(Deciding, Deciding)
PAUSE_MENU_NERVE_DECL(WaitLoad, WaitLoad)
PAUSE_MENU_NERVE_DECL(EndLoad, End)
PAUSE_MENU_NERVE_DECL(EndQuit, End)
PAUSE_MENU_NERVE_DECL(Wait, Wait)
PAUSE_MENU_NERVE_DECL(WaitControllerChange, Wait)
PAUSE_MENU_NERVE_DECL(ConfirmRestart, Confirm)
PAUSE_MENU_NERVE_DECL(ConfirmReturnToMap, Confirm)
PAUSE_MENU_NERVE_DECL(ConfirmControllerChange, Confirm)
PAUSE_MENU_NERVE_DECL(SaveDataMenu, SaveDataMenu)
PAUSE_MENU_NERVE_DECL(Guide, Guide)
PAUSE_MENU_NERVE_DECL(Options, Options)
PAUSE_MENU_NERVE_DECL(FadeAssistMode, FadeAssistMode)
PAUSE_MENU_NERVE_DECL(HandleAssistModeOff, HandleAssistMode)
PAUSE_MENU_NERVE_DECL(HandleAssistModeOn, HandleAssistMode)
PAUSE_MENU_NERVE_DECL(EndAssistMode, EndAssistMode)
PAUSE_MENU_NERVE_DECL(DataManagement, DataManagement)
PAUSE_MENU_NERVE_DECL(SaveDataMenuBack, SaveDataMenu)
PAUSE_MENU_NERVE_DECL(ConfirmGameChange, Confirm)
PAUSE_MENU_NERVE_DECL(ConfirmReturnToTitle, Confirm)
PAUSE_MENU_NERVE_DECL(EndGameChange, End)

NERVES_MAKE_NOSTRUCT(PauseMenu, End, Appear, KoopaJrOptions, Deciding, WaitLoad, EndLoad, EndQuit,
                     Wait, WaitControllerChange, ConfirmRestart, ConfirmReturnToMap,
                     ConfirmControllerChange, SaveDataMenu, Guide, Options, FadeAssistMode,
                     HandleAssistModeOff, HandleAssistModeOn, EndAssistMode, DataManagement,
                     SaveDataMenuBack, ConfirmGameChange, ConfirmReturnToTitle, EndGameChange)

/// Button names of the main pause menu.
constexpr const char* cButtonContinue = "つづける";
constexpr const char* cButtonRestart = "Restart";
constexpr const char* cButtonReturnToMap = "マップへもどる";
constexpr const char* cButtonController = "Controller";
constexpr const char* cButtonData = "Data";
constexpr const char* cButtonGuide = "Guide";
constexpr const char* cButtonOptions = "Options";
constexpr const char* cButtonQuit = "Quit";
constexpr const char* cButtonAssistMode = "AssistMode";

/// Message file and button label of the pause menu's confirmation window.
constexpr const char* cConfirmFileName = "WindowConfirmPauseMenu";
constexpr const char* cConfirmButtonName = "Rボタン";

/// Cutscene that unlocks the two-player assist mode in Bowser's Fury.
constexpr s32 cCutsceneIdAssistMode = 1;

/// Island number of the open water between the islands.
constexpr s32 cIslandNoOpenWater = 16;

/// Frames of the wipe used when switching the assist mode.
constexpr s32 cAssistModeWipeFrames = 15;

/// Priority of the guide window explaining the two-player assist mode.
constexpr GuideMessagePriority cAssistModeGuidePriority = static_cast<GuideMessagePriority>(6);

/// Assist mode type enabled by the Bowser Jr. demo of the options.
constexpr u8 cAssistModeTypeKoopaJr = 1;

/**
 * @brief Access the cursor layout of a button group.
 * @param pButtonGroup Button group owning the cursor.
 * @return The button cursor parts.
 */
inline ButtonCursorParts* getCursorParts(const ButtonGroup* pButtonGroup) {
    return static_cast<ButtonCursorParts*>(pButtonGroup->getCursor());
}

/**
 * @brief Hide the cursor of a button group and stop it from taking input.
 * @param pButtonGroup Button group to deactivate.
 */
inline void deactivateButtonGroup(ButtonGroup* pButtonGroup) {
    pButtonGroup->hideCursor();
    pButtonGroup->invalidate();
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
 * @brief Check whether the stage is a small room that can't be left through the pause menu.
 * @param accessor Game data.
 * @param courseId Course to check.
 * @return True for the casino, the Toad houses and the fairy house.
 */
inline bool isStageRoom(GameDataHolderAccessor accessor, s32 courseId) {
    return GameDataFunction::isStageCasinoRoom(accessor, courseId) ||
           GameDataFunction::isStageKinopioHouse(accessor, courseId) ||
           GameDataFunction::isStageKinopioHouseHide(accessor, courseId) ||
           GameDataFunction::isStageFairyHouse(accessor, courseId);
}

/**
 * @brief Kill the guide bar layout.
 * @param pGuideBar Guide bar to kill.
 * @note The guide bar is a layout actor; its header doesn't declare the base class yet.
 */
inline void killGuideBar(RCSControlGuideBar* pGuideBar) {
    reinterpret_cast<al::LayoutActor*>(pGuideBar)->kill();
}
}  // namespace

/**
 * @brief Creates the Super Mario 3D World pause menu.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data of the running session.
 * @param pPlayerAliveWatcher Watcher deciding whether the stage may be left.
 * @param pCameraDirector Camera director adjusted by the options.
 * @param pPlayerHolder Players of the scene.
 * @param pGamePadSystem Pad system of the scene.
 * @param pScreenCaptureExecutor Screen capture shown behind the confirmation window.
 */
PauseMenu::PauseMenu(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                     const PlayerAliveWatcher* pPlayerAliveWatcher,
                     al::CameraDirector* pCameraDirector, al::PlayerHolder* pPlayerHolder,
                     al::GamePadSystem* pGamePadSystem,
                     al::ScreenCaptureExecutor* pScreenCaptureExecutor)
    : al::LayoutActor("ポーズメニュー"), mGameDataHolder(pGameDataHolder),
      mPlayerAliveWatcher(pPlayerAliveWatcher), mPlayerHolder(pPlayerHolder), mWipe(nullptr),
      mGamePadSystem(pGamePadSystem), mScreenCaptureExecutor(pScreenCaptureExecutor),
      mIsSingleMode(false), mSaveDataMenuDecision(nullptr), mLastSubMenuButtonName(nullptr) {
    al::initLayoutActor(this, rInfo, "PauseMenu", nullptr);
    initNerve(&NrvPauseMenuEnd, 0);
    s32 courseId = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(mGameDataHolder));
    bool isKinopioBrigade = GameDataFunction::isStageKinopioBrigade(
        GameDataHolderAccessor(mGameDataHolder), courseId);

    mMenuFrameParts = new al::LayoutActor("RCS_MenuFrameParts");
    al::initLayoutPartsActor(mMenuFrameParts, this, rInfo, "ParFrame", nullptr);
    const char* cursorSuffix = mIsSingleMode ? "SingleMode" : nullptr;
    mButtonGroup = new ButtonGroup(rInfo, this, "PauseMenu", cursorSuffix, false);
    mOptionsMenu = new OptionsMenu(rInfo, pCameraDirector, isKinopioBrigade);
    mSaveDataMenu = new SimpleMenuLayout("RCS_SaveDataManage", rInfo, 4, nullptr);
    mSaveDataLayout = new RCS_SaveDataLayout(rInfo, pGameDataHolder, mScreenCaptureExecutor,
                                             mGuideBar, true);
    mSaveDataLayout->setPauseMenuLayout(mMenuFrameParts);
    mControlGuide = new ControlGuide(rInfo, false);
    mGuideBar = new RCSControlGuideBar(rInfo);
    mSaveDataLayout->setGuideBar(mGuideBar);
    mWindowConfirm = new WindowConfirm(WindowConfirmType_Double, rInfo, "PauseMenu", false);

    if (mIsSingleMode) {
        al::setPaneSystemMessage(this, "TxtIslandName", "IslandName", "OpenWater");
        sead::WFixedSafeString<2> emptyString(u"");
        al::setPaneString(this, "TxtShineCounter", emptyString.cstr());
    } else {
        if (GameDataFunction::isInvalidCourseId(courseId) ||
            GameDataFunction::isStageNoCourseName(GameDataHolderAccessor(mGameDataHolder),
                                                  courseId)) {
            al::setPaneString(this, "TxtCourseName", u"");
        } else if (getMessageSystem()->getSystemMessageHolder("StageName")->tryGetText(
                       GameDataFunction::findStageName(GameDataHolderAccessor(pGameDataHolder),
                                                       courseId)) != nullptr) {
            rc::setPaneStageNameString(this, this, "TxtCourseName", mGameDataHolder, courseId);
        } else {
            al::setPaneString(this, "TxtCourseName", u"NULL");
        }

        s32 worldId;
        s32 stageId;
        GameDataFunction::calcWorldAndStageId(this, &worldId, &stageId, courseId);
        rc::setPaneWorldStageString(this, this, "TxtWorld", "PauseMenu", "PauseMenu_World",
                                    worldId, stageId, mGameDataHolder, nullptr, false);
    }

    mCourseNameTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCourseName", 2);
}

/**
 * @brief Creates the Bowser's Fury pause menu.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data of the running session.
 * @param pPlayerAliveWatcher Watcher deciding whether the stage may be left.
 * @param pCameraDirector Camera director adjusted by the options.
 * @param pPlayerHolder Players of the scene.
 * @param pGamePadSystem Pad system of the scene.
 * @param pScreenCaptureExecutor Screen capture shown behind the confirmation window.
 * @param pWipe Wipe covering the screen while the assist mode is switched.
 */
PauseMenu::PauseMenu(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                     const PlayerAliveWatcher* pPlayerAliveWatcher,
                     al::CameraDirector_RS* pCameraDirector, al::PlayerHolder* pPlayerHolder,
                     al::GamePadSystem* pGamePadSystem,
                     al::ScreenCaptureExecutor* pScreenCaptureExecutor, al::WipeSimple* pWipe)
    : al::LayoutActor("ポーズメニュー"), mGameDataHolder(pGameDataHolder),
      mPlayerAliveWatcher(pPlayerAliveWatcher), mPlayerHolder(pPlayerHolder), mWipe(pWipe),
      mGamePadSystem(pGamePadSystem), mScreenCaptureExecutor(pScreenCaptureExecutor),
      mIsSingleMode(true) {
    al::initLayoutActor(this, rInfo, "PauseMenu", nullptr);
    initNerve(&NrvPauseMenuEnd, 0);
    s32 courseId = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(mGameDataHolder));
    GameDataFunction::isStageKinopioBrigade(GameDataHolderAccessor(mGameDataHolder), courseId);

    mMenuFrameParts = new al::LayoutActor("RCS_MenuFrameParts");
    al::initLayoutPartsActor(mMenuFrameParts, this, rInfo, "ParFrame", nullptr);
    const char* cursorSuffix = mIsSingleMode ? "SingleMode" : nullptr;
    mButtonGroup = new ButtonGroup(rInfo, this, "PauseMenu", cursorSuffix, false);
    mOptionsMenu = new OptionsMenu(rInfo, pCameraDirector);
    mSaveDataMenu = new SimpleMenuLayout("RCS_SaveDataManage", rInfo, 4, nullptr);
    mSaveDataLayout = new RCS_SaveDataLayout(rInfo, pGameDataHolder, pScreenCaptureExecutor,
                                             mGuideBar, true);
    mControlGuide = new ControlGuide(rInfo, true);
    mGuideBar = new RCSControlGuideBar(rInfo);
    mSaveDataLayout->setGuideBar(mGuideBar);
    mWindowConfirm = new WindowConfirm(WindowConfirmType_Double, rInfo, "PauseMenu", false);

    if (mIsSingleMode) {
        al::setPaneSystemMessage(this, "TxtIslandName", "IslandName", "OpenWater");
    } else {
        if (GameDataFunction::isInvalidCourseId(courseId) ||
            GameDataFunction::isStageNoCourseName(GameDataHolderAccessor(mGameDataHolder),
                                                  courseId)) {
            al::setPaneString(this, "TxtCourseName", u"");
        } else if (getMessageSystem()->getSystemMessageHolder("StageName")->tryGetText(
                       GameDataFunction::findStageName(GameDataHolderAccessor(pGameDataHolder),
                                                       courseId)) != nullptr) {
            rc::setPaneStageNameString(this, this, "TxtCourseName", mGameDataHolder, courseId);
        } else {
            al::setPaneString(this, "TxtCourseName", u"NULL");
        }

        s32 worldId;
        s32 stageId;
        GameDataFunction::calcWorldAndStageId(this, &worldId, &stageId, courseId);
        rc::setPaneWorldStageString(this, this, "TxtWorld", "PauseMenu", "PauseMenu_World",
                                    worldId, stageId, mGameDataHolder, nullptr, false);
    }

    mCourseNameTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtCourseName", 2);
}

/** @brief Keeps the course name text box laid out and the assist mode text up to date. */
void PauseMenu::control() {
    mCourseNameTextInfo.applyFix();

    if (mIsSingleMode && rc::isControllerAssignmentChanged()) {
        CursorTarget* assistButton = mButtonGroup->getButton(cButtonAssistMode);

        if (SingleModeDataFunction::getIs2PAssistMode(this)) {
            al::setPaneSystemMessage(assistButton, "TxtButton", "PauseMenu", "2PPlayStop");
        } else {
            al::setPaneSystemMessage(assistButton, "TxtButton", "PauseMenu", "2PPlayStart");
        }
    }
}

/**
 * @brief Opens the pause menu.
 * @param port Controller port that paused the game.
 */
void PauseMenu::appear(s32 port) {
    if (mIsSingleMode) {
        al::startAction(this, "SetPauseMenuSingle", "PauseMenuType");
        al::startAction(mMenuFrameParts, "Koopa", nullptr);
        al::startFreezeAction(this, "Appear", 0.0f, nullptr);
        const char* assistLabel =
            SingleModeDataFunction::getIs2PAssistMode(this) ? "2PPlayStop" : "2PPlayStart";
        al::setPaneSystemMessage(mButtonGroup->getButton(cButtonAssistMode), "TxtButton",
                                 "PauseMenu", assistLabel);
    }

    al::LayoutActor::appear();
    mIsForceExit = false;
    mPort = rc::tryGetConnectCheckedPort(port);
    mButtonGroup->setPort(mPort);
    rc::getActiveInputPortList(GameDataHolderAccessor(mGameDataHolder));

    if (!mIsSingleMode) {
        mButtonGroup->getButton(cButtonData)->disable();
        s32 userId =
            rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(mGameDataHolder), mPort);
        rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder), userId);
    }

    if (mGuideBar != nullptr) {
        mGuideBar->appearWithMessage(RCSControlGuideBar::GuideBarMsgType_PauseMenu, mPort);
    }

    mLastSubMenuButtonName = cButtonController;
    al::setNerve(this, &NrvPauseMenuAppear);
}

/** @brief Opens the options directly on the Bowser Jr. assist demo. */
void PauseMenu::appearKoopaJrOptions() {
    al::LayoutActor::appear();
    al::startFreezeActionEnd(this, "EndSubMenu", "Main");
    SingleModeDataFunction::setAssistModeType(GameDataHolderAccessor(this),
                                              cAssistModeTypeKoopaJr);
    al::setNerve(this, &NrvPauseMenuKoopaJrOptions);
}

/** @brief Closes whatever sub-menu is open, or the whole menu, without animations. */
void PauseMenu::forceExit() {
    mIsForceExit = true;

    if (al::isNerve(this, &NrvPauseMenuAppear) || al::isNerve(this, &NrvPauseMenuDeciding) ||
        al::isNerve(this, &NrvPauseMenuEnd) || al::isNerve(this, &NrvPauseMenuWaitLoad) ||
        al::isNerve(this, &NrvPauseMenuEndLoad) || al::isNerve(this, &NrvPauseMenuEndQuit)) {
        return;
    }

    if (mGuideBar != nullptr) {
        mGuideBar->end();
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
        mWindowConfirm->setDefaultSelectLeft(false);

        if (!mWindowConfirm->isEnding()) {
            mWindowConfirm->forceExit();
        }

        mScreenCaptureExecutor->offDraw(2);
        mScreenCaptureExecutor->onDraw(1, true);
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

    al::setNerve(this, &NrvPauseMenuEnd);
}

/** @brief Closes the menu. */
void PauseMenu::end() {
    al::setNerve(this, &NrvPauseMenuEnd);
}

/** @brief Confirms the continue button as if it had been pressed. */
void PauseMenu::decideBack() {
    mButtonGroup->decide(cButtonContinue);
}

/** @brief Plays the opening animation and sets up the buttons for the current stage. */
void PauseMenu::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startSe(this, "Appear");
        al::startFreezeActionEnd(mMenuFrameParts, "End_Options", nullptr);
        mButtonGroup->reset();
        mButtonGroup->validate();
        getCursorParts(mButtonGroup)->hide();

        if (mIsSingleMode) {
            if (mGuideBar != nullptr) {
                mGuideBar->setCharacterSingleMode(mPort);
            }

            bool isSeenAssistMode =
                SingleModeDataFunction::hasSeenCutscene(this, cCutsceneIdAssistMode);
            CursorTarget* assistButton = mButtonGroup->getButton(cButtonAssistMode);

            if (isSeenAssistMode) {
                assistButton->enable();
                mButtonGroup->resetDestination(cButtonContinue, nullptr, cButtonAssistMode,
                                               nullptr, nullptr, false);
                mButtonGroup->resetDestination(cButtonQuit, cButtonAssistMode, nullptr, nullptr,
                                               nullptr, false);
            } else {
                assistButton->disable();
            }
        } else {
            mButtonGroup->getButton(cButtonData)->disable();
            s32 userId =
                rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(mGameDataHolder), mPort);
            s32 characterType =
                rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder), userId);
            s32 courseId =
                GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(mGameDataHolder));
            bool isKinopioBrigade = GameDataFunction::isStageKinopioBrigade(
                GameDataHolderAccessor(mGameDataHolder), courseId);

            if (isKinopioBrigade) {
                s32 brigadeNo = mPort - 1;

                if (brigadeNo == 0) {
                    al::startAction(mMenuFrameParts, "CaptainKinopio", nullptr);
                } else {
                    al::startAction(mMenuFrameParts,
                                    al::StringTmp<32>("KinopioBrigade%d", brigadeNo).cstr(),
                                    nullptr);
                }

                if (mGuideBar != nullptr) {
                    mGuideBar->setCharacterKinopioBrigade(mPort);
                }
            } else {
                const char* characterName = rc::getPlayerCharacterName(characterType);
                al::startAction(mMenuFrameParts, characterName, nullptr);

                if (mGuideBar != nullptr) {
                    mGuideBar->setCharacter(characterName);
                }
            }

            mIsExitDisabled = false;
            bool isEnableExit =
                mPlayerAliveWatcher->isEnableExitStage(characterType) || isKinopioBrigade;

            if (!isEnableExit ||
                GameDataFunction::isStageGoldenExpress(GameDataHolderAccessor(mGameDataHolder),
                                                       courseId) ||
                isStageRoom(GameDataHolderAccessor(mGameDataHolder), courseId)) {
                mButtonGroup->getButton(cButtonReturnToMap)->disable();

                if (!mIsSingleMode) {
                    mButtonGroup->getButton(cButtonRestart)->disable();
                }

                mIsExitDisabled = true;
            }
        }

        if (mIsExitDisabled) {
            mButtonGroup->resetDestination(cButtonContinue, nullptr, mLastSubMenuButtonName,
                                           nullptr, nullptr, false);
        } else if (!mIsSingleMode) {
            mButtonGroup->resetDestination(cButtonContinue, nullptr, cButtonRestart, nullptr,
                                           nullptr, false);
        }
    }

    if (al::isActionEnd(this, nullptr)) {
        mButtonGroup->select(cButtonContinue);
        PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
        al::setNerve(this, &NrvPauseMenuWait);
    }
}

/**
 * @brief Reactivates the pause menu buttons, disabling the ones that can't be used right now.
 * @param pButtonGroup Buttons of the pause menu.
 * @param isExitDisabled True when the stage can't be left.
 * @param isSingleMode True in Bowser's Fury.
 */
void PauseMenuFunction::validateButtonGroup(ButtonGroup* pButtonGroup, bool isExitDisabled,
                                            bool isSingleMode) {
    getCursorParts(pButtonGroup)->reset();
    pButtonGroup->showCursor();
    pButtonGroup->validate();

    if (isExitDisabled) {
        pButtonGroup->getButton(cButtonReturnToMap)->disable();

        if (!GameDataFunction::isSingleMode(pButtonGroup->getCursor())) {
            pButtonGroup->getButton(cButtonRestart)->disable();
        }
    }

    if (isSingleMode) {
        CursorTarget* assistButton = pButtonGroup->getButton(cButtonAssistMode);

        if (SingleModeDataFunction::hasSeenCutscene(assistButton, cCutsceneIdAssistMode)) {
            assistButton->enable();
            pButtonGroup->resetDestination(cButtonContinue, nullptr, cButtonAssistMode, nullptr,
                                           nullptr, false);
            pButtonGroup->resetDestination(cButtonQuit, cButtonAssistMode, nullptr, nullptr,
                                           nullptr, false);
        } else {
            assistButton->disable();
        }
    } else {
        pButtonGroup->getButton(cButtonData)->disable();
    }
}

/** @brief Moves the cursor and waits for a button to be confirmed. */
void PauseMenu::exeWait() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvPauseMenuWaitControllerChange) &&
        mIsSingleMode && SingleModeDataFunction::getIs2PAssistMode(this)) {
        rc::appearGuideGameWindowWithPriority(this, nullptr, nullptr, cAssistModeGuidePriority,
                                              -1, 0.0f);
    }

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

        mButtonGroup->tryMove(0);
    }

    if (rc::isPadTriggerUiDownByPort(mPort)) {
        if (mIsSingleMode) {
            if (mButtonGroup->isSelect(cButtonQuit)) {
                mButtonGroup->resetDestination(cButtonQuit, nullptr, mLastSubMenuButtonName,
                                               nullptr, nullptr, false);
            }
        } else if (mIsExitDisabled) {
            if (mButtonGroup->isSelect(cButtonContinue)) {
                mButtonGroup->resetDestination(cButtonContinue, nullptr, mLastSubMenuButtonName,
                                               nullptr, nullptr, false);
            }
        } else if (mButtonGroup->isSelect(cButtonReturnToMap)) {
            mButtonGroup->resetDestination(cButtonReturnToMap, nullptr, mLastSubMenuButtonName,
                                           nullptr, nullptr, false);
        }

        mButtonGroup->tryMove(1);
    }

    if (rc::isPadTriggerUiLeftByPort(mPort)) {
        mButtonGroup->tryMove(2);
        const char* name = mButtonGroup->getSelectedButtonName();

        if (name != nullptr && isSubMenuButton(name)) {
            mLastSubMenuButtonName = mButtonGroup->getSelectedButtonName();
        }
    }

    if (rc::isPadTriggerUiRightByPort(mPort)) {
        mButtonGroup->tryMove(3);
        const char* name = mButtonGroup->getSelectedButtonName();

        if (name != nullptr && isSubMenuButton(name)) {
            mLastSubMenuButtonName = mButtonGroup->getSelectedButtonName();
        }
    }

    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        if (mButtonGroup->isSelect(cButtonContinue)) {
            mButtonGroup->decide(cButtonContinue);
        } else if (mButtonGroup->isSelect(cButtonRestart)) {
            mButtonGroup->decide(cButtonRestart);
        } else if (mButtonGroup->isSelect(cButtonReturnToMap)) {
            mButtonGroup->decide(cButtonReturnToMap);
        } else if (mButtonGroup->isSelect(cButtonController)) {
            mButtonGroup->decide(cButtonController);
        } else if (mButtonGroup->isSelect(cButtonData)) {
            mButtonGroup->decide(cButtonData);
        } else if (mButtonGroup->isSelect(cButtonGuide)) {
            mButtonGroup->decide(cButtonGuide);
        } else if (mButtonGroup->isSelect(cButtonOptions)) {
            mButtonGroup->decide(cButtonOptions);
        } else if (mButtonGroup->isSelect(cButtonQuit)) {
            mButtonGroup->decide(cButtonQuit);
        } else if (mIsSingleMode && mButtonGroup->isSelect(cButtonAssistMode)) {
            mButtonGroup->decide(cButtonAssistMode);
        }
    } else if (rc::isPadTriggerUiCancelByPort(mPort)) {
        mButtonGroup->decide(cButtonContinue);
    }

    if (mButtonGroup->isDecideAny()) {
        al::setNerve(this, &NrvPauseMenuDeciding);
        mButtonGroup->hideCursor();
        mButtonGroup->invalidate();
    }
}

/** @brief Waits for the confirmed button's animation and opens what it stands for. */
void PauseMenu::exeDeciding() {
    if (!mButtonGroup->isDecideEndAny()) {
        return;
    }

    if (mButtonGroup->isDecideEnd(cButtonContinue)) {
        al::startSe(this, "Return");
        al::setNerve(this, &NrvPauseMenuEnd);
        return;
    }

    if (mIsSingleMode) {
        if (mButtonGroup->isDecideEnd(cButtonQuit)) {
            al::startSe(this, "Return");
            al::setNerve(this, &NrvPauseMenuEndQuit);
            return;
        }
    } else if (mButtonGroup->isDecideEnd(cButtonRestart)) {
        al::setNerve(this, &NrvPauseMenuConfirmRestart);
        return;
    } else if (mButtonGroup->isDecideEnd(cButtonReturnToMap)) {
        al::setNerve(this, &NrvPauseMenuConfirmReturnToMap);
        return;
    }

    if (mButtonGroup->isDecideEnd(cButtonController)) {
        al::setNerve(this, &NrvPauseMenuConfirmControllerChange);
    } else if (mButtonGroup->isDecideEnd(cButtonData)) {
        al::setNerve(this, &NrvPauseMenuSaveDataMenu);
    } else if (mButtonGroup->isDecideEnd(cButtonGuide)) {
        al::setNerve(this, &NrvPauseMenuGuide);
    } else if (mButtonGroup->isDecideEnd(cButtonOptions)) {
        al::setNerve(this, &NrvPauseMenuOptions);
    } else if (mIsSingleMode && mButtonGroup->isDecideEnd(cButtonAssistMode)) {
        al::startSe(this, "PgAssist");
        al::setNerve(this, &NrvPauseMenuFadeAssistMode);
    } else {
        al::setNerve(this, &NrvPauseMenuEnd);
    }
}

/** @brief Plays the closing animation and hides the menu (quitting skips the animation). */
void PauseMenu::exeEnd() {
    if (al::isFirstStep(this)) {
        if (!al::isNerve(this, &NrvPauseMenuEndQuit)) {
            al::startAction(this, "End", nullptr);

            if (mGuideBar != nullptr) {
                mGuideBar->end();
            }
        }

        getCursorParts(mButtonGroup)->hide();
    }

    if (!al::isNerve(this, &NrvPauseMenuEndQuit) && al::isActionEnd(this, nullptr)) {
        kill();
    }
}

/** @brief Fades the screen out and toggles the two-player assist mode. */
void PauseMenu::exeFadeAssistMode() {
    if (al::isFirstStep(this)) {
        mWipe->startClose(cAssistModeWipeFrames);
    }

    if (!mWipe->isCloseEnd()) {
        return;
    }

    if (SingleModeDataFunction::getIs2PAssistMode(this)) {
        SingleModeDataFunction::setIs2PAssistMode(GameDataHolderAccessor(this), false);
        rc::set2PAssistMode(false, true);
        rc::setAppletCancel2PAssistMode(false);
        al::setNerve(this, &NrvPauseMenuHandleAssistModeOff);
    } else {
        SingleModeDataFunction::setIs2PAssistMode(GameDataHolderAccessor(this), true);
        rc::set2PAssistMode(true, true);
        rc::setAppletCancel2PAssistMode(false);
        al::setNerve(this, &NrvPauseMenuHandleAssistModeOn);
    }
}

/** @brief Waits for the controller setup of the assist mode switch, reopening the menu if it
 *         was cancelled. */
void PauseMenu::exeHandleAssistMode() {
    if (al::isLessStep(this, 5)) {
        return;
    }

    if (al::isNerve(this, &NrvPauseMenuHandleAssistModeOff)) {
        if (!SingleModeDataFunction::getIs2PAssistMode(this)) {
            mOptionsMenu->getCameraDirectorRS()->setActiveInputNum(1);
            al::setNerve(this, &NrvPauseMenuEndAssistMode);
            return;
        }
    } else if (al::isNerve(this, &NrvPauseMenuHandleAssistModeOn) &&
               SingleModeDataFunction::getIs2PAssistMode(this)) {
        al::setNerve(this, &NrvPauseMenuEndAssistMode);
        mOptionsMenu->getCameraDirectorRS()->setActiveInputNum(2);
        rc::appearGuideGameWindowWithPriority(this, nullptr, nullptr, cAssistModeGuidePriority,
                                              -1, 0.0f);
        return;
    }

    mWipe->startOpen(cAssistModeWipeFrames);
    mButtonGroup->reset();
    PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
    mButtonGroup->showCursor();
    mButtonGroup->select(cButtonAssistMode);
    al::setNerve(this, &NrvPauseMenuWait);
}

/** @brief Shows the options sub-menu until it is closed. */
void PauseMenu::exeOptions() {
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
    PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
    al::setNerve(this, &NrvPauseMenuWait);
}

/** @brief Plays the Bowser Jr. assist demo of the options, then closes the menu. */
void PauseMenu::exeKoopaJrOptions() {
    if (al::isFirstStep(this)) {
        al::startAction(mMenuFrameParts, "Appear_KoopaJrDemo", nullptr);
        al::hidePaneNoRecursive(this, "Buttons");
        al::hidePaneNoRecursive(this, "Info");
        mPort = al::getMainControllerPort();

        if (mGuideBar != nullptr) {
            mGuideBar->appearWithMessage(RCSControlGuideBar::GuideBarMsgType_KoopaJrDemo, mPort);
        }

        mOptionsMenu->appearKoopaJrDemo(mPort);
    }

    if (!mOptionsMenu->isKoopaJrDemoEnd()) {
        return;
    }

    if (al::isActionPlaying(this, "End", "Main")) {
        if (al::isActionEnd(this, "Main")) {
            al::showPaneNoRecursive(this, "Buttons");
            al::showPaneNoRecursive(this, "Info");
            kill();
        }

        return;
    }

    if (mGuideBar != nullptr) {
        mGuideBar->end();
    }

    al::startAction(this, "End", nullptr);
}

/** @brief Shows the save data sub-menu (save, load, delete). */
void PauseMenu::exeSaveDataMenu() {
    if (al::isFirstStep(this)) {
        deactivateButtonGroup(mButtonGroup);
        mSaveDataMenuDecision = nullptr;

        if (al::isNerve(this, &NrvPauseMenuSaveDataMenu)) {
            mSaveDataMenu->appear(mPort);
            al::startSe(mSaveDataMenu, "Appear");
            al::startAction(this, "EndSubMenu", "Main");
        }
    }

    if (mSaveDataMenu->isDecideEnd()) {
        mSaveDataMenuDecision = mSaveDataMenu->getDecidedButtonName();

        if (al::isEqualString(mSaveDataMenuDecision, "Save")) {
            mSaveDataLayout->appearSave(mPort, -1);
        } else if (al::isEqualString(mSaveDataMenuDecision, "Load")) {
            mSaveDataLayout->appearLoad(mPort);
        } else {
            mSaveDataLayout->appearDelete(mPort, true);
        }

        al::startAction(mSaveDataMenu, "End", "Main");
        al::setNerve(this, &NrvPauseMenuDataManagement);
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
        PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
        al::setNerve(this, &NrvPauseMenuWait);
    }
}

/** @brief Shows the save file layout until it is closed or a file has to be loaded. */
void PauseMenu::exeDataManagement() {
    if (al::isFirstStep(this)) {
        al::startAction(mMenuFrameParts, "Appear_SaveFile", nullptr);
    }

    if (mSaveDataLayout->isLoading()) {
        al::setNerve(this, &NrvPauseMenuWaitLoad);
        return;
    }

    if (mSaveDataLayout->isAlive()) {
        return;
    }

    mSaveDataMenu->reset(mSaveDataMenuDecision);
    al::startAction(mSaveDataMenu, "Appear", "Main");
    al::startAction(mMenuFrameParts, "End_SaveFile", nullptr);
    al::setNerve(this, &NrvPauseMenuSaveDataMenuBack);
}

/** @brief Waits until the chosen save file is ready to be loaded. */
void PauseMenu::exeWaitLoad() {
    if (mSaveDataLayout->isNeedLoad()) {
        al::setNerve(this, &NrvPauseMenuEndLoad);
    }
}

/** @brief Shows the control guide until it is closed. */
void PauseMenu::exeGuide() {
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
        mGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_PauseMenu, mPort, true);
    }

    mButtonGroup->reset();
    mButtonGroup->select(cButtonGuide);
    PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
    al::startAction(this, "AppearSubMenu", nullptr);
    al::setNerve(this, &NrvPauseMenuWait);
}

/** @brief Asks for confirmation of a restart, a return to the map or a controller change. */
void PauseMenu::exeConfirm() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPauseMenuConfirmGameChange)) {
            mWindowConfirm->appearWithSystemMessage(cConfirmFileName, "ChangeGame", mPort,
                                                    cConfirmButtonName);
        } else if (al::isNerve(this, &NrvPauseMenuConfirmRestart)) {
            mWindowConfirm->appearWithSystemMessage(cConfirmFileName, "RestartStage", mPort,
                                                    cConfirmButtonName);
        } else if (al::isNerve(this, &NrvPauseMenuConfirmControllerChange)) {
            mWindowConfirm->appearWithSystemMessage(cConfirmFileName, "ChangeControls", mPort,
                                                    cConfirmButtonName);
        } else if (al::isNerve(this, &NrvPauseMenuConfirmReturnToTitle)) {
            mWindowConfirm->appearWithSystemMessage(cConfirmFileName, "ReturnToTitle", mPort,
                                                    cConfirmButtonName);
        } else {
            mWindowConfirm->appearWithSystemMessage(cConfirmFileName, "ReturnToMap", mPort,
                                                    cConfirmButtonName);
        }

        mWindowConfirm->setDefaultSelectLeft(true);
        deactivateButtonGroup(mButtonGroup);
        mScreenCaptureExecutor->requestCapture(true, 2, true);
    }

    if (al::isStep(this, 2) && mGuideBar != nullptr) {
        mGuideBar->hide();
    }

    if (al::isStep(this, 10) && al::isNerve(this, &NrvPauseMenuConfirmControllerChange)) {
        mButtonGroup->reset();
        mButtonGroup->select(cButtonController);
    }

    if (mWindowConfirm->isDecideLeft()) {
        if (mButtonGroup->isDecideAny()) {
            mWindowConfirm->setDefaultSelectLeft(false);
            mButtonGroup->reset();

            if (al::isNerve(this, &NrvPauseMenuConfirmGameChange)) {
                mButtonGroup->select("GameChange");
            } else if (al::isNerve(this, &NrvPauseMenuConfirmRestart)) {
                mButtonGroup->select(cButtonRestart);
            } else if (al::isNerve(this, &NrvPauseMenuConfirmControllerChange)) {
                mButtonGroup->select(cButtonController);
            } else if (al::isNerve(this, &NrvPauseMenuConfirmReturnToTitle)) {
                mButtonGroup->select(cButtonQuit);
            } else {
                mButtonGroup->select(cButtonReturnToMap);
            }
        }

        PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
    }

    if (mWindowConfirm->isDecideLeftEnd()) {
        CursorTarget* selectedButton = mButtonGroup->getSelectedButton();
        al::startFreezeAction(selectedButton, "Select",
                              al::getActionFrameMax(selectedButton, "Select", "Main"), nullptr);
        mScreenCaptureExecutor->offDraw(2);
        mScreenCaptureExecutor->onDraw(1, true);

        if (mGuideBar != nullptr) {
            mGuideBar->show();
        }

        al::setNerve(this, &NrvPauseMenuWait);
    }

    if (mWindowConfirm->isDecideRight() &&
        al::isNerve(this, &NrvPauseMenuConfirmControllerChange)) {
        mScreenCaptureExecutor->offDraw(2);
        mScreenCaptureExecutor->onDraw(1, true);

        if (mGuideBar != nullptr) {
            mGuideBar->show();
        }

        PauseMenuFunction::validateButtonGroup(mButtonGroup, mIsExitDisabled, mIsSingleMode);
    }

    if (!mWindowConfirm->isDecideRightEnd()) {
        return;
    }

    if (al::isNerve(this, &NrvPauseMenuConfirmReturnToTitle)) {
        al::setNerve(this, &NrvPauseMenuEndQuit);
    } else if (al::isNerve(this, &NrvPauseMenuConfirmReturnToMap)) {
        mGameDataHolder->getPlayingFile()->requestReturnToMap();
        al::setNerve(this, &NrvPauseMenuEnd);
    } else if (al::isNerve(this, &NrvPauseMenuConfirmRestart)) {
        mGameDataHolder->getPlayingFile()->requestRestartStage();
        al::setNerve(this, &NrvPauseMenuEnd);
    } else if (al::isNerve(this, &NrvPauseMenuConfirmGameChange)) {
        al::setNerve(this, &NrvPauseMenuEndGameChange);
    } else if (al::isNerve(this, &NrvPauseMenuConfirmControllerChange)) {
        if (!mWindowConfirm->isAlive()) {
            rc::forceControllerApplet();
            al::setNerve(this, &NrvPauseMenuWaitControllerChange);
        }
    } else {
        al::setNerve(this, &NrvPauseMenuEnd);
    }
}

/** @brief Fades the screen back in after the assist mode was switched and closes the menu. */
void PauseMenu::exeEndAssistMode() {
    if (al::isFirstStep(this)) {
        mWipe->startOpen(cAssistModeWipeFrames);

        if (mGuideBar != nullptr) {
            killGuideBar(mGuideBar);
        }

        al::startFreezeActionEnd(this, "End", nullptr);
    }
}

/**
 * @brief Checks whether the menu was closed with the continue button.
 * @return True when continue was confirmed.
 */
bool PauseMenu::isDecideBack() const {
    return mButtonGroup->isDecide(cButtonContinue);
}

/**
 * @brief Checks whether the return to the map was confirmed.
 * @return True once the menu closes after the return to the map was chosen.
 */
bool PauseMenu::isDecideMap() const {
    return mButtonGroup->isDecide(cButtonReturnToMap) && al::isNerve(this, &NrvPauseMenuEnd);
}

/**
 * @brief Checks whether the stage restart was confirmed.
 * @return True once the menu closes after the restart was chosen.
 */
bool PauseMenu::isDecideReenterStage() const {
    return mButtonGroup->isDecide(cButtonRestart) && al::isNerve(this, &NrvPauseMenuEnd);
}

/**
 * @brief Checks whether the game is being quit.
 * @return True after quit was confirmed.
 */
bool PauseMenu::isDecideQuit() const {
    return al::isNerve(this, &NrvPauseMenuEndQuit);
}

/**
 * @brief Checks whether the switch to the other game was confirmed.
 * @return True a while after the switch was confirmed.
 */
bool PauseMenu::isDecideGameChange() const {
    return al::isNerve(this, &NrvPauseMenuEndGameChange) && al::isGreaterStep(this, 20);
}

/**
 * @brief Checks whether a save file has to be loaded.
 * @return True after a save file was chosen to be loaded.
 */
bool PauseMenu::isEndLoad() const {
    return al::isNerve(this, &NrvPauseMenuEndLoad);
}

/**
 * @brief Checks whether the menu is idle and waits for input.
 * @return True while the main buttons are waiting.
 */
bool PauseMenu::isWait() const {
    return al::isNerve(this, &NrvPauseMenuWait) && getCursorParts(mButtonGroup)->isWait();
}

/**
 * @brief Checks whether the closing animation finished.
 * @return True once the menu has closed.
 */
bool PauseMenu::isEnd() const {
    return al::isNerve(this, &NrvPauseMenuEnd) && al::isGreaterStep(this, 5) &&
           al::isActionEnd(this, nullptr);
}

/**
 * @brief Checks whether the assist mode switch finished.
 * @return True a while after the assist mode was switched.
 */
bool PauseMenu::isEndAssistMode() const {
    return al::isNerve(this, &NrvPauseMenuEndAssistMode) && al::isGreaterStep(this, 5);
}

/**
 * @brief Shows the name of the island the player is on in the header.
 * @param islandNo Island the player is on, or a negative value for none.
 */
void PauseMenu::setPauseMenuHeader(s32 islandNo) {
    if (islandNo >= 0 && islandNo != cIslandNoOpenWater) {
        auto* islandName = reinterpret_cast<const char16_t*>(
            IslandDataFunction::getIslandName(this, this, islandNo + 1));
        al::setPaneString(this, "TxtIslandName", islandName);
    } else {
        al::setPaneSystemMessage(this, "TxtIslandName", "IslandName", "OpenWater");
    }
}

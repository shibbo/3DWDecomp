#include "Layout/Switch/OptionsMenu.hpp"

#include <prim/seadSafeString.h>

#include "Layout/ButtonCursorParts.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/ButtonTextScrollParts.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/SceneCameraCtrl.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Camera/Holder/CameraRequestParamHolder.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/InputUtil.hpp"

// Non-const nerve objects: the nerves live in one writable data block next to the label tables.
#define OPTIONS_MENU_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(OptionsMenu, Appear);
NERVE_DECL(OptionsMenu, Wait);
NERVE_DECL(OptionsMenu, WaitKoopaJrDemo);
NERVE_DECL(OptionsMenu, End);

/** Closes the menu at the end of the Bowser Jr. assist demo. */
class OptionsMenuNrvEndKoopaJrDemo : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<OptionsMenu>()->exeEnd();
    }
};

NERVE_DECL(OptionsMenu, FullEndKoopaJrDemo);
NERVE_DECL(OptionsMenu, AppearKoopaJrDemo);

OPTIONS_MENU_NERVE_MAKE(OptionsMenu, WaitKoopaJrDemo)
OPTIONS_MENU_NERVE_MAKE(OptionsMenu, AppearKoopaJrDemo)
OPTIONS_MENU_NERVE_MAKE(OptionsMenu, Appear)
OPTIONS_MENU_NERVE_MAKE(OptionsMenu, Wait)
OPTIONS_MENU_NERVE_MAKE(OptionsMenu, End)
OPTIONS_MENU_NERVE_MAKE(OptionsMenu, EndKoopaJrDemo)
OPTIONS_MENU_NERVE_MAKE(OptionsMenu, FullEndKoopaJrDemo)

/** Message labels of the horizontal camera inversion button. */
const char* sHorizontalLabels[] = {"CameraNormal", "CameraReverse"};
/** Message labels of the vertical camera inversion button. */
const char* sVerticalLabels[] = {"CameraNormal", "CameraReverse"};
/** Message labels of the assist mode button. */
const char* sAssistModeLabels[] = {"AssistOff", "AssistLow", "AssistOn"};
/** Message labels of the camera sensitivity button (index = sensitivity + 2). */
const char* sSensitivityLabels[] = {"CameraSensitivityVeryLow", "CameraSensitivityLow",
                                    "CameraSensitivityMedium", "CameraSensitivityHigh",
                                    "CameraSensitivityVeryHigh"};

/** Offset between a camera sensitivity value and its label index. */
constexpr s32 cSensitivityLabelOffset = 2;
/** Frames the left/right input stays locked after moving the cursor up or down. */
constexpr s32 cSideInputDelay = 10;
}  // namespace

/**
 * @brief Creates the Super Mario 3D World options menu (camera inversion only).
 * @param rInfo Layout initialization context.
 * @param pCameraDirector Camera director that receives the inversion settings.
 * @param isKinopioBrigade Whether the menu is opened from Captain Toad mode.
 */
OptionsMenu::OptionsMenu(const al::LayoutInitInfo& rInfo, al::CameraDirector* pCameraDirector,
                         bool isKinopioBrigade)
    : al::LayoutActor("OptionsMenu"), mMainControllerPort(al::getMainControllerPort()),
      mCameraDirector(pCameraDirector), mCameraDirectorRS(nullptr),
      mIsKinopioBrigade(isKinopioBrigade) {
    al::initLayoutActor(this, rInfo, "RCS_PauseOptions", nullptr);
    mButtonGroup = new ButtonGroup(rInfo, this, "RCS_PauseOptions", nullptr, false);
    mHorizontalButton = new ButtonTextScrollParts(rInfo, "Horizontal", "ParButtonHorizontal",
                                                  this, "PauseMenu", sHorizontalLabels, 2);
    mVerticalButton = new ButtonTextScrollParts(rInfo, "Vertical", "ParButtonVertical", this,
                                                "PauseMenu", sVerticalLabels, 2);
    mButtonGroup->registerButton(this, mHorizontalButton);
    mButtonGroup->registerButton(this, mVerticalButton);
    mButtonGroup->resetDestination("Horizontal", "Vertical", "Vertical", nullptr, nullptr, true);
    mButtonGroup->resetDestination("Vertical", "Horizontal", "Horizontal", nullptr, nullptr,
                                   true);
    initNerve(&NrvOptionsMenuAppear, 0);
}

/**
 * @brief Creates the Bowser's Fury options menu (inversion, sensitivity and assist mode).
 * @param rInfo Layout initialization context.
 * @param pCameraDirector Camera director that receives the camera settings.
 */
OptionsMenu::OptionsMenu(const al::LayoutInitInfo& rInfo, al::CameraDirector_RS* pCameraDirector)
    : al::LayoutActor("OptionsMenu"), mMainControllerPort(al::getMainControllerPort()),
      mCameraDirector(nullptr), mCameraDirectorRS(pCameraDirector), mIsKinopioBrigade(false) {
    al::initLayoutActor(this, rInfo, "RCS_PauseOptions", nullptr);
    mButtonGroup = new ButtonGroup(rInfo, this, "RCS_PauseOptions", nullptr, false);
    mHorizontalButton = new ButtonTextScrollParts(rInfo, "Horizontal", "ParButtonHorizontal",
                                                  this, "PauseMenu", sHorizontalLabels, 2);
    mVerticalButton = new ButtonTextScrollParts(rInfo, "Vertical", "ParButtonVertical", this,
                                                "PauseMenu", sVerticalLabels, 2);
    mSensitivityButton = new ButtonTextScrollParts(rInfo, "Sensitivity", "ParButtonSensitivity",
                                                   this, "PauseMenu", sSensitivityLabels, 5);
    mAssistModeButton = new ButtonTextScrollParts(rInfo, "Assist", "ParButtonAssistmode", this,
                                                  "PauseMenu", sAssistModeLabels, 3);
    mButtonGroup->registerButton(this, mSensitivityButton);
    mButtonGroup->registerButton(this, mHorizontalButton);
    mButtonGroup->registerButton(this, mVerticalButton);
    mButtonGroup->registerButton(this, mAssistModeButton);
    mButtonGroup->resetDestination("Horizontal", "Assist", "Vertical", nullptr, nullptr, true);
    mButtonGroup->resetDestination("Vertical", "Horizontal", "Sensitivity", nullptr, nullptr,
                                   true);
    mButtonGroup->resetDestination("Sensitivity", "Vertical", "Assist", nullptr, nullptr, true);
    mButtonGroup->resetDestination("Assist", "Sensitivity", "Horizontal", nullptr, nullptr, true);
    initNerve(&NrvOptionsMenuAppear, 0);
}

/** @brief Plays the appear animation, then waits for input. */
void OptionsMenu::exeAppear() {
    if (al::isFirstStep(this)) {
        mButtonGroup->invalidate();
        al::startAction(this, "Appear", "Main");
        mButtonGroup->reset();
        mButtonGroup->hideCursor();
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvOptionsMenuWait);
    }
}

/** @brief Plays the appear animation of the assist demo menu, then waits for input. */
void OptionsMenu::exeAppearKoopaJrDemo() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", "Main");
        mButtonGroup->hideCursor();
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvOptionsMenuWaitKoopaJrDemo);
    }
}

/** @brief Handles cursor input and writes the chosen settings to the save data on decide. */
void OptionsMenu::exeWait() {
    if (al::isFirstStep(this)) {
        mButtonGroup->validate();
        mButtonGroup->select("Horizontal");
        mButtonGroup->getCursorParts()->reset();
        mButtonGroup->showCursor();
        al::startAction(this, "Wait", "Main");
        mSideInputDelay = cSideInputDelay;
    }

    mButtonGroup->update();
    if (mButtonGroup->isInputLocked()) {
        return;
    }

    if (mCameraDirector != nullptr) {
        if (!mHorizontalButton->isEnableControl() || !mVerticalButton->isEnableControl()) {
            mButtonGroup->invalidate();
            return;
        }
    } else if (!mHorizontalButton->isEnableControl() || !mVerticalButton->isEnableControl() ||
               !mSensitivityButton->isEnableControl() || !mAssistModeButton->isEnableControl()) {
        mButtonGroup->invalidate();
        return;
    }

    mButtonGroup->validate();
    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        al::startSe(this, "Decided");

        s32 changedNum;
        if (mIsKinopioBrigade) {
            changedNum = GameDataFunction::setCameraReverseHorizontal(
                GameDataHolderWriter(this), mHorizontalButton->getLabelIdx() != 0);
            changedNum += GameDataFunction::setCameraReverseVertical(
                GameDataHolderWriter(this), mVerticalButton->getLabelIdx() != 0);
            GameDataFunction::setKinopioBrigadeCameraReverseHorizontal(
                GameDataHolderWriter(this), mHorizontalButton->getLabelIdx() != 0);
            GameDataFunction::setKinopioBrigadeCameraReverseVertical(
                GameDataHolderWriter(this), mVerticalButton->getLabelIdx() != 0);
        } else if (mCameraDirector != nullptr) {
            changedNum = GameDataFunction::setCameraReverseHorizontal(
                GameDataHolderWriter(this), mHorizontalButton->getLabelIdx() != 0);
            changedNum += GameDataFunction::setCameraReverseVertical(
                GameDataHolderWriter(this), mVerticalButton->getLabelIdx() != 0);
            GameDataFunction::setKinopioBrigadeCameraReverseHorizontal(
                GameDataHolderWriter(this), mHorizontalButton->getLabelIdx() != 0);
            GameDataFunction::setKinopioBrigadeCameraReverseVertical(
                GameDataHolderWriter(this), mVerticalButton->getLabelIdx() != 0);
        } else {
            changedNum = SingleModeDataFunction::setCameraReverseHorizontal(
                GameDataHolderWriter(this), mHorizontalButton->getLabelIdx() != 0);
            changedNum += SingleModeDataFunction::setCameraReverseVertical(
                GameDataHolderWriter(this), mVerticalButton->getLabelIdx() != 0);
            changedNum += SingleModeDataFunction::setCameraSensitivity(
                GameDataHolderWriter(this),
                mSensitivityButton->getLabelIdx() - cSensitivityLabelOffset);
            changedNum += SingleModeDataFunction::setAssistModeType(
                GameDataHolderWriter(this), mAssistModeButton->getLabelIdx());
        }

        if (changedNum != 0) {
            GameDataFunction::playReportOptionsEvent(GameDataHolderWriter(this));
        }

        if (GameDataFunction::isSaveRequested(GameDataHolderAccessor(this))) {
            GameDataFunction::setSaveRequested(GameDataHolderWriter(this), false);
            SaveDataAccessFunction::startSaveDataWriteSync(
                GameDataHolderAccessor(this).getHolder(), true);
        }

        mButtonGroup->invalidate();
        al::setNerve(this, &NrvOptionsMenuEnd);
    }

    if (rc::isPadTriggerUiCancelByPort(mPort)) {
        al::startSe(this, "Canceled");
        mButtonGroup->invalidate();
        al::setNerve(this, &NrvOptionsMenuEnd);
        return;
    }

    if (rc::isPadTriggerUiUpByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Up);
        mSideInputDelay = 0;
    } else if (rc::isPadTriggerUiDownByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Down);
        mSideInputDelay = 0;
    } else if (mSideInputDelay >= cSideInputDelay) {
        if (rc::isPadTriggerUiLeftByPort(mPort)) {
            mButtonGroup->tryMove(ButtonGroup::Direction_Left);
        } else if (rc::isPadTriggerUiRightByPort(mPort)) {
            mButtonGroup->tryMove(ButtonGroup::Direction_Right);
        }

        return;
    }

    mSideInputDelay++;
}

/**
 * @brief Handles input in the Bowser Jr. assist demo, where only the assist mode can be changed.
 */
void OptionsMenu::exeWaitKoopaJrDemo() {
    if (al::isFirstStep(this)) {
        mButtonGroup->validate();
        mButtonGroup->select("Assist");
        mButtonGroup->getCursorParts()->reset();
        mButtonGroup->showCursor();
        al::startAction(this, "Wait", "Main");
        mSideInputDelay = cSideInputDelay;
    }

    mButtonGroup->update();
    if (mButtonGroup->isInputLocked()) {
        return;
    }

    if (mCameraDirector != nullptr) {
        if (!mHorizontalButton->isEnableControl() || !mVerticalButton->isEnableControl()) {
            mButtonGroup->invalidate();
            return;
        }
    } else if (!mHorizontalButton->isEnableControl() || !mVerticalButton->isEnableControl() ||
               !mSensitivityButton->isEnableControl() || !mAssistModeButton->isEnableControl()) {
        mButtonGroup->invalidate();
        return;
    }

    mButtonGroup->validate();
    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        al::startSe(this, "Decided");
        SingleModeDataFunction::setCameraReverseHorizontal(
            GameDataHolderWriter(this), mHorizontalButton->getLabelIdx() != 0);
        SingleModeDataFunction::setCameraReverseVertical(GameDataHolderWriter(this),
                                                         mVerticalButton->getLabelIdx() != 0);
        SingleModeDataFunction::setCameraSensitivity(
            GameDataHolderWriter(this),
            mSensitivityButton->getLabelIdx() - cSensitivityLabelOffset);
        SingleModeDataFunction::setAssistModeType(GameDataHolderWriter(this),
                                                  mAssistModeButton->getLabelIdx());
        mButtonGroup->invalidate();

        if (GameDataFunction::isSaveRequested(GameDataHolderAccessor(this))) {
            GameDataFunction::setSaveRequested(GameDataHolderWriter(this), false);
            SaveDataAccessFunction::startSaveDataWriteSync(
                GameDataHolderAccessor(this).getHolder(), true);
        }

        al::setNerve(this, &NrvOptionsMenuEndKoopaJrDemo);
    }

    if (mSideInputDelay >= cSideInputDelay) {
        if (rc::isPadTriggerUiLeftByPort(mPort)) {
            mButtonGroup->tryMove(ButtonGroup::Direction_Left);
        } else if (rc::isPadTriggerUiRightByPort(mPort)) {
            mButtonGroup->tryMove(ButtonGroup::Direction_Right);
        }
    } else {
        mSideInputDelay++;
    }
}

/** @brief Plays the end animation, applies the saved camera settings and closes the menu. */
void OptionsMenu::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", "Main");
        mButtonGroup->getCursorParts()->hide();
        if (mCameraDirector != nullptr) {
            if (mIsKinopioBrigade) {
                mCameraDirector->setReverseHorizontal(
                    GameDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
                mCameraDirector->setReverseVertical(
                    GameDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
                mCameraDirector->setKinopioBrigadeReverseHorizontal(
                    GameDataFunction::getKinopioBrigadeCameraReverseHorizontal(
                        GameDataHolderWriter(this)));
                mCameraDirector->setKinopioBrigadeReverseVertical(
                    GameDataFunction::getKinopioBrigadeCameraReverseVertical(
                        GameDataHolderWriter(this)));
            } else {
                mCameraDirector->setReverseHorizontal(
                    GameDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
                mCameraDirector->setReverseVertical(
                    GameDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
                mCameraDirector->setKinopioBrigadeReverseHorizontal(
                    GameDataFunction::getKinopioBrigadeCameraReverseHorizontal(
                        GameDataHolderWriter(this)));
                mCameraDirector->setKinopioBrigadeReverseVertical(
                    GameDataFunction::getKinopioBrigadeCameraReverseVertical(
                        GameDataHolderWriter(this)));
            }
        } else if (mCameraDirectorRS != nullptr) {
            mCameraDirectorRS->setReverseRightAndLeftFlag(
                SingleModeDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
            mCameraDirectorRS->setReverseUpAndDownFlag(
                SingleModeDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
            mCameraDirectorRS->getSceneCameraCtrl()
                ->getRequestParamHolder()
                ->setStickSensitivityLevel(
                    SingleModeDataFunction::getCameraSensitiviy(GameDataHolderWriter(this)));
        }
    }

    if (al::isNerve(this, &NrvOptionsMenuEndKoopaJrDemo)) {
        al::setNerve(this, &NrvOptionsMenuFullEndKoopaJrDemo);
        return;
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/** @brief Idles once the assist demo menu has closed. */
void OptionsMenu::exeFullEndKoopaJrDemo() {}

/**
 * @brief Checks whether the assist demo menu has fully closed.
 * @return True once the menu has finished closing.
 */
bool OptionsMenu::isKoopaJrDemoEnd() {
    return al::isNerve(this, &NrvOptionsMenuFullEndKoopaJrDemo);
}

/**
 * @brief Opens the menu with the buttons set to the current save data.
 * @param port Controller port that operates the menu.
 */
void OptionsMenu::appear(s32 port) {
    al::LayoutActor::appear();
    mPort = port;
    mButtonGroup->setPort(port);
    if (mIsKinopioBrigade) {
        mHorizontalButton->setLabelIdx(GameDataFunction::getKinopioBrigadeCameraReverseHorizontal(
            GameDataHolderWriter(this)));
        mVerticalButton->setLabelIdx(GameDataFunction::getKinopioBrigadeCameraReverseVertical(
            GameDataHolderWriter(this)));
    } else if (mCameraDirector != nullptr) {
        mHorizontalButton->setLabelIdx(
            GameDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
        mVerticalButton->setLabelIdx(
            GameDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
    } else {
        mSensitivityButton->setLabelIdx(
            SingleModeDataFunction::getCameraSensitiviy(GameDataHolderWriter(this)) +
            cSensitivityLabelOffset);
        mHorizontalButton->setLabelIdx(
            SingleModeDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
        mVerticalButton->setLabelIdx(
            SingleModeDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
        s32 assistModeType = SingleModeDataFunction::getAssistModeType(GameDataHolderWriter(this));
        mAssistModeButton->setLabelIdx(assistModeType);
        al::startAction(this, "SetPauseOptionsSingle", "OptionMenuType");
        al::startAction(this, "SetAssistModeInfoOff", "Info");
        al::setNerve(this, &NrvOptionsMenuAppear);
        return;
    }

    al::startAction(this, "SetPauseOptions3DW", "OptionMenuType");
    al::startAction(this, "SetAssistModeInfoOff", "Info");
    al::setNerve(this, &NrvOptionsMenuAppear);
}

/**
 * @brief Opens the menu for the Bowser Jr. assist demo, where only the assist mode is enabled.
 * @param port Controller port that operates the menu.
 */
void OptionsMenu::appearKoopaJrDemo(s32 port) {
    al::LayoutActor::appear();
    mPort = port;
    mButtonGroup->setPort(port);
    mSensitivityButton->setLabelIdx(
        SingleModeDataFunction::getCameraSensitiviy(GameDataHolderWriter(this)) +
        cSensitivityLabelOffset);
    mSensitivityButton->disable();
    mHorizontalButton->setLabelIdx(
        SingleModeDataFunction::getCameraReverseHorizontal(GameDataHolderWriter(this)));
    mHorizontalButton->disable();
    mVerticalButton->setLabelIdx(
        SingleModeDataFunction::getCameraReverseVertical(GameDataHolderWriter(this)));
    mVerticalButton->disable();
    s32 assistModeType = SingleModeDataFunction::getAssistModeType(GameDataHolderWriter(this));
    mAssistModeButton->setLabelIdx(assistModeType);
    al::startAction(this, "SetPauseOptionsSingle", "OptionMenuType");
    al::startAction(this, "SetAssistModeInfoOff", "Info");
    al::setNerve(this, &NrvOptionsMenuAppearKoopaJrDemo);
}

/** @brief Starts closing the menu if it is waiting for input. */
void OptionsMenu::forceExit() {
    if (al::isNerve(this, &NrvOptionsMenuWait)) {
        al::setNerve(this, &NrvOptionsMenuEnd);
    }
}

/**
 * @brief Checks whether the menu is playing its end animation.
 * @return True while closing.
 */
bool OptionsMenu::isEnding() {
    return al::isNerve(this, &NrvOptionsMenuEnd);
}

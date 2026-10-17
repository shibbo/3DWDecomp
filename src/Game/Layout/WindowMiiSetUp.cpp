#include "Layout/WindowMiiSetUp.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/ButtonTextScrollParts.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/InputUtil.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
NERVE_DECL(WindowMiiSetUp, End);
NERVE_DECL(WindowMiiSetUp, Appear);
NERVE_DECL(WindowMiiSetUp, Error);
NERVE_DECL(WindowMiiSetUp, Wait);
struct {
    WindowMiiSetUpNrvEnd End;
    WindowMiiSetUpNrvAppear Appear;
    WindowMiiSetUpNrvError Error;
    WindowMiiSetUpNrvWait Wait;
    const char* GhostLabels[2] = {"GhostShowAll", "GhostShowNoOne"};
    const char* MiiverseLabels[2] = {"MiiverseShowAll", "MiiverseShowNoOne"};
} NrvWindowMiiSetUp;
}
namespace GameDataFunction { bool isMiiverseForbidden(GameDataHolderAccessor accessor); }
/** @brief Creates option buttons, their navigation links, and the error dialog. */
WindowMiiSetUp::WindowMiiSetUp(const al::LayoutInitInfo& rInfo, const char* pLayoutName)
    : al::LayoutActor("Mii設定ウィンドウ") {
    al::initLayoutActor(this, rInfo, pLayoutName, nullptr);
    mButtons = new ButtonGroup(rInfo, this, pLayoutName, nullptr, false);
    mGhost = new ButtonTextScrollParts(rInfo, "Ghost表示設定", "ParScrollButtonGhost", this, "PauseMenu", NrvWindowMiiSetUp.GhostLabels, 2);
    mMiiverse = new ButtonTextScrollParts(rInfo, "Miiverse表示設定", "ParScrollButtonComment", this, "PauseMenu", NrvWindowMiiSetUp.MiiverseLabels, 2);
    mButtons->registerButton(this, mGhost);
    mButtons->registerButton(this, mMiiverse);
    mButtons->resetDestination("キャンセル", "Ghost表示設定", nullptr, nullptr, "OK", true);
    mButtons->resetDestination("OK", "Ghost表示設定", nullptr, "キャンセル", nullptr, true);
    mButtons->resetDestination("Ghost表示設定", "Miiverse表示設定", "OK", nullptr, nullptr, true);
    mButtons->resetDestination("Miiverse表示設定", nullptr, "Ghost表示設定", nullptr, nullptr, true);
    mError = new WindowConfirm(WindowConfirmType_Notice, rInfo, "PauseMenu", false);
    initNerve(&NrvWindowMiiSetUp.Appear, 0);
}
/** @brief Checks parental restrictions before opening the settings. */
void WindowMiiSetUp::exeAppear() {
    if (al::isFirstStep(this)) {
        if (GameDataFunction::isMiiverseForbidden(GameDataHolderAccessor(this)) || GameDataFunction::isInGameNetworkForbidden(GameDataHolderAccessor(this))) {
            al::setNerve(this, &NrvWindowMiiSetUp.Error);
            return;
        }
        mButtons->invalidate();
        mButtons->reset();
        al::showPaneRootNoRecursive(this);
        al::startAction(this, "Appear", "Main");
        mButtons->hideCursor();
    }
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvWindowMiiSetUp.Wait);
}
/** @brief Handles option navigation, saving, and cancellation. */
void WindowMiiSetUp::exeWait() {
    if (al::isFirstStep(this)) {
        mButtons->validate();
        mButtons->select("Miiverse表示設定");
        mButtons->getCursorParts()->reset();
        mButtons->showCursor();
        al::startAction(this, "Wait", "Main");
    }
    mButtons->update();
    if (mButtons->isInputLocked()) return;
    if (!mGhost->isEnableControl() || !mMiiverse->isEnableControl()) {
        mButtons->invalidate();
        return;
    }
    mButtons->validate();
    if (rc::isPadTriggerUiUpByPort(mPort)) mButtons->tryMove(0);
    if (rc::isPadTriggerUiDownByPort(mPort)) mButtons->tryMove(1);
    if (rc::isPadTriggerUiLeftByPort(mPort)) mButtons->tryMove(2);
    if (rc::isPadTriggerUiRightByPort(mPort)) mButtons->tryMove(3);
    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        if (mButtons->isSelect("キャンセル")) mButtons->decide("キャンセル");
        else if (mButtons->isSelect("OK")) {
            mButtons->decide("OK");
            GameDataFunction::setGhostSetting(GameDataHolderWriter(this), mGhost->getLabelIdx());
            GameDataFunction::setMiiverseSetting(GameDataHolderWriter(this), mMiiverse->getLabelIdx());
        } else mButtons->tryMove(1);
    }
    if (rc::isPadTriggerUiCancelByPort(mPort)) {
        mButtons->decide("キャンセル");
        al::startSe(this, "Canceled", nullptr);
        al::setNerve(this, &NrvWindowMiiSetUp.End);
    } else if (mButtons->isDecideAny()) {
        const char* pName = mButtons->getSelectedButton()->getName();
        if (al::isEqualString(pName, "キャンセル")) {
            mButtons->decide("キャンセル");
            al::startSe(this, "Canceled", nullptr);
        }
        if (al::isEqualString(pName, "OK")) {
            mButtons->decide("OK");
            al::startSe(this, "Decided", nullptr);
            GameDataFunction::setGhostSetting(GameDataHolderWriter(this), mGhost->getLabelIdx());
            GameDataFunction::setMiiverseSetting(GameDataHolderWriter(this), mMiiverse->getLabelIdx());
        }
        mButtons->invalidate();
        al::setNerve(this, &NrvWindowMiiSetUp.End);
    }
}
/** @brief Displays the parental-lock error and closes when acknowledged. */
void WindowMiiSetUp::exeError() {
    if (al::isFirstStep(this)) mError->appearWithSystemMessage("ErrorMessage", "ParentalLock", mPort, nullptr);
    if (!mError->isAlive()) kill();
}
/** @brief Plays the closing animation and hides the cursor. */
void WindowMiiSetUp::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", "Main");
        mButtons->getCursorParts()->hide();
    }
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Opens the settings with saved option values for the requested controller. */
void WindowMiiSetUp::appear(int port) {
    al::LayoutActor::appear();
    mPort = port;
    mButtons->setPort(port);
    al::hidePaneRootNoRecursive(this);
    mGhost->setLabelIdx(GameDataFunction::getGhostSetting(GameDataHolderAccessor(this)));
    mMiiverse->setLabelIdx(GameDataFunction::getMiiverseSetting(GameDataHolderAccessor(this)));
    al::setNerve(this, &NrvWindowMiiSetUp.Appear);
}
/** @brief Reports a closed settings window. */
bool WindowMiiSetUp::isEnd() const { return !isAlive(); }
/** @brief Opening requires the controller-specific overload. */
void WindowMiiSetUp::appear() {}

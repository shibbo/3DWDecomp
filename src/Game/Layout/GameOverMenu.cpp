#include "Layout/GameOverMenu.hpp"

#include "Layout/ButtonGroup.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
NERVE_DECL(GameOverMenu, Appear);
NERVE_DECL(GameOverMenu, Wait);
NERVE_DECL(GameOverMenu, End);
NERVES_MAKE_NOSTRUCT(GameOverMenu, Appear, Wait, End)
}  // namespace

/** @brief Creates the game-over menu controller.
 * @param pGameData Player and controller-assignment data. */
GameOverMenu::GameOverMenu(const GameDataHolder* pGameData)
    : al::LayoutActor("ゲームオーバーメニュー"), mGameData(pGameData) {}

/** @brief Initializes the menu layout and its button group.
 * @param rInfo Layout initialization context. */
void GameOverMenu::init(const al::LayoutInitInfo& rInfo) {
    initNerve(&NrvGameOverMenuAppear, 0);
    al::initLayoutActor(this, rInfo, "GameOverMenu", nullptr);
    mButtonGroup = new ButtonGroup(rInfo, this, "GameOverMenu", nullptr, false);
    kill();
}

/** @brief Shows the menu with Continue selected for the first active player's controller. */
void GameOverMenu::appear() {
    al::LayoutActor::appear();
    mPadPort = rc::calcPadPortByFirstActiveUser(
        GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData)));
    mButtonGroup->setPort(mPadPort);
    al::startAction(this, "Appear", nullptr);
    mButtonGroup->reset();
    mButtonGroup->hideCursor();
    mButtonGroup->updateCursor();
    mButtonGroup->select("上ボタン");
    calcAnim(true);
    al::setNerve(this, &NrvGameOverMenuAppear);
}

/** @brief Checks whether Continue was selected.
 * @return Whether the upper button was confirmed. */
bool GameOverMenu::isDecideContinue() const {
    return mButtonGroup->isDecide("上ボタン");
}

/** @brief Checks whether Quit was selected.
 * @return Whether the lower button was confirmed. */
bool GameOverMenu::isDecideQuit() const {
    return mButtonGroup->isDecide("下ボタン");
}

/** @brief Enables button input once the appearance action finishes. */
void GameOverMenu::exeAppear() {
    if (al::isActionEnd(this, nullptr)) {
        mButtonGroup->validate();
        mButtonGroup->showCursor();
        al::setNerve(this, &NrvGameOverMenuWait);
    }
}

/** @brief Waits for a button confirmation before disabling input and closing. */
void GameOverMenu::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }
    mButtonGroup->updateAndCursorDefault(mPadPort);
    if (mButtonGroup->isDecideAny()) {
        mButtonGroup->hideCursor();
        mButtonGroup->invalidate();
        al::setNerve(this, &NrvGameOverMenuEnd);
    }
}

/** @brief Plays the selection sound and closing animation, then hides the menu. */
void GameOverMenu::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        if (isDecideQuit()) {
            al::startSe(this, "Canceled", nullptr);
        } else {
            al::startSe(this, "Decided", nullptr);
        }
    }
    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}

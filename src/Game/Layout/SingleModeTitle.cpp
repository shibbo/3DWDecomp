#include "Layout/SingleModeTitle.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/CursorTarget.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
namespace {
NERVE_DECL(SingleModeTitle, Appear);
NERVE_DECL(SingleModeTitle, Wait);
class SingleModeTitleNrvResume : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<SingleModeTitle>()->exeDecide(); }
};
class SingleModeTitleNrvNewGame : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<SingleModeTitle>()->exeDecide(); }
};
class SingleModeTitleNrvBack : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<SingleModeTitle>()->exeDecide(); }
};
class SingleModeTitleNrvStartNewGame : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<SingleModeTitle>()->exeEnd(); }
};
NERVE_DECL(SingleModeTitle, End);
NERVES_MAKE_NOSTRUCT(SingleModeTitle, Appear, Wait, Resume, NewGame, Back, StartNewGame, End)
}
/** @brief Creates the title menu and its button group. */
SingleModeTitle::SingleModeTitle(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData)
    : al::LayoutActor("SingleModeTitle"), mGameData(pGameData) {
    al::initLayoutActor(this, rInfo, "RCS_TitleLogoNew", nullptr);
    initNerve(&NrvSingleModeTitleAppear, 0);
    mButtons = new ButtonGroup(rInfo, this, "RCS_TitleLogoNew", nullptr, false);
}
/** @brief Starts the title entrance animation. */
void SingleModeTitle::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvSingleModeTitleAppear);
}
/** @brief Restores the title after returning from another menu. */
void SingleModeTitle::returnFromMenu(const char* pButtonName) {
    if (GameDataFunction::isAllNewFile(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData))))
        al::startAction(this, "SetScreenUnplayed", "ButtonSetOnOff");
    else {
        mButtons->reset();
        if (pButtonName) mButtons->select(pButtonName);
        al::startAction(this, "SetScreenPlayed", "ButtonSetOnOff");
    }
    al::setNerve(this, &NrvSingleModeTitleWait);
}
/** @brief Selects the title presentation according to existing save data. */
void SingleModeTitle::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", "Main");
        if (GameDataFunction::isAllNewFile(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData))))
            al::startAction(this, "SetScreenUnplayed", "ButtonSetOnOff");
        else al::startAction(this, "SetScreenPlayed", "ButtonSetOnOff");
    }
    if (al::isActionEnd(this, "Main")) al::setNerve(this, &NrvSingleModeTitleWait);
}
/** @brief Synchronizes title animation to music and handles menu input. */
void SingleModeTitle::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StartTextLoop", "StartLoop");
        if (!GameDataFunction::isAllNewFile(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData)))) {
            mButtons->reset();
            mButtons->select("Resume");
            mButtons->showCursorAppear();
            mButtons->validate();
        }
        float frame = al::getRhythmAnimFrame(this) - 2.0f;
        mStartLoopFrame = al::wrapValue(frame, al::getActionFrameMax(this, "StartTextLoop", "StartLoop"));
    }
    if (al::isEnableRhythmAnim(this, nullptr)) {
        float frame = al::getRhythmAnimFrame(this) - 2.0f;
        al::setActionFrame(this, al::wrapValue(frame, al::getActionFrameMax(this, nullptr)), nullptr);
    }
    int port = al::getMainControllerPort();
    bool isDecide = al::isPadTriggerA(port);
    if (!GameDataFunction::isAllNewFile(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData)))) {
        mButtons->updateAndCursorDefault(port);
        if (isDecide) mButtons->getSelectedButton()->decide();
        if (mButtons->isDecideAny()) {
            if (al::isEqualString(mButtons->getSelectedButtonName(), "Resume"))
                al::setNerve(this, &NrvSingleModeTitleResume);
            else {
                mNewFileId = GameDataFunction::getFirstNewFileId(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData)));
                al::setNerve(this, &NrvSingleModeTitleNewGame);
            }
            mButtons->hideCursor();
            return;
        }
    }
    if (isDecide) al::setNerve(this, &NrvSingleModeTitleNewGame);
    else if (al::isPadTriggerB(port)) {
        mIsBack = true;
        al::setNerve(this, &NrvSingleModeTitleBack);
    }
}
/** @brief Resolves the selected title action. */
void SingleModeTitle::exeDecide() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvSingleModeTitleNewGame)) {
            if (GameDataFunction::getFirstNewFileId(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData))) >= 0)
                al::startSe(this, "TitleStart", nullptr);
            al::setNerve(this, &NrvSingleModeTitleStartNewGame);
        } else al::setNerve(this, &NrvSingleModeTitleEnd);
    }
}
/** @brief Holds the completed title state. */
void SingleModeTitle::exeEnd() {}
/** @brief Reports that a new game was requested. */
bool SingleModeTitle::isStartNewGame() const { return al::isNerve(this, &NrvSingleModeTitleStartNewGame); }
/** @brief Reports completion without starting a new game. */
bool SingleModeTitle::isEnd() { return al::isNerve(this, &NrvSingleModeTitleEnd); }

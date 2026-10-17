#include "Demo/DemoKoopaW7.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Demo/DemoScenePlayer.hpp"
#include "Demo/IDemoScenePlayerInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "MapObj/GoalPole.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}

/** @brief Orders cutscene characters by their goal-pole height. */
class DemoKoopaW7ScenePlayerInfo : public IDemoScenePlayerInfo {
public:
    /** @brief Creates the player selector. @param pDemo Owning demo. */
    explicit DemoKoopaW7ScenePlayerInfo(DemoKoopaW7* pDemo) : mDemo(pDemo) {}
    void setup(DemoScenePlayer* pPlayer) override;
    /** @brief Gets the number of successful players. @return Player count. */
    int getActivePlayerNum() const override { return mPlayerCount; }
    /** @brief Gets a character. @param index Height-order index. @return Character type. */
    int getCharacterType(int index) const override { return mCharacters[index]; }
    /** @brief Gets a power-up form. @param index Height-order index. @return Figure type. */
    int getFigureType(int index) const override { return mFigures[index]; }

private:
    DemoKoopaW7* mDemo;
    int mPlayerCount = 0;
    int mCharacters[4];
    int mFigures[4];
};

namespace {
NERVE_DECL(DemoKoopaW7, CloseFade);
NERVE_DECL(DemoKoopaW7, Done);
NERVE_DECL(DemoKoopaW7, WaitFade);
NERVE_DECL(DemoKoopaW7, Play);
NERVE_DECL(DemoKoopaW7, EndFade);
NERVE_DECL(DemoKoopaW7, WaitEnd);
NERVES_MAKE_NOSTRUCT(DemoKoopaW7, CloseFade, Done, WaitFade, Play, EndFade, WaitEnd)
}

/** @brief Collects successful players in goal order. @param pPlayer Cutscene player context. */
void DemoKoopaW7ScenePlayerInfo::setup(DemoScenePlayer* pPlayer) {
    mPlayerCount = 0;
    int users[4] = {-1, -1, -1, -1};
    auto* pole = mDemo->getGoalPole();
    for (int i = 0; i < rc::getControlUserNumMax(); ++i) {
        if (pole->isCatchSuccess(i)) {
            users[pole->getHeightOrder(i)] = i;
            ++mPlayerCount;
        }
    }
    for (int i = 0; i < mPlayerCount; ++i) {
        int user = users[i];
        auto* player = rc::tryFindAlivePlayerActorFirstByUserId(pPlayer, user);
        mCharacters[i] = rc::getControlUserCharacterType(GameDataHolderAccessor(pPlayer), user);
        mFigures[i] = player ? rc::getPlayerFigureTypeWithNext(player) : 0;
    }
}

/** @brief Creates the World 7 cutscene. @param pName Actor name. */
DemoKoopaW7::DemoKoopaW7(const char* pName) : al::LiveActor(pName) {}
/** @brief Assigns the goal pole. @param pGoalPole Goal pole. */
void DemoKoopaW7::setGoalPole(GoalPole* pGoalPole) { mGoalPole = pGoalPole; }

/** @brief Creates the cutscene and its wipes. @param rInfo Actor initialization data. */
void DemoKoopaW7::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvDemoKoopaW7CloseFade, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    al::initStageSwitch(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    makeActorAppeared();
    mDemo = rc::createDemoSceneHolder("DemoKoopaCatchFairyAgainStage", rInfo,
        rc::createPlayerRetargettingSelector(this), nullptr, false, 4);
    rc::setDemoScenePlayerInfo(mDemo, new DemoKoopaW7ScenePlayerInfo(this));
    mCurtain = new al::WipeSimple("デモカーテンワイプ", "WipeCurtainDemo", al::getLayoutInitInfo(rInfo), nullptr);
    mCurtain->kill();
    mWhiteFade = new al::WipeSimple("白フェード", "WipeFadeWhite", al::getLayoutInitInfo(rInfo), nullptr);
    mWhiteFade->kill();
    makeActorDead();
}

/** @brief Starts the curtain-close sequence. */
void DemoKoopaW7::startDemo() {
    appear();
    al::setNerve(this, &NrvDemoKoopaW7CloseFade);
}
/** @brief Checks completion. @return Whether the cutscene has finished. */
bool DemoKoopaW7::isEndDemo() const { return al::isNerve(this, &NrvDemoKoopaW7Done); }

/** @brief Closes the curtain and prepares the goal-pole players. */
void DemoKoopaW7::exeCloseFade() {
    if (al::isFirstStep(this)) mCurtain->startClose(-1);
    else if (mCurtain->isCloseEnd()) {
        mGoalPole->prepareDemoW7KoopaCastleClear();
        al::setNerve(this, &NrvDemoKoopaW7WaitFade);
    }
}

/** @brief Signals the stage while the curtain is closed, then begins playback. */
void DemoKoopaW7::exeWaitFade() {
    if (al::isStep(this, 40)) {
        al::tryOnStageSwitch(this, "SwitchDemoStartOn");
        al::startHitReaction(mGoalPole, "W7クッパデモカーテン中");
    }
    if (al::isGreaterEqualStep(this, 120)) al::setNerve(this, &NrvDemoKoopaW7Play);
}

/** @brief Plays the cutscene and its timed music cues. */
void DemoKoopaW7::exePlay() {
    if (al::isFirstStep(this)) {
        mCurtain->startOpen(-1);
        rc::tryStartDemo(mDemo);
        mDemo->startAction(0, false);
        rc::tryHideDemoDisableFairyPrincess(mDemo, this);
        mGoalPole->startDemoW7KoopaCastleClear();
    }
    if (al::isStep(this, 200)) al::startBgm(this, "KoopaW7Demo01", -1, 0, -1, -1);
    if (al::isStep(this, 735)) al::startSequenceBgm(this, "KoopaW7Demo02", -1, 0);
    if (mDemo->isActionEndCamera(120)) al::setNerve(this, &NrvDemoKoopaW7EndFade);
}

/** @brief Closes the white fade at the end of playback. */
void DemoKoopaW7::exeEndFade() {
    if (al::isFirstStep(this)) mWhiteFade->startClose(120);
    if (mWhiteFade->isCloseEnd()) al::setNerve(this, &NrvDemoKoopaW7WaitEnd);
}
/** @brief Waits before releasing the cutscene. */
void DemoKoopaW7::exeWaitEnd() {
    if (al::isGreaterEqualStep(this, 60)) {
        rc::tryEndDemo(mDemo);
        al::setNerve(this, &NrvDemoKoopaW7Done);
    }
}
/** @brief Keeps the completed cutscene idle. */
void DemoKoopaW7::exeDone() {}

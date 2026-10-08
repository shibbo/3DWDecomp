#include "Scene/SingleModeStateGameOver.hpp"

#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include "Demo/DemoSceneActorHolder.hpp"
#include "Layout/GameOverMenu.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/PadRumbleDirector.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/SimpleLayoutAppear.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWait.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Scene/GameOverGraphicsState.hpp"
#include "Scene/SingleModeScene.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/DemoUtil.hpp"

class PlayerRetargettingSelector;

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}  // namespace rc

namespace {
NERVE_DECL(SingleModeStateGameOver, Wait)
NERVE_DECL(SingleModeStateGameOver, Miss)
NERVE_DECL(SingleModeStateGameOver, DemoGameOver)
NERVE_DECL(SingleModeStateGameOver, GameOverMenu)
NERVE_DECL(SingleModeStateGameOver, DemoContinue)
NERVE_DECL(SingleModeStateGameOver, Done)
NERVES_MAKE_NOSTRUCT(SingleModeStateGameOver, Wait, Miss, DemoGameOver, GameOverMenu,
                     DemoContinue, Done)
}  // namespace

/**
 * Constructs the game over state and creates its layouts, demo and graphics settings.
 * @param pScene The owning scene.
 * @param rLayoutInfo Layout initialization info.
 * @param rActorInfo Actor initialization info.
 * @param pGameDataHolder The game data holder.
 * @param pAudioDirector The audio director.
 * @param pWipe The wipe closed during the miss.
 * @param pMissLayout The miss layout.
 */
SingleModeStateGameOver::SingleModeStateGameOver(SingleModeScene* pScene,
                                                 const al::LayoutInitInfo& rLayoutInfo,
                                                 const al::ActorInitInfo& rActorInfo,
                                                 GameDataHolder* pGameDataHolder,
                                                 al::AudioDirector* pAudioDirector,
                                                 al::WipeSimple* pWipe,
                                                 al::SimpleLayoutAppearWait* pMissLayout)
    : al::NerveStateBase("SingleModeStateGameOver"), mScene(pScene),
      mGameDataHolder(pGameDataHolder), mAudioDirector(pAudioDirector), mWipe(pWipe),
      mMissLayout(pMissLayout) {
    initNerve(&NrvSingleModeStateGameOverWait, 0);
    mGraphicsState = new GameOverGraphicsState(mScene);
    mWipeFadeBlack = new al::WipeSimple("黒フェード", "WipeFadeBlack", rLayoutInfo, "Demo");
    mGameOverLayout =
        new al::SimpleLayoutAppear("ゲームオーバーワイプ", "HeadGameOver", rLayoutInfo, nullptr);
    mGameOverMenu = new GameOverMenu(mGameDataHolder);
    mGameOverMenu->init(rLayoutInfo);

    sead::Matrix34f baseMtx;
    baseMtx.setBase(0, {1.0f, 0.0f, 0.0f});
    baseMtx.setBase(1, {0.0f, 1.0f, 0.0f});
    baseMtx.setBase(2, {0.0f, 0.0f, 1.0f});
    baseMtx.setTranslation({0.0f, -10000.0f, 0.0f});
    mDemoHolder = rc::createDemoSceneHolder("DemoGameOverStage", rActorInfo,
                                            rc::createPlayerRetargettingSelector(pScene),
                                            &baseMtx, true, 4);
    mPadRumbleDirector = rActorInfo.getActorSceneInfo().padRumbleDirector;
}

/**
 * Starts the state: stops the music and begins the miss.
 */
void SingleModeStateGameOver::appear() {
    al::NerveStateBase::appear();
    al::stopAllBgm(mScene, -1);
    al::changeAudioEffect(mScene, nullptr);
    al::setNerve(this, &NrvSingleModeStateGameOverMiss);
}

/**
 * Registers a layout to kill when the game over layout appears.
 * @param pLayout The layout to kill.
 */
void SingleModeStateGameOver::entryKillLayout(al::LayoutActor* pLayout) {
    mKillLayouts.pushBack(pLayout);
}

/**
 * Checks whether the player chose to continue.
 * @return Whether continue was decided.
 */
bool SingleModeStateGameOver::isContinue() const {
    if (mGameOverMenu != nullptr) {
        return mGameOverMenu->isDecideContinue();
    }

    return false;
}

/**
 * Checks whether the player chose to quit.
 * @return Whether quit was decided (true when there is no menu).
 */
bool SingleModeStateGameOver::isQuit() const {
    if (mGameOverMenu != nullptr) {
        return mGameOverMenu->isDecideQuit();
    }

    return true;
}

/**
 * Checks whether the game over demo (anything after the miss) is playing.
 * @return Whether the demo is playing.
 */
bool SingleModeStateGameOver::isDemo() const {
    if (al::isNerve(this, &NrvSingleModeStateGameOverWait)) {
        return false;
    }

    return !al::isNerve(this, &NrvSingleModeStateGameOverMiss);
}

/**
 * Updates the pad rumbles.
 */
void SingleModeStateGameOver::control() {
    if (mPadRumbleDirector != nullptr) {
        mPadRumbleDirector->update();
    }
}

/**
 * Kills every registered layout.
 */
void SingleModeStateGameOver::killAllLayouts() {
    for (auto it = mKillLayouts.begin(); it != mKillLayouts.end(); ++it) {
        it->kill();
    }
}

/**
 * Miss: plays the miss and game over jingles, closes the wipe and shows the game over layout.
 */
void SingleModeStateGameOver::exeMiss() {
    if (al::isFirstStep(this)) {
        alSeFunction::stopAllSeWithExceptList(mAudioDirector, "ミス", 0);
        mMissLayout->appear();
        al::stopAllBgm(mScene, 5);
        al::disableBgmStart(mScene);
    }

    if (al::isStep(this, 28)) {
        al::startSequenceBgm(mScene, "Miss", -1, 0);
    }

    if (al::isStep(this, 225)) {
        al::stopAllSequenceBgm(mScene, 5);
        al::changeAudioEffect(mScene, nullptr);
        al::startSequenceBgm(mScene, "GameOver", -1, 0);
    }

    if (al::isStep(this, 119)) {
        mWipe->startClose(-1);
    }

    if (al::isStep(this, 200)) {
        killAllLayouts();
        mGameOverLayout->appear();
    }

    if (al::isGreaterEqualStep(this, 230)) {
        al::setNerve(this, &NrvSingleModeStateGameOverDemoGameOver);
    }
}

/**
 * Game over demo: moves the demo players away and plays the game over demo.
 */
void SingleModeStateGameOver::exeDemoGameOver() {
    mGraphicsState->setGraphicsState();

    if (al::isFirstStep(this) && mDemoHolder != nullptr) {
        al::LiveActor* pDemoActor = mDemoHolder->getDemoSceneActor(0);
        s32 playerNum = al::getPlayerNumMax(pDemoActor);

        for (s32 i = 0; i < playerNum; i++) {
            auto* pPlayer = static_cast<PlayerActor*>(al::getPlayerActor(pDemoActor, i));
            pPlayer->getProperty()->mTrans = {10000.0f, -20000.0f, 10000.0f};
            pPlayer->getProperty()->mVelocity = sead::Vector3f::zero;
            pPlayer->updatePosture();
        }

        rc::tryStartDemo(mDemoHolder);
        mDemoHolder->startAction(0, false);
    }

    if (al::isStep(this, 1)) {
        mWipe->kill();
        mMissLayout->kill();

        if (mDemoHolder == nullptr) {
            al::setNerve(this, &NrvSingleModeStateGameOverGameOverMenu);
            return;
        }
    }

    if (mDemoHolder != nullptr && mDemoHolder->isActionEndCamera(0)) {
        al::setNerve(this, &NrvSingleModeStateGameOverGameOverMenu);
    }
}

/**
 * Game over menu: waits for the player to choose between continue and quit.
 */
void SingleModeStateGameOver::exeGameOverMenu() {
    mGraphicsState->setGraphicsState();

    if (al::isFirstStep(this)) {
        mDemoHolder->startAction(1, false);
        mGameOverMenu->appear();
    }

    if (mGameOverMenu->isAlive()) {
        return;
    }

    if (mGameOverMenu->isDecideContinue()) {
        al::setNerve(this, &NrvSingleModeStateGameOverDemoContinue);
    } else if (mGameOverMenu->isDecideQuit()) {
        al::setNerve(this, &NrvSingleModeStateGameOverDone);
    }
}

/**
 * Continue demo: plays the continue demo.
 */
void SingleModeStateGameOver::exeDemoContinue() {
    mGraphicsState->setGraphicsState();

    if (al::isFirstStep(this)) {
        mDemoHolder->startAction(2, false);
    }

    if (mDemoHolder->isActionEndCamera(2) && al::isGreaterEqualStep(this, 200)) {
        al::setNerve(this, &NrvSingleModeStateGameOverDone);
    }
}

/**
 * Save: writes the save data.
 */
void SingleModeStateGameOver::exeSave() {
    mGraphicsState->setGraphicsState();

    if (al::isFirstStep(this)) {
        mScene->prepareDestroy();
        SaveDataAccessFunction::startSaveDataWriteNoMessage(mGameDataHolder, true);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::setNerve(this, &NrvSingleModeStateGameOverDone);
    }
}

/**
 * Done: fades to black and ends the state.
 */
void SingleModeStateGameOver::exeDone() {
    mGraphicsState->setGraphicsState();

    if (al::isFirstStep(this)) {
        mWipeFadeBlack->startClose(30);
    }

    if (mWipeFadeBlack->isCloseEnd()) {
        kill();
    }
}

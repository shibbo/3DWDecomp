#include "Sequence/ProductStateSingleMode.hpp"
#include "Layout/Switch/CounterCoinInfo.hpp"
#include "Layout/WindowProcessing.hpp"
#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/PhaseBossScene.hpp"
#include "Scene/SingleModeScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/InputUtil.hpp"
#include <nn/oe.h>
#include <thread/seadThread.h>
namespace {
struct ThreadPriorities {
    int sceneInit;
    int normal;
};
const ThreadPriorities cThreadPriorities = {sead::Thread::cDefaultPriority + 3,
                                            sead::Thread::cDefaultPriority};
NERVE_DECL(ProductStateSingleMode, LoadStage);
#define SINGLE_END_NERVE(Name)                                                                     \
    class ProductStateSingleModeNrv##Name : public al::Nerve {                                     \
      public:                                                                                      \
        void execute(al::NerveKeeper* p) const override {                                          \
            p->getParent<ProductStateSingleMode>()->exeEnd();                                      \
        }                                                                                          \
    };
SINGLE_END_NERVE(GameChange)
SINGLE_END_NERVE(GameEnd)
SINGLE_END_NERVE(LoadGame)
SINGLE_END_NERVE(Goal)
SINGLE_END_NERVE(Quit)
#undef SINGLE_END_NERVE
NERVE_DECL(ProductStateSingleMode, EndLoad);
NERVE_DECL(ProductStateSingleMode, Stage);
NERVE_DECL(ProductStateSingleMode, DestroyStage);
NERVE_DECL(ProductStateSingleMode, WaitLoadScreenEnd);
NERVE_DECL(ProductStateSingleMode, RetryStage);
NERVE_DECL(ProductStateSingleMode, SaveAndQuit);
NERVES_MAKE_NOSTRUCT(ProductStateSingleMode, LoadStage, GameChange, GameEnd, LoadGame, EndLoad,
                     Stage, DestroyStage, WaitLoadScreenEnd, Goal, RetryStage, SaveAndQuit, Quit)
} // namespace
ProductStateSingleMode::ProductStateSingleMode(
    ProductSequence* pSequence, ProductStageStartParam* pStartParam, StageWipeKeeper* pWipeKeeper,
    const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
    GameDataHolder* pHolder, ControllerConnectChecker* pChecker,
    al::ScreenCaptureExecutor* pCapture)
    : al::HostStateBase<ProductSequence>("SingleModeState", pSequence), mpStartParam(pStartParam),
      mpConnectChecker(pChecker), mpWipeKeeper(pWipeKeeper),
      mpGamePadSystem(rInfo.mGameSystemInfo->getGamePadSystem()) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), pCapture, nullptr);
    mpCoinCounter = new CounterCoinInfo(rLayoutInfo, pHolder);
    mpSaveWindow = new WindowProcessing(rLayoutInfo, "FileSelect");
}
ProductStateSingleMode::~ProductStateSingleMode() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
}
void ProductStateSingleMode::init() { initNerve(&NrvProductStateSingleModeLoadStage, 0); }
void ProductStateSingleMode::appear() {
    mPhase = -1;
    al::NerveStateBase::appear();
    GameDataFunction::startSingleModeStage(GameDataHolderWriter(getHost()->getGameDataHolder()), -1,
                                           -1);
    al::setNerve(this, &NrvProductStateSingleModeLoadStage);
}
void ProductStateSingleMode::kill() {
    al::NerveStateBase::kill();
    mPhase = -1;
}
bool ProductStateSingleMode::isGameChange() const {
    return al::isNerve(this, &NrvProductStateSingleModeGameChange);
}
bool ProductStateSingleMode::isGameEnd() const {
    return al::isNerve(this, &NrvProductStateSingleModeGameEnd);
}
bool ProductStateSingleMode::isLoadGame() const {
    return al::isNerve(this, &NrvProductStateSingleModeLoadGame);
}
void ProductStateSingleMode::exeLoadStage() {
    if (al::isFirstStep(this))
        createAndStartInitStageScene();
    if (mpWipeKeeper->isActiveNoResultWipe() && !mpWipeKeeper->isCloseEndNoResultWipe())
        return;
    if (al::tryEndSceneInitThread(this)) {
        if (mIsAfterOpeningDemo && al::isLessStep(this, 180))
            return;
        getHost()->requestLoadScreenEnd();
        al::setNerve(this, &NrvProductStateSingleModeEndLoad);
    } else if (getHost()->isLoadLayoutEnd()) {
        switch (mPhase) {
        case 1:
        case 3:
        case 5:
        case 8:
            getHost()->startLoadScreen(false, false);
            break;
        }
    }
}
void ProductStateSingleMode::createAndStartInitStageScene() {
    int phase = mPhase;
    if (phase < 0) {
        phase = SingleModeDataFunction::getUnlockedPhase(
            GameDataHolderAccessor(getHost()->getGameDataHolder()));
        bool ending = SingleModeDataFunction::hasSeenEnding(
            GameDataHolderAccessor(getHost()->getGameDataHolder()));
        mPhase = rc::getNonBossPhase(phase, ending);
        if (mPhase != phase) {
            phase = mPhase;
            SingleModeDataFunction::setUnlockedPhase(
                GameDataHolderWriter(getHost()->getGameDataHolder()), phase);
        }
    }
    if (rc::isBossPhase(phase))
        al::setCustomSceneHeapAlloc(
            reinterpret_cast<al::MemorySceneHeapCustomAlloc*>(&PhaseBossScene::sCustomAlloc));
    else
        al::setCustomSceneHeapAlloc(nullptr);
    const char* name = rc::getSingleModePhaseName(phase);
    al::createSceneHeap(name);
    al::SceneHeapSetter heapSetter;
    al::StringTmp<256> debugName;
    debugName.format("X Stage=%s WorldId=%d StageId=%d", "stage_name", -1, -1);
    mpScene = rc::createPhaseScene(phase);
    mpScene->setSaveWindow(mpSaveWindow);
    al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit, name, 1,
                                 debugName.cstr(), nullptr);
}
void ProductStateSingleMode::exeEndLoad() {
    if (getHost()->isLoadLayoutEnd()) {
        getHost()->offDrawScreenCapture();
        mpGamePadSystem->disableControllerApplet(false);
        getHost()->refreshDebugMenu();
        mpWipeKeeper->tryOpenFadeBlack();
        al::setNerve(this, &NrvProductStateSingleModeStage);
    }
}
void ProductStateSingleMode::exeStage() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
        mpWipeKeeper->tryOpenNoResultWipe();
    }
    if (mpScene->isEnableOpenStartWipe() && mpWipeKeeper->tryOpenStartOrRetryWipe())
        mpScene->setInvalidateInputFrames(15);
    if (!mpScene->isAlive() || mpScene->isGameChange() || mpScene->isLoadGame()) {
        getHost()->requestCaptureTopBottom();
        if (mpScene->isLoadGame())
            mpWipeKeeper->closeNoResultWipeTitle();
        al::setNerve(this, &NrvProductStateSingleModeDestroyStage);
    }
    if (al::isRequestCaptureScreenCover(mpScene)) {
        getHost()->requestCaptureTopBottom();
        return;
    }
    if (al::isOffDrawScreenCover(mpScene)) {
        al::resetCaptureScreenCover(mpScene);
        getHost()->offDrawScreenCapture();
    }
}
void ProductStateSingleMode::exeRetryStage() {
    if (al::isFirstStep(this)) {
        createAndStartInitStageScene();
        mpCoinCounter->appear();
    }
    if (!mpCoinCounter->isAlive() && getHost()->isLoadLayoutEnd())
        getHost()->startLoadScreen(false, true);
    if (al::tryEndSceneInitThread(this) && !mpCoinCounter->isAlive()) {
        getHost()->requestLoadScreenEnd();
        getHost()->offDrawScreenCapture();
        al::setNerve(this, &NrvProductStateSingleModeWaitLoadScreenEnd);
    }
}
void ProductStateSingleMode::exeWaitLoadScreenEnd() {
    if (getHost()->isLoadLayoutEnd())
        al::setNerve(this, &NrvProductStateSingleModeStage);
}
void ProductStateSingleMode::exeDestroyStage() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        if (mpScene->isRestart() ||
            SingleModeDataFunction::isPhaseEnd(GameDataHolderAccessor(mpScene))) {
            if (SingleModeDataFunction::shouldFadeToWhite(
                    GameDataHolderAccessor(getHost()->getGameDataHolder()))) {
                mpWipeKeeper->closeNoResultWipeRestartTitleWhiteFade(true);
                SingleModeDataFunction::setShouldFadeToWhite(
                    GameDataHolderAccessor(getHost()->getGameDataHolder()), false);
            } else
                mpWipeKeeper->closeNoResultWipeRestartTitle();
        } else if (mpScene->isReenterStage())
            mpWipeKeeper->closeNoResultWipeRestartTitle();
        else if (mpScene->isRetire())
            mpWipeKeeper->closeNoResultWipeTitle();
        mpScene->prepareDestroy();
        return;
    }
    bool destroyAll = true;
    if (mpScene->isGoal()) {
        al::setNerve(this, &NrvProductStateSingleModeGoal);
        kill();
    } else if (mpScene->isRestart() || mpScene->isReenterStage() ||
               SingleModeDataFunction::isPhaseEnd(GameDataHolderAccessor(mpScene))) {
        if (!(mpWipeKeeper->isActiveRetryWipe() && mpWipeKeeper->isCloseEndRetryWipe()) &&
            !mpWipeKeeper->isCloseEndNoResultWipe())
            return;
        if (SingleModeDataFunction::isPhaseEnd(GameDataHolderAccessor(mpScene)))
            nn::oe::ExitApplicationAndGoBackToInStoreDemoMenu();
        if (mpScene->isRestart())
            al::setNerve(this, &NrvProductStateSingleModeRetryStage);
        else
            al::setNerve(this, &NrvProductStateSingleModeLoadStage);
        destroyAll = false;
    } else if (mpScene->isRetire()) {
        al::setNerve(this, &NrvProductStateSingleModeSaveAndQuit);
        return;
    } else if (mpScene->isGameChange()) {
        al::setNerve(this, &NrvProductStateSingleModeGameChange);
        kill();
    } else if (mpScene->isGameEnd()) {
        al::setNerve(this, &NrvProductStateSingleModeGameEnd);
        kill();
    } else if (mpScene->isLoadGame()) {
        if (!mpWipeKeeper->isCloseEndNoResultWipe())
            return;
        rc::set2PAssistMode(false, al::isPadTypeJoySingle(al::getMainControllerPort()));
        al::setNerve(this, &NrvProductStateSingleModeLoadGame);
        kill();
    }
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
    al::destroySceneHeap(destroyAll);
}
void ProductStateSingleMode::exeSaveAndQuit() {
    if (al::isFirstStep(this)) {
        GameDataFunction::updatePlayerFigures(GameDataHolderWriter(mpScene), -1);
        SaveDataAccessFunction::startSaveDataWriteNoWindow(getHost()->getGameDataHolder(), false,
                                                           true);
        mpSaveWindow->appearWithSystemMessage("SaveSequence", "WindowProcessing_Save", 0, true);
    }
    if (SaveDataAccessFunction::updateSaveDataAccess(getHost()->getGameDataHolder(), false)) {
        mpSaveWindow->requestClose();
        if (mpSaveWindow->isEnd()) {
            if (mpScene != nullptr) {
                if (getHost()->mCurrentScene == mpScene)
                    getHost()->mCurrentScene = nullptr;
                delete mpScene;
                mpScene = nullptr;
            }
            al::destroySceneHeap(true);
            al::setNerve(this, &NrvProductStateSingleModeQuit);
            kill();
        }
    }
}
void ProductStateSingleMode::exeEnd() {}

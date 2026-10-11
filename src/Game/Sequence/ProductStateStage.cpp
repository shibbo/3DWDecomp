#include "Sequence/ProductStateStage.hpp"
#include "Layout/CounterPlayerInfo.hpp"
#include "Layout/PlayerEntryMini.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutUtil.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/StageScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Sequence/ProductStageStartParam.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderWriter.hpp"
#include <thread/seadThread.h>
namespace {
struct ThreadPriorities {
    int sceneInit;
    int normal;
};
const ThreadPriorities cThreadPriorities = {sead::Thread::cDefaultPriority + 3,
                                            sead::Thread::cDefaultPriority};
NERVE_DECL(ProductStateStage, LoadStage);
#define STAGE_END_NERVE(Name)                                                                      \
    class ProductStateStageNrv##Name : public al::Nerve {                                          \
      public:                                                                                      \
        void execute(al::NerveKeeper* p) const override {                                          \
            p->getParent<ProductStateStage>()->exeEnd();                                           \
        }                                                                                          \
    };
STAGE_END_NERVE(Clear)
STAGE_END_NERVE(ClearWithResult)
STAGE_END_NERVE(ClearWorldWarp)
STAGE_END_NERVE(Retire)
STAGE_END_NERVE(Retry)
STAGE_END_NERVE(GameOver)
STAGE_END_NERVE(GameEnd)
STAGE_END_NERVE(GameChange)
STAGE_END_NERVE(LoadGame)
#undef STAGE_END_NERVE
NERVE_DECL(ProductStateStage, Stage);
NERVE_DECL(ProductStateStage, DestroyStage);
NERVE_DECL(ProductStateStage, RetryStage);
class ProductStateStageNrvReenterStage : public al::Nerve {
  public:
    void execute(al::NerveKeeper* p) const override {
        p->getParent<ProductStateStage>()->exeRetryStage();
    }
};
NERVE_DECL(ProductStateStage, CountAndRetire);
NERVES_MAKE_NOSTRUCT(ProductStateStage, LoadStage, Clear, ClearWithResult, ClearWorldWarp, Retire,
                     Retry, GameOver, GameEnd, GameChange, LoadGame, Stage, DestroyStage,
                     RetryStage, ReenterStage, CountAndRetire)
} // namespace
ProductStateStage::ProductStateStage(ProductSequence* pSequence,
                                     ProductStageStartParam* pStartParam,
                                     StageWipeKeeper* pWipeKeeper,
                                     const al::SequenceInitInfo& rInfo,
                                     const al::LayoutInitInfo& rLayoutInfo, GameDataHolder* pHolder,
                                     ControllerConnectChecker* pChecker,
                                     al::ScreenCaptureExecutor* pCapture)
    : al::HostStateBase<ProductSequence>("ステージステート", pSequence), mpStartParam(pStartParam),
      mpWipeKeeper(pWipeKeeper), mpConnectChecker(pChecker) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), pCapture, nullptr);
    mpPlayerEntry = new PlayerEntryMini(rLayoutInfo, pHolder, nullptr, 1);
    mpPlayerEntry->kill();
    mpPlayerCounter = new CounterPlayerInfo(rLayoutInfo, pHolder);
}
ProductStateStage::~ProductStateStage() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
}
void ProductStateStage::init() { initNerve(&NrvProductStateStageLoadStage, 0); }
void ProductStateStage::appear() {
    al::NerveStateBase::appear();
    al::reinitializeShaders(mpPlayerEntry);
    int world = mpStartParam->getWorldId();
    int stage = mpStartParam->getStageId();
    GameDataFunction::startStage(GameDataHolderWriter(getHost()->getGameDataHolder()), world,
                                 stage);
    al::setNerve(this, &NrvProductStateStageLoadStage);
}
bool ProductStateStage::isClear() const { return al::isNerve(this, &NrvProductStateStageClear); }
bool ProductStateStage::isClearWithResult() const {
    return al::isNerve(this, &NrvProductStateStageClearWithResult);
}
bool ProductStateStage::isClearWorldWarp() const {
    return al::isNerve(this, &NrvProductStateStageClearWorldWarp);
}
bool ProductStateStage::isRetire() const { return al::isNerve(this, &NrvProductStateStageRetire); }
bool ProductStateStage::isRetry() const { return al::isNerve(this, &NrvProductStateStageRetry); }
bool ProductStateStage::isGameOver() const {
    return al::isNerve(this, &NrvProductStateStageGameOver);
}
bool ProductStateStage::isGameEnd() const {
    return al::isNerve(this, &NrvProductStateStageGameEnd);
}
bool ProductStateStage::isGameChange() const {
    return al::isNerve(this, &NrvProductStateStageGameChange);
}
bool ProductStateStage::isLoadGame() const {
    return al::isNerve(this, &NrvProductStateStageLoadGame);
}
void ProductStateStage::exeLoadStage() {
    if (al::isFirstStep(this)) {
        createAndStartInitStageScene();
        mpWipeKeeper->appearStartWipe(getHost()->getGameDataHolder());
        mpWipeKeeper->tryOpenNoResultWipeByCircle();
    }
    al::isStep(this, 20);
    if (mpWipeKeeper->isCloseEndNoResultWipe() && mpWipeKeeper->isWaitStartWipe() &&
        !mpPlayerEntry->isAlive()) {
        int course = GameDataFunction::getLastPlayCourseId(
            GameDataHolderAccessor(getHost()->getGameDataHolder()));
        if (GameDataFunction::isStageKinopioBrigade(
                GameDataHolderAccessor(getHost()->getGameDataHolder()), course)) {
            getHost()->offDrawScreenCapture();
            mpPlayerEntry->setSceneType(PlayerEntryMini::SceneType_KinopioPreStageWipe);
            mpPlayerEntry->appear();
            mpPlayerEntry->startCharacterSelect();
        } else if (!GameDataFunction::isStageKinopioHouse(
                       GameDataHolderAccessor(getHost()->getGameDataHolder()), course) &&
                   !GameDataFunction::isStageKinopioHouseHide(
                       GameDataHolderAccessor(getHost()->getGameDataHolder()), course) &&
                   !GameDataFunction::isStageFairyHouse(
                       GameDataHolderAccessor(getHost()->getGameDataHolder()), course) &&
                   !GameDataFunction::isStageGoldenExpress(
                       GameDataHolderAccessor(getHost()->getGameDataHolder()), course) &&
                   !GameDataFunction::isStageCasinoRoom(
                       GameDataHolderAccessor(getHost()->getGameDataHolder()), course)) {
            getHost()->offDrawScreenCapture();
            mpPlayerEntry->setSceneType(PlayerEntryMini::SceneType_PreStageWipe);
            mpPlayerEntry->appear();
            mpPlayerEntry->startCharacterSelect();
        }
    }
    if (al::tryEndSceneInitThread(this) && mpWipeKeeper->isEnableEndStartWipe() &&
        (!mpPlayerEntry->isAlive() || mpPlayerEntry->isPlayerEntryEnd())) {
        getHost()->offDrawScreenCapture();
        al::setNerve(this, &NrvProductStateStageStage);
    }
}
void ProductStateStage::createAndStartInitStageScene() {
    const char* name = mpStartParam->getStageDataName();
    al::createSceneHeap(name);
    al::SceneHeapSetter heapSetter;
    al::StringTmp<256> command;
    mpStartParam->createCommandString(&command);
    mpScene = new StageScene(mpWipeKeeper);
    al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit, name, 1,
                                 command.cstr(), nullptr);
}
void ProductStateStage::exeStage() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (mpScene->isEnableOpenStartWipe() && mpWipeKeeper->tryOpenStartOrRetryWipe())
        mpScene->setInvalidateInputFrames(15);
    if (!mpWipeKeeper->isActiveStartWipe() && mpPlayerEntry->isAlive())
        mpPlayerEntry->kill();
    if (!mpScene->isAlive() || mpScene->isGameChange() || mpScene->isLoadGame()) {
        if (mpScene->isLoadGame()) {
            al::stopAllBgm(mpScene, 0);
            al::stopAllSequenceBgm(mpScene, 0);
            mpWipeKeeper->closeWipeFadeBlack(-1);
        }
        getHost()->requestCaptureTopBottom();
        al::setNerve(this, &NrvProductStateStageDestroyStage);
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
void ProductStateStage::exeRetryStage() {
    bool reenter = al::isNerve(this, &NrvProductStateStageReenterStage);
    if (al::isFirstStep(this)) {
        createAndStartInitStageScene();
        if (!reenter)
            mpPlayerCounter->appear();
    }
    bool ready = al::tryEndSceneInitThread(this) && !mpPlayerCounter->isAlive();
    if (reenter)
        ready = ready && mpWipeKeeper->isCloseEndRetryWipe();
    if (ready) {
        getHost()->offDrawScreenCapture();
        al::setNerve(this, &NrvProductStateStageStage);
    }
}
void ProductStateStage::exeDestroyStage() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        if (mpScene->isRestartStage() || mpScene->isRestartMysteryBox())
            mpWipeKeeper->closeEndRetryWipe();
        else if (mpScene->isTimeUpMysteryBox())
            mpWipeKeeper->closeRetryWipeMysteryTimeUp();
        else if (mpScene->isReenterStage())
            mpWipeKeeper->closeRetryWipe(true);
        mpScene->prepareDestroy();
        return;
    }
    bool destroyAll = true;
    if (mpScene->isRestartStage() || mpScene->isRestartMysteryBox()) {
        al::setNerve(this, &NrvProductStateStageRetryStage);
        destroyAll = false;
    } else if (mpScene->isTimeUpMysteryBox()) {
        al::setNerve(this, &NrvProductStateStageReenterStage);
        destroyAll = false;
    } else if (mpScene->isReenterStage()) {
        if (!mpWipeKeeper->isCloseEndRetryWipe())
            return;
        al::setNerve(this, &NrvProductStateStageReenterStage);
        destroyAll = false;
    } else if (mpScene->isDeadGoldenExpress())
        al::setNerve(this, &NrvProductStateStageCountAndRetire);
    else if (mpScene->isWorldWarp()) {
        al::setNerve(this, &NrvProductStateStageClearWorldWarp);
        kill();
    } else if (mpScene->isGoal()) {
        if (mpScene->isUseResult())
            al::setNerve(this, &NrvProductStateStageClearWithResult);
        else
            al::setNerve(this, &NrvProductStateStageClear);
        kill();
    } else if (mpScene->isRetire()) {
        al::setNerve(this, &NrvProductStateStageRetire);
        kill();
    } else if (mpScene->isGameOver()) {
        al::setNerve(this, &NrvProductStateStageGameOver);
        kill();
    } else if (mpScene->isGameEnd()) {
        al::setNerve(this, &NrvProductStateStageGameEnd);
        kill();
    } else if (mpScene->isGameChange()) {
        al::setNerve(this, &NrvProductStateStageGameChange);
        kill();
    } else if (mpScene->isLoadGame()) {
        if (!mpWipeKeeper->isCloseEndFadeBlack() || !al::isGreaterEqualStep(this, 60))
            return;
        al::setNerve(this, &NrvProductStateStageLoadGame);
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
void ProductStateStage::exeCountAndRetire() {
    if (al::isFirstStep(this))
        mpPlayerCounter->appear();
    if (!mpPlayerCounter->isAlive()) {
        al::setNerve(this, &NrvProductStateStageRetire);
        kill();
    }
}
void ProductStateStage::exeEnd() {}
void ProductStateStage::exeWaitIntroWipeClose() {
    if (mpWipeKeeper->isCloseEndNoResultWipe())
        al::setNerve(this, &NrvProductStateStageLoadStage);
}

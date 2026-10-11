#include "Sequence/ProductStateTitle.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Scene/TitleScene.hpp"
#include "Scene/TitleSceneFunction.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "Stage/StateResultAndSceneInit.hpp"
#include "System/AssetLoadingThread.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SelectedGameDataFlagFunction.hpp"
#include <thread/seadThread.h>
namespace {
const int cSceneInitPriority = sead::Thread::cDefaultPriority + 3;
NERVE_DECL(ProductStateTitle, Load);
NERVE_DECL(ProductStateTitle, LoadWithResult);
NERVE_DECL(ProductStateTitle, LoadWithWipe);
NERVE_DECL(ProductStateTitle, LoadWithWhiteFade);
NERVE_DECL(ProductStateTitle, LoadWithRestartWipe);
NERVE_DECL(ProductStateTitle, WaitWipeClose);
NERVE_DECL(ProductStateTitle, WipeOpen);
#define TITLE_WIPE_NERVE(Name)                                                                     \
    class ProductStateTitleNrv##Name : public al::Nerve {                                          \
      public:                                                                                      \
        void execute(al::NerveKeeper* p) const override {                                          \
            p->getParent<ProductStateTitle>()->exeWipeOpen();                                      \
        }                                                                                          \
    };
TITLE_WIPE_NERVE(WipeOpenWithWipe)
TITLE_WIPE_NERVE(WipeOpenWithWhiteFade)
TITLE_WIPE_NERVE(WipeOpenWithRestartWipe)
TITLE_WIPE_NERVE(WipeOpenWithResult)
#undef TITLE_WIPE_NERVE
NERVE_DECL(ProductStateTitle, Wait);
NERVE_DECL(ProductStateTitle, Destroy);
NERVE_DECL(ProductStateTitle, End);
const struct {
    ProductStateTitleNrvLoad Main;
    ProductStateTitleNrvLoadWithResult Result;
} NrvProductStateTitleLoad;
NERVES_MAKE_NOSTRUCT(ProductStateTitle, LoadWithWipe, LoadWithWhiteFade, LoadWithRestartWipe,
                     WaitWipeClose, WipeOpen, WipeOpenWithWipe, WipeOpenWithWhiteFade,
                     WipeOpenWithRestartWipe, WipeOpenWithResult, Wait, Destroy, End)
} // namespace
/** @brief Create the title state, result transition and demo rotation. */
ProductStateTitle::ProductStateTitle(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                                     const al::LayoutInitInfo& rLayoutInfo,
                                     StageWipeKeeper* pWipeKeeper,
                                     ProductAsyncResourceLoader* pLoader,
                                     al::ScreenCaptureExecutor* pCapture,
                                     WipeCurtainResult* pResultWipe)
    : al::HostStateBase<ProductSequence>("タイトル", pSequence), mpWipeKeeper(pWipeKeeper),
      mpResourceLoader(pLoader) {
    al::initSceneCreator(
        this, rInfo, reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolderBase()),
        pSequence->getAudioDirector(), pCapture, nullptr);
    mpResultState = new StateResultAndSceneInit(
        this, rInfo, rLayoutInfo, pSequence->getGameDataHolder(), pResultWipe, nullptr, nullptr);
    mpResultState->setForTitleScene();
    mpDemoIds = new int[TitleSceneFunction::getTitleDemoNumAll()];
    for (int i = 0; i < TitleSceneFunction::getTitleDemoNumAll(); ++i)
        mpDemoIds[i] = i;
}
/** @brief Release the title scene and restore normal asset-loading speed. */
ProductStateTitle::~ProductStateTitle() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
    rc::AssetLoadingThread::sInstance->disableFastLoad(false);
}
/** @brief Retain the title state without additional reset work. */
void ProductStateTitle::resetState() {}
/** @brief Initialize title loading and the result-screen child state. */
void ProductStateTitle::init() {
    initNerve(&NrvProductStateTitleLoad.Main, 1);
    al::initNerveState(this, mpResultState, &NrvProductStateTitleLoad.Result, "LoadWithResult");
}
/** @brief Choose and shuffle available title demos before selecting a load transition. */
void ProductStateTitle::appear() {
    mIsLoadAudioPlayed = false;
    al::NerveStateBase::appear();
    GameDataFunction::onStartTitleScene(GameDataHolderWriter(getHost()->getGameDataHolder()));
    mDemoIndex = 0;
    for (int i = 0; i < TitleSceneFunction::getTitleDemoNumAll(); ++i)
        mpDemoIds[i] = i;
    auto* holder = getHost()->getGameDataHolder();
    auto* file = holder->getGameDataFile(holder->getLastPlayingFileId());
    int courseId = file->getLastPlayCourseId();
    bool rosetta = SelectedGameDataFlagFunction::isAlreadyOpenRosetta(file);
    if (courseId > 0) {
        int worldId =
            GameDataFunction::findStageDatabaseInfo(GameDataHolderAccessor(holder), courseId)
                ->getWorldId();
        bool rosettaCourse = GameDataFunction::calcRosettaAppearanceCourseId(
                                 GameDataHolderAccessor(holder)) == courseId;
        if (rosetta && rosettaCourse) {
            mDemoCount = TitleSceneFunction::getTitleDemoNumAll();
            int id = TitleSceneFunction::getTitleDemoIdRosetta();
            mpDemoIds[0] = id;
            mpDemoIds[id] = 0;
        } else if (worldId > 8) {
            mDemoCount = TitleSceneFunction::getTitleDemoNumIncludeSuperPlay();
            int id = TitleSceneFunction::getTitleDemoIdSuperPlay();
            mpDemoIds[0] = id;
            mpDemoIds[id] = 0;
        } else
            mDemoCount = TitleSceneFunction::getTitleDemoNumDefault();
    } else
        mDemoCount = TitleSceneFunction::getTitleDemoNumDefault();
    int count = mDemoCount;
    int* ids = mpDemoIds;
    for (int n = count; n > 1; --n) {
        int idx = al::getRandom(n - 1) + 1;
        int value = ids[n - 1];
        ids[n - 1] = ids[idx];
        ids[idx] = value;
    }
    mIsWaitResultWipe = false;
    if (mpWipeKeeper->isExistActiveClosedNoResultWipe())
        al::setNerve(this, &NrvProductStateTitleLoadWithWipe);
    else if (getHost()->isShowResult())
        al::setNerve(this, &NrvProductStateTitleLoad.Result);
    else if (mIsLoadGame)
        al::setNerve(this, &NrvProductStateTitleLoadWithRestartWipe);
    else
        al::setNerve(this, &NrvProductStateTitleWaitWipeClose);
}
/** @brief Notify game data that the title sequence ended. */
void ProductStateTitle::kill() {
    GameDataFunction::onEndTitleScene(GameDataHolderWriter(getHost()->getGameDataHolder()));
    al::NerveStateBase::kill();
}
/** @brief Wait for the boot wipe before creating the title scene. */
void ProductStateTitle::exeWaitWipeClose() {
    if (mpWipeKeeper->isCloseEndBootWipe() || !mpWipeKeeper->isActiveBootWipe())
        al::setNerve(this, &NrvProductStateTitleLoad.Main);
}
/** @brief Play the title call and music appropriate to save progress. */
void ProductStateTitle::playLoadAudio() {
    mIsLoadAudioPlayed = true;
    auto* holder = getHost()->getGameDataHolder();
    if (GameDataFunction::isAllNewFile(GameDataHolderAccessor(holder)))
        al::startSe(getHost(), "PgTitleCallFirst", nullptr);
    else if (SelectedGameDataFlagFunction::isAlreadyOpenRosetta(
                 holder->getGameDataFile(holder->getLastPlayingFileId())))
        al::startSe(getHost(), "PgTitleCallPlusRosetta", nullptr);
    else
        al::startSe(getHost(), "PgTitleCallNormal", nullptr);
    mpScene->setStartBgm();
    al::startBgm(getHost(), "Title", -1, 118, -1, -1);
}
/** @brief Load the title and start its audio before opening the boot wipe. */
void ProductStateTitle::exeLoad() {
    if (al::isFirstStep(this))
        createTitleScene();
    if (!mIsLoadAudioPlayed && mpScene->isAudioReady())
        playLoadAudio();
    if (al::tryEndSceneInitThread(this) && al::isGreaterEqualStep(this, 0)) {
        if (!mIsLoadAudioPlayed)
            playLoadAudio();
        if (mpWipeKeeper->isCloseEndBootWipe() || !mpWipeKeeper->isActiveBootWipe()) {
            rc::AssetLoadingThread::sInstance->disableFastLoad(true);
            rc::AssetLoadingThread::sInstance->startLoad(
                rc::AssetLoadingThread::LOAD_TYPE_IN_3D_WORLD_TITLE);
            al::setNerve(this, &NrvProductStateTitleWipeOpen);
        }
    }
}
/** @brief Create the next title demo on the scene heap. */
void ProductStateTitle::createTitleScene() {
    al::createSceneHeap(nullptr);
    al::SceneHeapSetter heapSetter;
    mpScene = new TitleScene(mpWipeKeeper, getDemoIdAndSetNextDemoId());
    al::setSceneAndUseInitThread(this, mpScene, cSceneInitPriority, mpScene->getDemoStageName(), -1,
                                 "", nullptr);
}
/** @brief Load a title demo behind the normal return wipe. */
void ProductStateTitle::exeLoadWithWipe() {
    if (al::isFirstStep(this))
        createTitleScene();
    if (al::tryEndSceneInitThread(this) && mpWipeKeeper->isCloseEndNoResultWipe() &&
        !al::isLessEqualStep(this, 60))
        al::setNerve(this, &NrvProductStateTitleWipeOpenWithWipe);
}
/** @brief Load a title demo behind the white restart fade. */
void ProductStateTitle::exeLoadWithWhiteFade() {
    if (al::isFirstStep(this))
        createTitleScene();
    if (al::tryEndSceneInitThread(this) && mpWipeKeeper->isCloseEndNoResultWipe() &&
        !al::isLessEqualStep(this, 60))
        al::setNerve(this, &NrvProductStateTitleWipeOpenWithWhiteFade);
}
/** @brief Load a saved game behind the black restart fade. */
void ProductStateTitle::exeLoadWithRestartWipe() {
    if (al::isFirstStep(this)) {
        createTitleScene();
        mIsWaitResultWipe = true;
    }
    if (al::tryEndSceneInitThread(this) && mpWipeKeeper->isCloseEndFadeBlack() &&
        !al::isLessEqualStep(this, 60))
        al::setNerve(this, &NrvProductStateTitleWipeOpenWithRestartWipe);
}
/** @brief Load the title alongside the stage-clear result screen. */
void ProductStateTitle::exeLoadWithResult() {
    if (al::isFirstStep(this)) {
        createTitleScene();
        mIsWaitResultWipe = true;
    }
    if (al::updateNerveState(this))
        al::setNerve(this, &NrvProductStateTitleWipeOpenWithResult);
}
/** @brief Activate the loaded title and open its selected transition. */
void ProductStateTitle::exeWipeOpen() {
    if (al::isFirstStep(this)) {
        getHost()->offDrawScreenCapture();
        if (al::isNerve(this, &NrvProductStateTitleWipeOpen))
            mpScene->appearFromBoot();
        else if (mIsLoadGame) {
            mpScene->appearFromLoad();
            mIsLoadGame = false;
        } else
            mpScene->appear();
        getHost()->setNextScene(mpScene);
        if (mIsWaitResultWipe)
            mpScene->setAppearWaitSkip(true);
    }
    if (!mpScene->isReadyShow())
        return;
    if (al::isNerve(this, &NrvProductStateTitleWipeOpen))
        mpWipeKeeper->tryOpenBootWipe();
    else if (al::isNerve(this, &NrvProductStateTitleWipeOpenWithWipe))
        mpWipeKeeper->tryOpenNoResultWipeByCircle();
    else if (al::isNerve(this, &NrvProductStateTitleWipeOpenWithWhiteFade))
        mpWipeKeeper->tryOpenNoResultWipe();
    else if (al::isNerve(this, &NrvProductStateTitleWipeOpenWithRestartWipe))
        mpWipeKeeper->tryOpenFadeBlack();
    else
        mpResultState->tryOpenWipeForTitleScene();
    al::setNerve(this, &NrvProductStateTitleWait);
}
/** @brief Wait for title selection, releasing the result-screen hold when ready. */
void ProductStateTitle::exeWait() {
    if (al::isFirstStep(this))
        rc::AssetLoadingThread::sInstance->startLoad(rc::AssetLoadingThread::LOAD_TYPE_SHARED);
    if (mIsWaitResultWipe && mpResultState->isOpenWipeForNextScene()) {
        mIsWaitResultWipe = false;
        mpScene->setAppearWaitSkip(false);
    }
    if (mpScene->isAllPlayerDecideTrigger())
        al::startSe(getHost(), "PgAllPlayerDecided", nullptr);
    if (!mpScene->isAlive()) {
        mIsGotoLuigiBros = mpScene->isLuigiBros();
        al::setNerve(this, &NrvProductStateTitleDestroy);
    }
}
/** @brief Wait for assets, record the next destination and release the title scene. */
void ProductStateTitle::exeDestroy() {
    rc::AssetLoadingThread::sInstance->disableFastLoad(false);
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        if (mpScene->isRestart() && !mpScene->isRestartWhiteFade())
            mpWipeKeeper->closeNoResultWipeRestartTitle();
        return;
    }
    bool restart = mpScene->isRestart();
    bool loaded = rc::AssetLoadingThread::sInstance->isLoadDone(
        rc::AssetLoadingThread::LOAD_TYPE_IN_3D_WORLD_TITLE);
    if (restart) {
        if (!loaded)
            return;
        if (mpScene->isRestartWhiteFade())
            al::setNerve(this, &NrvProductStateTitleLoadWithWhiteFade);
        else
            al::setNerve(this, &NrvProductStateTitleLoadWithWipe);
    } else {
        if (!loaded)
            return;
        al::setNerve(this, &NrvProductStateTitleEnd);
        kill();
    }
    if (mpScene != nullptr) {
        mIsGotoTopMenu = mpScene->isReturnToTopMenu();
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
    al::destroySceneHeap(true);
}
/** @brief Remain idle after the title sequence. */
void ProductStateTitle::exeEnd() {}
/** @brief Return the next demo identifier and advance the rotation. */
int ProductStateTitle::getDemoIdAndSetNextDemoId() {
    int id = mpDemoIds[mDemoIndex];
    if (mDemoIndex >= mDemoCount - 1)
        mDemoIndex = 0;
    else
        ++mDemoIndex;
    return id;
}

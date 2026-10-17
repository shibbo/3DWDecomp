#include "Sequence/ProductStateCourseSelect.hpp"
#include "Layout/WindowProcessing.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Scene/CourseSelectScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "Stage/StateResultAndSceneInit.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/InputUtil.hpp"
#include <heap/seadHeapMgr.h>
#include <thread/seadThread.h>
namespace {
// Keep both priorities together, as in the original sequence thread settings.
struct ThreadPriorities {
    int sceneInit;
    int normal;
};
const ThreadPriorities cThreadPriorities = {sead::Thread::cDefaultPriority + 3,
                                            sead::Thread::cDefaultPriority};
NERVE_DECL(ProductStateCourseSelect, LoadStage);
NERVE_DECL(ProductStateCourseSelect, LoadStageWithResult);
class ProductStateCourseSelectNrvLoadGame : public al::Nerve {
  public:
    void execute(al::NerveKeeper* p) const override {
        p->getParent<ProductStateCourseSelect>()->exeEnd();
    }
};
NERVE_DECL(ProductStateCourseSelect, CourseSelect);
NERVE_DECL(ProductStateCourseSelect, DestroyStage);
NERVE_DECL(ProductStateCourseSelect, Save);
NERVE_DECL(ProductStateCourseSelect, End);
const struct {
    ProductStateCourseSelectNrvLoadStage Main;
    ProductStateCourseSelectNrvLoadStageWithResult Result;
} NrvProductStateCourseSelectLoadStage;
NERVES_MAKE_NOSTRUCT(ProductStateCourseSelect, LoadGame, CourseSelect, DestroyStage, Save, End)
} // namespace
/** @brief Create course-selection, result and save-window services. */
ProductStateCourseSelect::ProductStateCourseSelect(
    ProductSequence* pSequence, ProductStageStartParam* pStartParam, StageWipeKeeper* pWipeKeeper,
    const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
    ControllerConnectChecker* pChecker, al::ScreenCaptureExecutor* pCapture,
    WipeCurtainResult* pResultWipe)
    : al::HostStateBase<ProductSequence>("コース選択ステート", pSequence),
      mpStartParam(pStartParam), mpWipeKeeper(pWipeKeeper), mpConnectChecker(pChecker) {
    al::initSceneCreator(
        this, rInfo, reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolderBase()),
        pSequence->getAudioDirector(), pCapture, nullptr);
    mpResultState =
        new StateResultAndSceneInit(this, rInfo, rLayoutInfo, pSequence->getGameDataHolder(),
                                    pResultWipe, pWipeKeeper, mpConnectChecker);
    mpSaveWindow = new WindowProcessing(rLayoutInfo, "FileSelect");
}
/** @brief Detach and release the owned course-selection scene. */
ProductStateCourseSelect::~ProductStateCourseSelect() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
}
/** @brief Initialize loading and its result-screen child state. */
void ProductStateCourseSelect::init() {
    initNerve(&NrvProductStateCourseSelectLoadStage.Main, 1);
    al::initNerveState(this, mpResultState, &NrvProductStateCourseSelectLoadStage.Result,
                       "LoadWithResult");
}
/** @brief Disable controller connection while loading the course selection. */
void ProductStateCourseSelect::appear() {
    al::NerveStateBase::appear();
    mIsGotoTitle = false;
    rc::setControllerConnectDisabled(true);
    if (getHost()->isShowResult())
        al::setNerve(this, &NrvProductStateCourseSelectLoadStage.Result);
    else
        al::setNerve(this, &NrvProductStateCourseSelectLoadStage.Main);
}
/** @brief Check whether the completed state returns to the title. */
bool ProductStateCourseSelect::isGotoTitle() const { return isDead() && mIsGotoTitle; }
/** @brief Check whether loading is showing stage results. */
bool ProductStateCourseSelect::isResult() const {
    return al::isNerve(this, &NrvProductStateCourseSelectLoadStage.Result);
}
/** @brief Enable result effects after the first step. */
bool ProductStateCourseSelect::isEffectDraw() const {
    if (isResult() && al::isGreaterStep(this, 1))
        return true;
    return false;
}
/** @brief Check whether the transition requests loading a game. */
bool ProductStateCourseSelect::isLoadGame() const {
    return al::isNerve(this, &NrvProductStateCourseSelectLoadGame);
}
/** @brief Load course selection and open its pending wipes. */
void ProductStateCourseSelect::exeLoadStage() {
    if (al::isFirstStep(this)) {
        al::createCourseSelectHeap();
        sead::ScopedCurrentHeapSetter heapSetter(al::getCourseSelectHeap());
        mpScene = new CourseSelectScene(false);
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit,
                                     "CourseSelectStage", -1, "", al::getCourseSelectHeap());
    }
    if (al::tryEndSceneInitThread(this) &&
        (mpWipeKeeper->isCloseEndNoResultWipe() ||
         (mpWipeKeeper->isActiveRetryWipe() && mpWipeKeeper->isCloseEndRetryWipe()))) {
        if (!getHost()->getGameDataHolder()->isFirstPlay())
            mpWipeKeeper->tryOpenNoResultWipe();
        mpWipeKeeper->tryOpenStartOrRetryWipe();
        getHost()->offDrawScreenCapture();
        al::setNerve(this, &NrvProductStateCourseSelectCourseSelect);
    }
}
/** @brief Load course selection alongside stage results. */
void ProductStateCourseSelect::exeLoadStageWithResult() {
    if (al::isFirstStep(this)) {
        al::createCourseSelectHeap();
        sead::ScopedCurrentHeapSetter heapSetter(al::getCourseSelectHeap());
        mpScene = new CourseSelectScene(false);
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit,
                                     "CourseSelectStage", -1, "", al::getCourseSelectHeap());
    }
    if (al::updateNerveState(this)) {
        getHost()->offDrawScreenCapture();
        mpResultState->tryOpenWipeForNextScene();
        al::setNerve(this, &NrvProductStateCourseSelectCourseSelect);
    }
}
/** @brief Handle course selection, title return and screen-cover requests. */
void ProductStateCourseSelect::exeCourseSelect() {
    if (al::isFirstStep(this)) {
        rc::setControllerConnectDisabled(false);
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (getHost()->getGameDataHolder()->isFirstPlay() && al::isStep(this, 4))
        mpWipeKeeper->tryOpenNoResultWipeByFade();
    if (al::isGreaterEqualStep(this, 2))
        mpScene->isControllerChange();
    if (mpScene->isGotoTitle()) {
        getHost()->requestCaptureTopBottom();
        mIsGotoTitle = true;
        al::setNerve(this, &NrvProductStateCourseSelectDestroyStage);
        return;
    }
    if (!mpScene->isAlive() || mpScene->isLoadGame()) {
        if (mpScene->isLoadGame()) {
            mpWipeKeeper->closeWipeFadeBlack(-1);
            al::stopAllBgm(mpScene, 0);
            al::stopAllSequenceBgm(mpScene, 0);
        }
        getHost()->requestCaptureTopBottom();
        mpScene->getSelectedCourseInfo(mpStartParam);
        al::setNerve(this, &NrvProductStateCourseSelectDestroyStage);
        return;
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
/** @brief Record the transition, delete the scene and release its heap. */
void ProductStateCourseSelect::exeDestroyStage() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        return;
    }
    if (mpScene->isLoadGame()) {
        if (!mpWipeKeeper->isCloseEndFadeBlack() || !al::isGreaterEqualStep(this, 60))
            return;
        al::setNerve(this, &NrvProductStateCourseSelectLoadGame);
    } else if (mIsGotoTitle) {
        mpWipeKeeper->closeNoResultWipeTitle();
        al::setNerve(this, &NrvProductStateCourseSelectSave);
    } else
        al::setNerve(this, &NrvProductStateCourseSelectEnd);
    if (!mIsGotoTitle)
        kill();
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
    al::destroyCourseSelectHeap();
}
/** @brief Save course-selection progress and wait for the processing window. */
void ProductStateCourseSelect::exeSave() {
    auto* holder = getHost()->getGameDataHolder();
    if (al::isFirstStep(this)) {
        GameDataFunction::onSaveStartInCourseSelect(GameDataHolderWriter(holder));
        SaveDataAccessFunction::startSaveDataWriteNoMessage(holder, false);
        if (SaveDataAccessFunction::isEnableSave(holder))
            mpSaveWindow->appearWithSystemMessage("SaveSequence", "WindowProcessing_Save", 0, true);
    }
    if (!SaveDataAccessFunction::updateSaveDataAccess(holder, false))
        return;
    mpSaveWindow->requestClose();
    if (mpSaveWindow->isEnd() || !mpSaveWindow->isAlive())
        kill();
}
/** @brief Remain idle after course selection. */
void ProductStateCourseSelect::exeEnd() {}

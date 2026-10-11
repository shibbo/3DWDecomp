#include "Sequence/ProductStateAfterEndingEvent.hpp"

#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Scene/CourseSelectScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StateResultAndSceneInit.hpp"
#include "System/GameDataHolder.hpp"
#include <thread/seadThread.h>

namespace {
// Keep both priorities together, as in the original sequence thread settings.
struct ThreadPriorities {
    int sceneInit;
    int normal;
};
const ThreadPriorities cThreadPriorities = {sead::Thread::cDefaultPriority + 3,
                                            sead::Thread::cDefaultPriority};
NERVE_DECL(ProductStateAfterEndingEvent, Load);
NERVE_DECL(ProductStateAfterEndingEvent, Event);
NERVE_DECL(ProductStateAfterEndingEvent, Destroy);
NERVE_DECL(ProductStateAfterEndingEvent, End);
NERVES_MAKE_NOSTRUCT(ProductStateAfterEndingEvent, Load, Event, Destroy, End)
} // namespace

/**
 * @brief Create the post-ending event state and its result/loading substate.
 * @param pSequence Host providing game-data and audio services.
 * @param pStartParam Destination to update after course selection.
 * @param pWipeKeeper Stage wipe service to retain.
 * @param rInfo Sequence initialization services.
 * @param rLayoutInfo Layout initialization services.
 * @param pCapture Screen capture service.
 * @param pWipe Result curtain wipe.
 */
ProductStateAfterEndingEvent::ProductStateAfterEndingEvent(
    ProductSequence* pSequence, ProductStageStartParam* pStartParam, StageWipeKeeper* pWipeKeeper,
    const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
    al::ScreenCaptureExecutor* pCapture, WipeCurtainResult* pWipe)
    : al::HostStateBase<ProductSequence>("コース選択ステート", pSequence),
      mpStartParam(pStartParam), mpWipeKeeper(pWipeKeeper) {
    al::initSceneCreator(
        this, rInfo, reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolderBase()),
        pSequence->getAudioDirector(), pCapture, nullptr);
    mpResultState = new StateResultAndSceneInit(
        this, rInfo, rLayoutInfo, pSequence->getGameDataHolder(), pWipe, nullptr, nullptr);
}

/** @brief Detach and delete any post-ending event scene still owned by the state. */
ProductStateAfterEndingEvent::~ProductStateAfterEndingEvent() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
}

/** @brief Initialize the post-ending event load step. */
void ProductStateAfterEndingEvent::init() {
    initNerve(&NrvProductStateAfterEndingEventLoad, 1);
    al::initNerveState(this, mpResultState, &NrvProductStateAfterEndingEventLoad, "Load");
}

/** @brief Activate the state and restart post-ending event loading. */
void ProductStateAfterEndingEvent::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateAfterEndingEventLoad);
}

/** @brief Create the post-ending event scene on the scene heap and wait for its initialization. */
void ProductStateAfterEndingEvent::exeLoad() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap("CourseSelectStage");
        al::SceneHeapSetter heapSetter;
        mpScene = new CourseSelectScene(true);
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit,
                                     "CourseSelectStage", -1, "", nullptr);
    }
    if (al::updateNerveState(this)) {
        getHost()->offDrawScreenCapture();
        mpResultState->tryOpenWipeForNextScene();
        al::setNerve(this, &NrvProductStateAfterEndingEventEvent);
    }
}

/** @brief Activate the event and retain the selected course when it finishes. */
void ProductStateAfterEndingEvent::exeEvent() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (!mpScene->isAlive()) {
        getHost()->requestCaptureTopBottom();
        mpScene->getSelectedCourseInfo(mpStartParam);
        al::setNerve(this, &NrvProductStateAfterEndingEventDestroy);
    }
}

/** @brief Detach the current scene, then delete it and release its heap on the next step. */
void ProductStateAfterEndingEvent::exeDestroy() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        return;
    }
    al::setNerve(this, &NrvProductStateAfterEndingEventEnd);
    kill();
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
    al::destroySceneHeap(true);
}

/** @brief Remain idle after post-ending event teardown. */
void ProductStateAfterEndingEvent::exeEnd() {}

/** @brief Check whether the result/loading substate is active. @return Whether loading is active.
 */
bool ProductStateAfterEndingEvent::isResult() {
    return al::isNerve(this, &NrvProductStateAfterEndingEventLoad);
}

/** @brief Check whether result effects should be drawn. @return Whether loading has passed step
 * one. */
bool ProductStateAfterEndingEvent::isEffectDraw() {
    if (al::isNerve(this, &NrvProductStateAfterEndingEventLoad) && al::isGreaterStep(this, 1))
        return true;
    return false;
}

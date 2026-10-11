#include "Sequence/ProductStateTitleSingleMode.hpp"

#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Scene/SingleModeTitleScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StageWipeKeeper.hpp"
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
NERVE_DECL(ProductStateTitleSingleMode, Load);
NERVE_DECL(ProductStateTitleSingleMode, WaitWipeClose);
NERVE_DECL(ProductStateTitleSingleMode, WipeOpen);
NERVE_DECL(ProductStateTitleSingleMode, Wait);
NERVE_DECL(ProductStateTitleSingleMode, Destroy);
NERVE_DECL(ProductStateTitleSingleMode, End);
NERVES_MAKE_NOSTRUCT(ProductStateTitleSingleMode, Load, WaitWipeClose, WipeOpen, Wait, Destroy, End)
} // namespace

/**
 * @brief Create the Fury title state and its scene creator.
 * @param pSequence Host supplying game-data and audio services.
 * @param rInfo Sequence initialization services.
 * @param pWipeKeeper Shared stage wipe service.
 * @param rLayoutInfo Layout services, unused here.
 * @param pResultWipe Result curtain, unused here.
 * @param pLoader Loader for the title resources.
 * @param pCapture Screen capture service.
 */
ProductStateTitleSingleMode::ProductStateTitleSingleMode(ProductSequence* pSequence,
                                                         const al::SequenceInitInfo& rInfo,
                                                         const al::LayoutInitInfo& rLayoutInfo,
                                                         StageWipeKeeper* pWipeKeeper,
                                                         ProductAsyncResourceLoader* pLoader,
                                                         al::ScreenCaptureExecutor* pCapture,
                                                         WipeCurtainResult* pResultWipe)
    : al::HostStateBase<ProductSequence>("TitleSingleMode", pSequence), mpWipeKeeper(pWipeKeeper),
      mpResourceLoader(pLoader) {
    al::initSceneCreator(
        this, rInfo, reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolderBase()),
        pSequence->getAudioDirector(), pCapture, nullptr);
}

/** @brief Detach and delete any Fury title scene still owned by the state. */
ProductStateTitleSingleMode::~ProductStateTitleSingleMode() {
    if (al::isFirstStep(this))
        getHost()->mCurrentScene = nullptr;
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
}

/** @brief Initialize the Fury title load step. */
void ProductStateTitleSingleMode::init() { initNerve(&NrvProductStateTitleSingleModeLoad, 0); }

/** @brief Activate the state and restart Fury title loading. */
void ProductStateTitleSingleMode::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateTitleSingleModeWaitWipeClose);
}

/** @brief Create the Fury title scene on the scene heap and wait for its initialization. */
void ProductStateTitleSingleMode::exeLoad() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpScene = new SingleModeTitleScene(mpWipeKeeper);
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit, nullptr, -1, "",
                                     nullptr);
    }
    if (al::tryEndSceneInitThread(this)) {
        al::setNerve(this, &NrvProductStateTitleSingleModeWipeOpen);
    }
}

/** @brief Wait for the Fury title wipe to close before loading the title. */
void ProductStateTitleSingleMode::exeWaitWipeClose() {
    if (mpWipeKeeper->isCloseEndBootWipe() || !mpWipeKeeper->isActiveBootWipe())
        al::setNerve(this, &NrvProductStateTitleSingleModeLoad);
}

/** @brief Activate the title scene after the transition delay. */
void ProductStateTitleSingleMode::exeWipeOpen() {
    if (al::isFirstStep(this)) {
        getHost()->offDrawScreenCapture();
        getHost()->setNextScene(mpScene);
    }
    if (al::isGreaterEqualStep(this, 120)) {
        mpScene->appear();
        mpWipeKeeper->tryOpenBootWipe();
        al::setNerve(this, &NrvProductStateTitleSingleModeWait);
    }
}

/** @brief Capture the screen when the title scene finishes. */
void ProductStateTitleSingleMode::exeWait() {
    al::isFirstStep(this);
    if (!mpScene->isAlive()) {
        getHost()->requestCaptureTopBottom();
        al::setNerve(this, &NrvProductStateTitleSingleModeDestroy);
    }
}

/** @brief Detach the current scene, record the return destination and release its heap. */
void ProductStateTitleSingleMode::exeDestroy() {
    if (al::isFirstStep(this))
        getHost()->mCurrentScene = nullptr;
    if (mpScene != nullptr) {
        mIsReturnToTopMenu = mpScene->isReturnToTopMenu();
        if (getHost()->mCurrentScene == mpScene)
            getHost()->mCurrentScene = nullptr;
        delete mpScene;
        mpScene = nullptr;
    }
    al::setNerve(this, &NrvProductStateTitleSingleModeEnd);
    kill();
    al::destroySceneHeap(true);
}

/** @brief Remain idle after Fury title teardown. */
void ProductStateTitleSingleMode::exeEnd() {}

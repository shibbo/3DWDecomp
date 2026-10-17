#include "Sequence/ProductStateBoot.hpp"

#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Scene/BootScene.hpp"
#include "Sequence/ProductAsyncResourceLoader.hpp"
#include "Sequence/ProductSequence.hpp"
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
NERVE_DECL(ProductStateBoot, Load);
NERVE_DECL(ProductStateBoot, Wait);
NERVE_DECL(ProductStateBoot, WaitEndBootScene);
NERVE_DECL(ProductStateBoot, Destroy);
NERVE_DECL(ProductStateBoot, End);
NERVES_MAKE_NOSTRUCT(ProductStateBoot, Load, Wait, WaitEndBootScene, Destroy, End)
} // namespace

/**
 * @brief Create the boot state and its scene creator.
 * @param pSequence Host supplying game-data and audio services.
 * @param rInfo Sequence initialization services.
 * @param pWipeKeeper Wipe service, unused by this implementation.
 * @param pLoader Loader for the title resources.
 * @param pCapture Screen capture service.
 */
ProductStateBoot::ProductStateBoot(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                                   StageWipeKeeper* pWipeKeeper,
                                   ProductAsyncResourceLoader* pLoader,
                                   al::ScreenCaptureExecutor* pCapture)
    : al::HostStateBase<ProductSequence>("タイトル", pSequence), mpResourceLoader(pLoader) {
    al::initSceneCreator(
        this, rInfo, reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolderBase()),
        pSequence->getAudioDirector(), pCapture, nullptr);
}

/** @brief Detach and delete any boot scene still owned by the state. */
ProductStateBoot::~ProductStateBoot() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
}

/** @brief Initialize the boot load step. */
void ProductStateBoot::init() { initNerve(&NrvProductStateBootLoad, 0); }

/** @brief Activate the state and restart boot loading. */
void ProductStateBoot::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateBootLoad);
}

/** @brief Create the boot scene on the scene heap and wait for its initialization. */
void ProductStateBoot::exeLoad() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpScene = new BootScene();
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit, nullptr, -1, "",
                                     nullptr);
    }
    if (al::tryEndSceneInitThread(this)) {
        getHost()->offDrawScreenCapture();
        mpScene->appear();
        getHost()->setNextScene(mpScene);
        al::setNerve(this, &NrvProductStateBootWait);
    }
}

/** @brief Wait for save data and title resources before ending the boot scene. */
void ProductStateBoot::exeWait() {
    al::isFirstStep(this);
    if (mpScene->isSaveDataLoaded()) {
        mpResourceLoader->tryStartTitleLoad();
        if (mpResourceLoader->isTitleLoaded() && mpScene->tryEnd()) {
            al::setNerve(this, &NrvProductStateBootWaitEndBootScene);
        }
    }
}

/** @brief Capture the screen after the boot scene finishes. */
void ProductStateBoot::exeWaitEndBootScene() {
    if (!mpScene->isAlive()) {
        getHost()->requestCaptureTopBottom();
        al::setNerve(this, &NrvProductStateBootDestroy);
    }
}

/** @brief Detach the current scene, then delete it and release its heap on the next step. */
void ProductStateBoot::exeDestroy() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        return;
    }
    al::setNerve(this, &NrvProductStateBootEnd);
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

/** @brief Remain idle after boot teardown. */
void ProductStateBoot::exeEnd() {}

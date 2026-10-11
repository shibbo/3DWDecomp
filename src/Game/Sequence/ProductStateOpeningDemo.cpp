#include "Sequence/ProductStateOpeningDemo.hpp"

#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Scene/DemoScene.hpp"
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
NERVE_DECL(ProductStateOpeningDemo, Load);
NERVE_DECL(ProductStateOpeningDemo, Wait);
NERVE_DECL(ProductStateOpeningDemo, Destroy);
NERVE_DECL(ProductStateOpeningDemo, End);
NERVES_MAKE_NOSTRUCT(ProductStateOpeningDemo, Load, Wait, Destroy, End)
} // namespace

/**
 * @brief Create the opening-demo state and its scene creator.
 * @param pSequence Host supplying the game-data and audio services.
 * @param rInfo Sequence initialization services.
 */
ProductStateOpeningDemo::ProductStateOpeningDemo(ProductSequence* pSequence,
                                                 const al::SequenceInitInfo& rInfo)
    : al::HostStateBase<ProductSequence>("オープニングデモ", pSequence) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), nullptr, nullptr);
}

/** @brief Detach and delete any demo scene still owned by the state. */
ProductStateOpeningDemo::~ProductStateOpeningDemo() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
}

/** @brief Initialize the opening-demo load step. */
void ProductStateOpeningDemo::init() { initNerve(&NrvProductStateOpeningDemoLoad, 0); }

/** @brief Activate the state and restart demo loading. */
void ProductStateOpeningDemo::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateOpeningDemoLoad);
}

/** @brief Create the demo scene on the scene heap and wait for its initialization. */
void ProductStateOpeningDemo::exeLoad() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpScene = new DemoScene(false, false);
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit, "DemoOpeningStage",
                                     -1, "", nullptr);
        getHost()->getGameDataHolder()->startOpening();
    }
    if (al::tryEndSceneInitThread(this)) {
        getHost()->offDrawScreenCapture();
        al::setNerve(this, &NrvProductStateOpeningDemoWait);
    }
}

/** @brief Show the demo and request a screen capture when playback finishes. */
void ProductStateOpeningDemo::exeWait() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (!mpScene->isAlive()) {
        getHost()->requestCaptureTopBottom();
        al::setNerve(this, &NrvProductStateOpeningDemoDestroy);
    }
}

/** @brief Detach the current scene, then delete it and release its heap on the next step. */
void ProductStateOpeningDemo::exeDestroy() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
        return;
    }
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
    al::destroySceneHeap(true);
    al::setNerve(this, &NrvProductStateOpeningDemoEnd);
    kill();
}

/** @brief Remain idle after demo teardown. */
void ProductStateOpeningDemo::exeEnd() {}

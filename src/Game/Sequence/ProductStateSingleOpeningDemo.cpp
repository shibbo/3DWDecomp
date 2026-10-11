#include "Sequence/ProductStateSingleOpeningDemo.hpp"

#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Scene/DemoSceneSingleMode.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include <thread/seadThread.h>

namespace {
// Keep both priorities together, as in the original sequence thread settings.
struct ThreadPriorities {
    int sceneInit;
    int normal;
};
const ThreadPriorities cThreadPriorities = {sead::Thread::cDefaultPriority + 3,
                                            sead::Thread::cDefaultPriority};
NERVE_DECL(ProductStateSingleOpeningDemo, Load);
NERVE_DECL(ProductStateSingleOpeningDemo, Wait);
NERVE_DECL(ProductStateSingleOpeningDemo, FadeOut);
NERVE_DECL(ProductStateSingleOpeningDemo, Destroy);
NERVE_DECL(ProductStateSingleOpeningDemo, End);
NERVES_MAKE_NOSTRUCT(ProductStateSingleOpeningDemo, Load, Wait, FadeOut, Destroy, End)
} // namespace

/**
 * @brief Create the Bowser's Fury opening state and scene creator.
 * @param pSequence Host supplying game-data and audio services.
 * @param pWipeKeeper Shared stage wipe service.
 * @param rInfo Sequence initialization services.
 * @param pChecker Controller connection checker to retain.
 */
ProductStateSingleOpeningDemo::ProductStateSingleOpeningDemo(ProductSequence* pSequence,
                                                             StageWipeKeeper* pWipeKeeper,
                                                             const al::SequenceInitInfo& rInfo,
                                                             ControllerConnectChecker* pChecker)
    : al::HostStateBase<ProductSequence>("SingleModeOpening", pSequence),
      mpControllerChecker(pChecker), mpWipeKeeper(pWipeKeeper),
      mpGamePadSystem(rInfo.mGameSystemInfo->getGamePadSystem()) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), nullptr, nullptr);
}

/** @brief Detach and delete any demo scene still owned by the state. */
ProductStateSingleOpeningDemo::~ProductStateSingleOpeningDemo() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
}

/** @brief Initialize the opening-demo load step. */
void ProductStateSingleOpeningDemo::init() { initNerve(&NrvProductStateSingleOpeningDemoLoad, 0); }

/** @brief Activate the state and restart demo loading. */
void ProductStateSingleOpeningDemo::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateSingleOpeningDemoLoad);
}

/** @brief Create the demo scene on the scene heap and wait for its initialization. */
void ProductStateSingleOpeningDemo::exeLoad() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpScene = new DemoSceneSingleMode(true, false);
        al::setSceneAndUseInitThread(this, mpScene, cThreadPriorities.sceneInit,
                                     "DemoSingleModeOpeningStage", -1, "", nullptr);
        getHost()->getGameDataHolder()->startOpening();
        SaveDataAccessFunction::startSaveDataWriteSync(getHost()->getGameDataHolder(), false);
    }
    if (al::tryEndSceneInitThread(this)) {
        mpGamePadSystem->setAssistMode(false, false);
        mpGamePadSystem->forceImmediateDisconnect(false);
        mpWipeKeeper->tryOpenFadeBlack();
        mpGamePadSystem->disableControllerApplet(false);
        getHost()->offDrawScreenCapture();
        getHost()->requestLoadScreenEnd();
        al::setNerve(this, &NrvProductStateSingleOpeningDemoWait);
    }
}

/** @brief Fade the opening near its end, or stop its audio when skipped. */
void ProductStateSingleOpeningDemo::exeWait() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (al::isStep(this, mpScene->getDemoLength() - 90)) {
        mpWipeKeeper->closeWipeFadeBlack(100);
        al::setNerve(this, &NrvProductStateSingleOpeningDemoFadeOut);
    } else if (!mpScene->isAlive()) {
        if (mpScene->isSkipped()) {
            SingleModeDataFunction::setDemoWasCancelled(
                GameDataHolderWriter(getHost()->getGameDataHolder()), true);
            al::stopBgm(mpScene, "Stage", 30, -1);
            alSeFunction::stopAllSe(mpScene->getAudioDirector(), 30, nullptr);
        }
        getHost()->requestCaptureTopBottom();
        mpWipeKeeper->closeWipeFadeBlack(-1);
        al::setNerve(this, &NrvProductStateSingleOpeningDemoFadeOut);
    }
}

/** @brief Start the loading screen after the fade has closed. */
void ProductStateSingleOpeningDemo::exeFadeOut() {
    if (mpWipeKeeper->isCloseEndFadeBlack()) {
        getHost()->startLoadScreen(true, false);
        al::setNerve(this, &NrvProductStateSingleOpeningDemoDestroy);
    }
}

/** @brief Detach the current scene, then delete it and release its heap on the next step. */
void ProductStateSingleOpeningDemo::exeDestroy() {
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
    al::setNerve(this, &NrvProductStateSingleOpeningDemoEnd);
    kill();
}

/** @brief Remain idle after demo teardown. */
void ProductStateSingleOpeningDemo::exeEnd() {}

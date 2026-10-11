#include "Sequence/ProductStateSingleModeEnding.hpp"

#include <thread/seadThread.h>
#include "Scene/DemoSceneSingleMode.hpp"
#include "Sequence/ProductSequence.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Scene/DemoSingleModeEndingScene.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "Sequence/SequenceWindowKeeper.hpp"
#include "Layout/WindowMessage.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Layout/LayoutUtil.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
const int cSceneInitPriority = sead::Thread::cDefaultPriority + 3;
NERVE_DECL(ProductStateSingleModeEnding, LoadEnding);
class ProductStateSingleModeEndingNrvEndFirstTime : public al::Nerve {
public:
    /** @brief Execute the terminal state. @param pKeeper Host nerve keeper. */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<ProductStateSingleModeEnding>()->exeEnd();
    }
};
NERVE_DECL(ProductStateSingleModeEnding, Ending);
NERVE_DECL(ProductStateSingleModeEnding, WipeEnding);
NERVE_DECL(ProductStateSingleModeEnding, LoadCredits);
NERVE_DECL(ProductStateSingleModeEnding, Credits);
NERVE_DECL(ProductStateSingleModeEnding, WipeCredits);
NERVE_DECL(ProductStateSingleModeEnding, Save);
NERVE_DECL(ProductStateSingleModeEnding, Destroy);
NERVE_DECL(ProductStateSingleModeEnding, End);
NERVES_MAKE_NOSTRUCT(ProductStateSingleModeEnding, LoadEnding, EndFirstTime, Ending, WipeEnding, LoadCredits, Credits, WipeCredits, Save, Destroy, End)
}

/**
 * @brief Create the ending state, scene creator, and ending fade.
 * @param pSequence Host supplying game-data and audio services.
 * @param rInfo Sequence initialization services.
 * @param rLayoutInfo Layout initialization services.
 * @param pWipeKeeper Shared stage wipe service.
 * @param pCapture Screen capture service.
 */
ProductStateSingleModeEnding::ProductStateSingleModeEnding(ProductSequence* pSequence,
    const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
    StageWipeKeeper* pWipeKeeper, al::ScreenCaptureExecutor* pCapture)
    : al::HostStateBase<ProductSequence>("SingleModeEndingState", pSequence), mpWipeKeeper(pWipeKeeper) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), pCapture, nullptr);
    mpWipe = new al::WipeSimple("エンディングフェード", "WipeFadeBlack", rLayoutInfo, "2D");
}

/** @brief Detach and delete any demo scene still owned by the state. */
ProductStateSingleModeEnding::~ProductStateSingleModeEnding() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
    if (mpCreditsScene != nullptr) {
        if (getHost()->mCurrentScene == mpCreditsScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpCreditsScene;
        mpCreditsScene = nullptr;
    }
}

/** @brief Initialize the ending load step. */
void ProductStateSingleModeEnding::init() { initNerve(&NrvProductStateSingleModeEndingLoadEnding, 0); }

/** @brief Activate the state and restart demo loading. */
void ProductStateSingleModeEnding::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateSingleModeEndingLoadEnding);
}

/** @brief Create the demo scene on the scene heap and wait for its initialization. */
void ProductStateSingleModeEnding::exeLoadEnding() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpScene = new DemoSceneSingleMode(true, true);
        mpScene->enableCancelAudioOnSkip();
        al::setSceneAndUseInitThread(this, mpScene, cSceneInitPriority, "DemoSingleModeEndingStage",
                                    -1, "", nullptr);
    }
    if (al::tryEndSceneInitThread(this) && mpWipeKeeper->isCloseEndEndingWipe() &&
        al::isGreaterEqualStep(this, 240)) {
        getHost()->offDrawScreenCapture();
        mpWipeKeeper->tryOpenEndingWipe();
        al::setNerve(this, &NrvProductStateSingleModeEndingEnding);
    }
}

/** @brief Play the ending scene and start its music at frame 840. */
void ProductStateSingleModeEnding::exeEnding() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (al::isStep(this, 840))
        al::startBgm(mpScene, "Stage", -1, 0, -1, -1);
    if (!mpScene->isAlive())
        al::setNerve(this, &NrvProductStateSingleModeEndingWipeEnding);
}

/** @brief Fade out and release the ending scene before loading the credits. */
void ProductStateSingleModeEnding::exeWipeEnding() {
    if (al::isFirstStep(this)) {
        getHost()->requestCaptureTopBottom();
        mpWipeKeeper->closeWipeFadeBlack(-1);
    }
    if (!mpWipeKeeper->isCloseEndFadeBlack() || al::isLessStep(this, 60))
        return;
    getHost()->requestCaptureTopBottom();
    if (getHost()->mCurrentScene == mpScene)
        getHost()->mCurrentScene = nullptr;
    delete mpScene;
    mpScene = nullptr;
    al::destroySceneHeap(true);
    al::setNerve(this, &NrvProductStateSingleModeEndingLoadCredits);
}

/** @brief Load the credits scene and wait for the minimum transition duration. */
void ProductStateSingleModeEnding::exeLoadCredits() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpCreditsScene = new DemoSingleModeEndingScene();
        al::setSceneAndUseInitThread(this, mpCreditsScene, cSceneInitPriority,
                                    "DemoSingleModeEndRollStage", -1, "", nullptr);
    }
    if (al::tryEndSceneInitThread(this) && al::isGreaterEqualStep(this, 240)) {
        getHost()->offDrawScreenCapture();
        al::setNerve(this, &NrvProductStateSingleModeEndingCredits);
    }
}

/** @brief Show the credits and wait for them to finish. */
void ProductStateSingleModeEnding::exeCredits() {
    if (al::isFirstStep(this)) {
        mpWipeKeeper->tryOpenFadeBlack();
        mpCreditsScene->appear();
        getHost()->setNextScene(mpCreditsScene);
    }
    if (!mpCreditsScene->isAlive())
        al::setNerve(this, &NrvProductStateSingleModeEndingWipeCredits);
}

/** @brief Wait for the credits fade before saving. */
void ProductStateSingleModeEnding::exeWipeCredits() {
    if (al::isFirstStep(this))
        mpWipe->startClose(-1);
    if (!mpWipe->isCloseEnd() || al::isLessStep(this, 60))
        return;
    al::setNerve(this, &NrvProductStateSingleModeEndingSave);
}

/** @brief Save ending progress and capture the final screen. */
void ProductStateSingleModeEnding::exeSave() {
    if (al::isFirstStep(this)) {
        getHost()->requestCaptureTopBottom();
        SaveDataAccessFunction::startSaveDataWrite(getHost()->getGameDataHolder(), false, -1, false);
    }
    if (SaveDataAccessFunction::updateSaveDataAccess(getHost()->getGameDataHolder(), false)) {
        getHost()->requestCaptureTopBottom();
        al::setNerve(this, &NrvProductStateSingleModeEndingDestroy);
    }
}

/** @brief Detach the current scene, then delete it and release its heap on the next step. */
void ProductStateSingleModeEnding::exeDestroy() {
    if (al::isFirstStep(this)) {
        mpWipeKeeper->resetSceneWipes();
        mpWipe->kill();
        getHost()->mCurrentScene = nullptr;
        return;
    }
    if (mpCreditsScene != nullptr) {
        if (getHost()->mCurrentScene == mpCreditsScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpCreditsScene;
        mpCreditsScene = nullptr;
    }
    al::destroySceneHeap(true);
    kill();
    if (SingleModeDataFunction::hasSeenEnding(
            GameDataHolderAccessor(getHost()->getGameDataHolder())))
        al::setNerve(this, &NrvProductStateSingleModeEndingEnd);
    else
        al::setNerve(this, &NrvProductStateSingleModeEndingEndFirstTime);
}

/** @brief Remain idle after demo teardown. */
void ProductStateSingleModeEnding::exeEnd() {}

/** @brief Check whether the first ending has completed. @return Whether the first-time terminal state is active. */
bool ProductStateSingleModeEnding::isEndFirstTime() {
    return al::isNerve(this, &NrvProductStateSingleModeEndingEndFirstTime);
}

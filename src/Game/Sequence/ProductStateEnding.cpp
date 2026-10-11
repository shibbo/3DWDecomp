#include "Sequence/ProductStateEnding.hpp"

#include <thread/seadThread.h>
#include "Scene/DemoEndingScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/GameDataFlagFunction.hpp"
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
NERVE_DECL(ProductStateEnding, Load);
class ProductStateEndingNrvEndFirstTime : public al::Nerve {
public:
    /** @brief Execute the terminal state. @param pKeeper Host nerve keeper. */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<ProductStateEnding>()->exeEnd();
    }
};
NERVE_DECL(ProductStateEnding, Wait);
NERVE_DECL(ProductStateEnding, Wipe);
NERVE_DECL(ProductStateEnding, Info);
NERVE_DECL(ProductStateEnding, Save);
NERVE_DECL(ProductStateEnding, Destroy);
NERVE_DECL(ProductStateEnding, End);
NERVES_MAKE_NOSTRUCT(ProductStateEnding, Load, EndFirstTime, Wait, Wipe, Info, Save, Destroy, End)
}

/**
 * @brief Create the ending state, scene creator, and ending fade.
 * @param pSequence Host supplying game-data and audio services.
 * @param rInfo Sequence initialization services.
 * @param rLayoutInfo Layout initialization services.
 * @param pWipeKeeper Shared stage wipe service.
 * @param pCapture Screen capture service.
 */
ProductStateEnding::ProductStateEnding(ProductSequence* pSequence,
    const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
    StageWipeKeeper* pWipeKeeper, al::ScreenCaptureExecutor* pCapture)
    : al::HostStateBase<ProductSequence>("エンディングデモ", pSequence), mpWipeKeeper(pWipeKeeper) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), pCapture, nullptr);
    mpWipe = new al::WipeSimple("エンディングフェード", "WipeFadeBlack", rLayoutInfo, "Ending");
}

/** @brief Detach and delete any demo scene still owned by the state. */
ProductStateEnding::~ProductStateEnding() {
    if (mpScene != nullptr) {
        if (getHost()->mCurrentScene == mpScene) {
            getHost()->mCurrentScene = nullptr;
        }
        delete mpScene;
        mpScene = nullptr;
    }
}

/** @brief Initialize the ending load step. */
void ProductStateEnding::init() { initNerve(&NrvProductStateEndingLoad, 0); }

/** @brief Activate the state and restart demo loading. */
void ProductStateEnding::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateEndingLoad);
    al::reinitializeShaders(getHost()->getSequenceWindowKeeper()->getSequenceMessage());
}

/** @brief Create the demo scene on the scene heap and wait for its initialization. */
void ProductStateEnding::exeLoad() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap(nullptr);
        al::SceneHeapSetter heapSetter;
        mpScene = new DemoEndingScene();
        al::setSceneAndUseInitThread(this, mpScene, cSceneInitPriority, "DemoEndRollStage",
                                    -1, "", nullptr);
        getHost()->getGameDataHolder()->startEnding();
    }
    if (al::tryEndSceneInitThread(this) && mpWipeKeeper->isCloseEndEndingWipe() &&
        al::isGreaterEqualStep(this, 240)) {
        getHost()->offDrawScreenCapture();
        mpWipeKeeper->tryOpenEndingWipe();
        al::setNerve(this, &NrvProductStateEndingWait);
    }
}

/** @brief Show the demo and request a screen capture when playback finishes. */
void ProductStateEnding::exeWait() {
    if (al::isFirstStep(this)) {
        mpScene->appear();
        getHost()->setNextScene(mpScene);
    }
    if (!mpScene->isAlive()) {
        al::setNerve(this, &NrvProductStateEndingWipe);
    }
}

/** @brief Wait for the closing fade and a minimum transition duration. */
void ProductStateEnding::exeWipe() {
    if (al::isFirstStep(this))
        mpWipe->startClose(-1);
    if (!mpWipe->isCloseEnd() || al::isLessStep(this, 60))
        return;
    al::setNerve(this, &NrvProductStateEndingInfo);
}

/** @brief Show newly unlocked content messages before saving or leaving. */
void ProductStateEnding::exeInfo() {
    if (al::isFirstStep(this)) {
        const char* pLabel = GameDataFlagFunction::tryGetFirstNewOpenFlagMessageLabelByStageClear(
            GameDataHolderAccessor(getHost()->getGameDataHolder()));
        if (pLabel == nullptr) {
            if (GameDataFlagFunction::isAlreadyShowSpecialWorldOpenDemo(
                    GameDataHolderAccessor(getHost()->getGameDataHolder()))) {
                al::setNerve(this, &NrvProductStateEndingSave);
            } else {
                getHost()->requestCaptureTopBottom();
                al::setNerve(this, &NrvProductStateEndingDestroy);
            }
            return;
        }
        getHost()->requestCaptureTopBottom();
        getHost()->getSequenceWindowKeeper()->appearMessage(pLabel,
            rc::calcPadPortByFirstActiveUser(GameDataHolderAccessor(getHost()->getGameDataHolder())), true);
    }
    if (!getHost()->getSequenceWindowKeeper()->isMessageActive()) {
        GameDataFlagFunction::setFirstAlreadyOpenFlagByStageClear(
            GameDataHolderWriter(getHost()->getGameDataHolder()));
        al::setNerve(this, &NrvProductStateEndingInfo);
    }
}

/** @brief Save ending progress and capture the final screen. */
void ProductStateEnding::exeSave() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWrite(getHost()->getGameDataHolder(), false, -1, false);
        getHost()->requestCaptureTopBottom();
    }
    if (SaveDataAccessFunction::updateSaveDataAccess(getHost()->getGameDataHolder(), false)) {
        getHost()->requestCaptureTopBottom();
        al::setNerve(this, &NrvProductStateEndingDestroy);
    }
}

/** @brief Detach the current scene, then delete it and release its heap on the next step. */
void ProductStateEnding::exeDestroy() {
    if (al::isFirstStep(this)) {
        mpWipe->kill();
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
    kill();
    if (GameDataFlagFunction::isAlreadyShowSpecialWorldOpenDemo(
            GameDataHolderAccessor(getHost()->getGameDataHolder())))
        al::setNerve(this, &NrvProductStateEndingEnd);
    else
        al::setNerve(this, &NrvProductStateEndingEndFirstTime);
}

/** @brief Remain idle after demo teardown. */
void ProductStateEnding::exeEnd() {}

/** @brief Check whether the first ending has completed. @return Whether the first-time terminal state is active. */
bool ProductStateEnding::isEndFirstTime() {
    return al::isNerve(this, &NrvProductStateEndingEndFirstTime);
}

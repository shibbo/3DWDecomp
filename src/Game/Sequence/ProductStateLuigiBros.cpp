#include "Sequence/ProductStateLuigiBros.hpp"

#include <prim/seadSafeString.h>
#include "Sequence/ProductSequence.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(ProductStateLuigiBros, LoadStage);
NERVE_DECL(ProductStateLuigiBros, End);
NERVE_DECL(ProductStateLuigiBros, Stage);
NERVE_DECL(ProductStateLuigiBros, DestroyStage);
NERVES_MAKE_NOSTRUCT(ProductStateLuigiBros, LoadStage, End, Stage, DestroyStage)
}

/**
 * @brief Initialize the Luigi Bros. sequence state and its scene creator.
 * @param pSequence Host sequence supplying game data and audio services.
 * @param pStartParam Stage selection retained by the state.
 * @param rInfo Sequence initialization services.
 * @param rLayoutInfo Unused layout initialization services.
 * @param pHolder Unused; the host sequence supplies the game-data holder.
 * @param pWipeKeeper Wipe controller retained by the state.
 * @param pScreenCaptureExecutor Screen capture service for scene creation.
 * @param pConnectChecker Controller connection checker retained by the state.
 */
ProductStateLuigiBros::ProductStateLuigiBros(
    ProductSequence* pSequence, ProductStageStartParam* pStartParam,
    const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
    GameDataHolder* pHolder, StageWipeKeeper* pWipeKeeper,
    al::ScreenCaptureExecutor* pScreenCaptureExecutor, ControllerConnectChecker* pConnectChecker)
    : al::HostStateBase<ProductSequence>("ProductStateLuigiBros", pSequence),
      mpWipeKeeper(pWipeKeeper), mpStartParam(pStartParam), mpConnectChecker(pConnectChecker) {
    al::initSceneCreator(this, rInfo,
                         reinterpret_cast<al::GameDataHolderBase*>(pSequence->getGameDataHolder()),
                         pSequence->getAudioDirector(), pScreenCaptureExecutor, nullptr);
}

/** @brief Release the state's nerve machinery. */
ProductStateLuigiBros::~ProductStateLuigiBros() = default;

/** @brief Initialize the state machine at the scene-load step. */
void ProductStateLuigiBros::init() {
    initNerve(&NrvProductStateLuigiBrosLoadStage, 0);
}

/** @brief Activate the state and restart its scene-load step. */
void ProductStateLuigiBros::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvProductStateLuigiBrosLoadStage);
}

/**
 * @brief Check whether scene teardown has completed.
 * @return Whether the state machine is at its end step.
 */
bool ProductStateLuigiBros::isEnd() {
    return al::isNerve(this, &NrvProductStateLuigiBrosEnd);
}

/** @brief Prepare the scene heap and wait for any scene initialization thread. */
void ProductStateLuigiBros::exeLoadStage() {
    if (al::isFirstStep(this)) {
        al::createSceneHeap("LuigiBros");
        al::SceneHeapSetter heapSetter;
        sead::FixedSafeString<0x100> command;
    }
    if (al::tryEndSceneInitThread(this)) {
        al::setNerve(this, &NrvProductStateLuigiBrosStage);
    }
}

/** @brief Proceed directly to teardown in this build's disabled Luigi Bros. path. */
void ProductStateLuigiBros::exeStage() {
    al::setNerve(this, &NrvProductStateLuigiBrosDestroyStage);
}

/** @brief Detach the current scene, destroy its heap and finish the state. */
void ProductStateLuigiBros::exeDestroyStage() {
    if (al::isFirstStep(this)) {
        getHost()->mCurrentScene = nullptr;
    }
    al::destroySceneHeap(true);
    al::setNerve(this, &NrvProductStateLuigiBrosEnd);
    kill();
}

/** @brief Remain idle after teardown. */
void ProductStateLuigiBros::exeEnd() {}

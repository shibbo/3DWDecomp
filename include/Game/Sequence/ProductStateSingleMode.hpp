#pragma once

#include "Library/Scene/IUseSceneCreator.hpp"
#include <basis/seadTypes.h>
class SingleModeScene;
class CounterCoinInfo;
class WindowProcessing;
namespace al {
class GamePadSystem;
}
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
class GameDataHolder;
class ProductSequence;
class ProductStageStartParam;
class StageWipeKeeper;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
} // namespace al

/**
 * @brief Sequence state of Bowser's Fury.
 */
class ProductStateSingleMode : public al::HostStateBase<ProductSequence>,
                               public al::IUseSceneCreator {
  public:
    ProductStateSingleMode(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                           StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                           const al::LayoutInitInfo& rLayoutInfo, GameDataHolder* pHolder,
                           ControllerConnectChecker* pConnectChecker,
                           al::ScreenCaptureExecutor* pScreenCaptureExecutor);

    ~ProductStateSingleMode() override;
    void init() override;
    void appear() override;
    void kill() override;
    void exeLoadStage();
    void createAndStartInitStageScene();
    void exeEndLoad();
    void exeStage();
    void exeRetryStage();
    void exeWaitLoadScreenEnd();
    void exeDestroyStage();
    void exeSaveAndQuit();
    void exeEnd();
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }
    bool isGameChange() const;
    bool isGameEnd() const;
    bool isLoadGame() const;

    /**
     * @brief Mark that the game starts right after the opening demo.
     * @param isAfter True when coming from the opening demo.
     */
    void setAfterOpeningDemo(bool isAfter) { mIsAfterOpeningDemo = isAfter; }

  private:
    ProductStageStartParam* mpStartParam;
    ControllerConnectChecker* mpConnectChecker;
    SingleModeScene* mpScene = nullptr;
    StageWipeKeeper* mpWipeKeeper;
    al::GamePadSystem* mpGamePadSystem;
    al::SceneCreator* mpSceneCreator = nullptr;
    bool mIsAfterOpeningDemo = false;
    int mPhase = -1;
    CounterCoinInfo* mpCoinCounter = nullptr;
    WindowProcessing* mpSaveWindow = nullptr;
};

static_assert(sizeof(ProductStateSingleMode) == 0x70);

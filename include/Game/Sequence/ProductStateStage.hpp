#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"
#include <basis/seadTypes.h>
class StageScene;
class PlayerEntryMini;
class CounterPlayerInfo;

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
 * @brief Sequence state of a Super Mario 3D World stage.
 */
class ProductStateStage : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
  public:
    ProductStateStage(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                      StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                      const al::LayoutInitInfo& rLayoutInfo, GameDataHolder* pHolder,
                      ControllerConnectChecker* pConnectChecker,
                      al::ScreenCaptureExecutor* pScreenCaptureExecutor);

    ~ProductStateStage() override;
    void init() override;
    void appear() override;
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }
    void exeLoadStage();
    void createAndStartInitStageScene();
    void exeStage();
    void exeRetryStage();
    void exeDestroyStage();
    void exeCountAndRetire();
    void exeEnd();
    void exeWaitIntroWipeClose();
    bool isRetry() const;
    bool isGameChange() const;
    bool isClear() const;
    bool isClearWithResult() const;
    bool isClearWorldWarp() const;
    bool isGameOver() const;
    bool isGameEnd() const;
    bool isRetire() const;
    bool isLoadGame() const;

  private:
    ProductStageStartParam* mpStartParam;
    StageScene* mpScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    StageWipeKeeper* mpWipeKeeper;
    PlayerEntryMini* mpPlayerEntry = nullptr;
    CounterPlayerInfo* mpPlayerCounter = nullptr;
    ControllerConnectChecker* mpConnectChecker;
    u8 _60[0x10]; ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateStage) == 0x70);

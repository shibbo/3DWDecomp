#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ControllerConnectChecker;
class GameDataHolder;
class ProductSequence;
class ProductStageStartParam;
class StageWipeKeeper;
namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
class Scene;
struct SequenceInitInfo;
}

class ProductStateLuigiBros : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateLuigiBros(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                          const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
                          GameDataHolder* pHolder, StageWipeKeeper* pWipeKeeper,
                          al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                          ControllerConnectChecker* pConnectChecker);
    ~ProductStateLuigiBros() override;
    void init() override;
    void appear() override;
    bool isEnd();
    void exeLoadStage();
    void exeStage();
    void exeDestroyStage();
    void exeEnd();

    /**
     * @brief Get the state-owned scene creator.
     * @return Scene creator initialized by the constructor.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the scene creator installed during initialization.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    al::SceneCreator* mpSceneCreator = nullptr;
    StageWipeKeeper* mpWipeKeeper;
    al::Scene* mpScene = nullptr;
    ProductStageStartParam* mpStartParam;
    ControllerConnectChecker* mpConnectChecker;
};
static_assert(sizeof(ProductStateLuigiBros) == 0x50);

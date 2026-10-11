#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class DemoSceneSingleMode;
class StageWipeKeeper;
class ControllerConnectChecker;
namespace al { struct SequenceInitInfo; class GamePadSystem; }

class ProductStateSingleOpeningDemo : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateSingleOpeningDemo(ProductSequence* pSequence, StageWipeKeeper* pWipeKeeper,
                                  const al::SequenceInitInfo& rInfo, ControllerConnectChecker* pChecker);
    ~ProductStateSingleOpeningDemo() override;
    void init() override;
    void appear() override;
    void exeLoad();
    void exeWait();
    void exeFadeOut();
    void exeDestroy();
    void exeEnd();

    /**
     * @brief Get the opening-demo scene creator.
     * @return Scene creator installed during initialization.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the opening-demo scene creator.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    DemoSceneSingleMode* mpScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    ControllerConnectChecker* mpControllerChecker;
    StageWipeKeeper* mpWipeKeeper;
    al::GamePadSystem* mpGamePadSystem;
};
static_assert(sizeof(ProductStateSingleOpeningDemo) == 0x50);

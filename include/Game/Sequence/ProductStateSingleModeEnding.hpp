#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class DemoSceneSingleMode;
class DemoSingleModeEndingScene;
class StageWipeKeeper;
namespace al { struct SequenceInitInfo; class LayoutInitInfo; class ScreenCaptureExecutor; class WipeSimple; }

class ProductStateSingleModeEnding : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateSingleModeEnding(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                       const al::LayoutInitInfo& rLayoutInfo, StageWipeKeeper* pWipeKeeper,
                       al::ScreenCaptureExecutor* pCapture);
    ~ProductStateSingleModeEnding() override;
    void init() override;
    void appear() override;
    void exeLoadEnding();
    void exeLoadCredits();
    void exeEnding();
    void exeCredits();
    void exeWipeEnding();
    void exeWipeCredits();
    void exeSave();
    bool isEndFirstTime();
    void exeDestroy();
    void exeEnd();

    /**
     * @brief Get the ending scene creator.
     * @return Scene creator installed during initialization.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the ending scene creator.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    DemoSceneSingleMode* mpScene = nullptr;
    DemoSingleModeEndingScene* mpCreditsScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    StageWipeKeeper* mpWipeKeeper;
    al::WipeSimple* mpWipe = nullptr;
};
static_assert(sizeof(ProductStateSingleModeEnding) == 0x50);

#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class DemoEndingScene;
class StageWipeKeeper;
namespace al { struct SequenceInitInfo; class LayoutInitInfo; class ScreenCaptureExecutor; class WipeSimple; }

class ProductStateEnding : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateEnding(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                       const al::LayoutInitInfo& rLayoutInfo, StageWipeKeeper* pWipeKeeper,
                       al::ScreenCaptureExecutor* pCapture);
    ~ProductStateEnding() override;
    void init() override;
    void appear() override;
    void exeLoad();
    void exeWait();
    void exeWipe();
    void exeInfo();
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
    DemoEndingScene* mpScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    StageWipeKeeper* mpWipeKeeper;
    al::WipeSimple* mpWipe = nullptr;
};
static_assert(sizeof(ProductStateEnding) == 0x48);

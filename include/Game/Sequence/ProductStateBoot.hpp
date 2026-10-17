#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class BootScene;
class StageWipeKeeper;
class ProductAsyncResourceLoader;
namespace al { struct SequenceInitInfo; class ScreenCaptureExecutor; }

class ProductStateBoot : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateBoot(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo, StageWipeKeeper* pWipeKeeper,
                     ProductAsyncResourceLoader* pLoader, al::ScreenCaptureExecutor* pCapture);
    ~ProductStateBoot() override;
    void init() override;
    void appear() override;
    void exeLoad();
    void exeWait();
    void exeWaitEndBootScene();
    void exeDestroy();
    void exeEnd();

    /**
     * @brief Get the boot scene creator.
     * @return Scene creator installed during initialization.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the boot scene creator.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    BootScene* mpScene = nullptr;
    void* mUnreconstructed30 = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    ProductAsyncResourceLoader* mpResourceLoader;
};
static_assert(sizeof(ProductStateBoot) == 0x48);

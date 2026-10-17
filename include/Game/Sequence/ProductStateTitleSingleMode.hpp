#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class SingleModeTitleScene;
class StageWipeKeeper;
class WipeCurtainResult;
class ProductAsyncResourceLoader;
namespace al { struct SequenceInitInfo; class LayoutInitInfo; class ScreenCaptureExecutor; }

class ProductStateTitleSingleMode : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateTitleSingleMode(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo, const al::LayoutInitInfo& rLayoutInfo,
                     StageWipeKeeper* pWipeKeeper,
                     ProductAsyncResourceLoader* pLoader, al::ScreenCaptureExecutor* pCapture, WipeCurtainResult* pResultWipe);
    ~ProductStateTitleSingleMode() override;
    void init() override;
    void appear() override;
    void exeLoad();
    void exeWait();
    void exeWaitWipeClose();
    void exeWipeOpen();
    void exeDestroy();
    void exeEnd();

    /**
     * @brief Get the Fury title scene creator.
     * @return Scene creator installed during initialization.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the Fury title scene creator.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    SingleModeTitleScene* mpScene = nullptr;
    void* mUnreconstructed30;
    StageWipeKeeper* mpWipeKeeper;
    al::SceneCreator* mpSceneCreator = nullptr;
    ProductAsyncResourceLoader* mpResourceLoader;
    bool mIsReturnToTopMenu = false;
};
static_assert(sizeof(ProductStateTitleSingleMode) == 0x58);

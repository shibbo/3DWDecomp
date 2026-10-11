#pragma once

#include "Library/Scene/IUseSceneCreator.hpp"
#include <basis/seadTypes.h>
class TitleScene;
class StateResultAndSceneInit;
#include "Library/Nerve/NerveStateBase.hpp"

class ProductAsyncResourceLoader;
class ProductSequence;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
} // namespace al

/**
 * @brief Sequence state of the Super Mario 3D World title screen.
 */
class ProductStateTitle : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
  public:
    ProductStateTitle(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo,
                      const al::LayoutInitInfo& rLayoutInfo, StageWipeKeeper* pWipeKeeper,
                      ProductAsyncResourceLoader* pResourceLoader,
                      al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                      WipeCurtainResult* pWipeCurtainResult);

    ~ProductStateTitle() override;
    void init() override;
    void appear() override;
    void kill() override;
    void resetState();
    void exeWaitWipeClose();
    void playLoadAudio();
    void exeLoad();
    void createTitleScene();
    void exeLoadWithWipe();
    void exeLoadWithWhiteFade();
    void exeLoadWithRestartWipe();
    void exeLoadWithResult();
    void exeWipeOpen();
    void exeWait();
    void exeDestroy();
    void exeEnd();
    int getDemoIdAndSetNextDemoId();
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

    /**
     * @brief Check whether Luigi Bros. was chosen on the title screen.
     * @return True to start Luigi Bros.
     */
    bool isGotoLuigiBros() const { return mIsGotoLuigiBros; }

    /**
     * @brief Check whether the title screen goes back to the top menu.
     * @return True to go back to the top menu.
     */
    bool isGotoTopMenu() const { return mIsGotoTopMenu; }

    /**
     * @brief Request loading the save data when the title opens.
     * @param isLoad True to load the save data.
     */
    void setLoadGame(bool isLoad) { mIsLoadGame = isLoad; }

  private:
    TitleScene* mpScene = nullptr;
    StateResultAndSceneInit* mpResultState;
    StageWipeKeeper* mpWipeKeeper;
    void* mUnreconstructed40;
    al::SceneCreator* mpSceneCreator = nullptr;
    bool mIsGotoLuigiBros = false;
    ProductAsyncResourceLoader* mpResourceLoader;
    int mDemoIndex = 0;
    int mDemoCount = 0;
    int* mpDemoIds = nullptr;
    bool mIsWaitResultWipe = false;
    bool mIsGotoTopMenu = false;
    bool mIsLoadGame = false;
    bool mIsLoadAudioPlayed = false;
};

static_assert(sizeof(ProductStateTitle) == 0x78);

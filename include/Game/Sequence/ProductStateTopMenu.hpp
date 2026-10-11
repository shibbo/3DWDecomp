#pragma once

#include "Library/Scene/IUseSceneCreator.hpp"
#include <basis/seadTypes.h>
class TopMenuScene;
class StateResultAndSceneInit;
namespace al {
class AudioSystemInfo;
}
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
class ProductAsyncResourceLoader;
class ProductSequence;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
} // namespace al

/// What the top menu is entered from, which decides how it opens.
enum class ProductTopMenuEntry : s32 {
  Default = 0,          ///< Boot, title or a game exit.
  SingleMode = 1,       ///< Leaving Bowser's Fury.
  SingleModeEnding = 2, ///< After the Bowser's Fury ending.
  StageClear = 3,       ///< After clearing a stage.
};

/**
 * @brief Sequence state showing the top menu (game selection).
 */
class ProductStateTopMenu : public al::HostStateBase<ProductSequence>,
                            public al::IUseSceneCreator {
public:
  ProductStateTopMenu(ProductSequence *pSequence,
                      const al::SequenceInitInfo &rInfo,
                      const al::LayoutInitInfo &rLayoutInfo,
                      StageWipeKeeper *pWipeKeeper,
                      ProductAsyncResourceLoader *pResourceLoader,
                      al::ScreenCaptureExecutor *pScreenCaptureExecutor,
                      ControllerConnectChecker *pConnectChecker,
                      WipeCurtainResult *pWipeCurtainResult);

  ~ProductStateTopMenu() override;
  void init() override;
  void appear() override;
  void exeLoad();
  void exeWait();
  void exeWaitLoadTitle();
  void exeWipeClose();
  void exeDestroy();
  void exeEnd();
  al::SceneCreator *getSceneCreator() const override { return mpSceneCreator; }
  void setSceneCreator(al::SceneCreator *pCreator) override {
    mpSceneCreator = pCreator;
  }
  bool isEffectDraw() const;

  /**
   * @brief Set what the top menu is entered from.
   * @param entry The entry kind.
   */
  void setEntry(ProductTopMenuEntry entry) { mEntry = entry; }

private:
  al::SceneCreator *mpSceneCreator = nullptr;
  TopMenuScene *mpScene = nullptr;
  StageWipeKeeper *mpWipeKeeper;
  ProductAsyncResourceLoader *mpResourceLoader;
  bool mIsFirstAppear = true;
  ProductTopMenuEntry mEntry = ProductTopMenuEntry::Default;
  al::AudioSystemInfo *mpAudioSystemInfo = nullptr;
  StateResultAndSceneInit *mpResultState;
};

static_assert(sizeof(ProductStateTopMenu) == 0x60);

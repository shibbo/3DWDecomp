#include "Sequence/ProductStateTopMenu.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Project/Audio/System/AudioSystem.hpp"
#include "Scene/TopMenuScene.hpp"
#include "Sequence/ProductSequence.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "Stage/StateResultAndSceneInit.hpp"
#include "System/Application.hpp"
#include "System/AssetLoadingThread.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/GameDataHolder.hpp"
#include "Util/InputUtil.hpp"
#include <nn/oe.h>
#include <thread/seadThread.h>
namespace {
const int cSceneInitPriority = sead::Thread::cDefaultPriority + 3;
NERVE_DECL(ProductStateTopMenu, Load);
class ProductStateTopMenuNrvLoadWithResult : public al::Nerve {
public:
  void execute(al::NerveKeeper *p) const override {
    p->getParent<ProductStateTopMenu>()->exeLoad();
  }
};
NERVE_DECL(ProductStateTopMenu, Wait);
NERVE_DECL(ProductStateTopMenu, WaitLoadTitle);
class ProductStateTopMenuNrvWipeCloseSingleMode : public al::Nerve {
public:
  void execute(al::NerveKeeper *p) const override {
    p->getParent<ProductStateTopMenu>()->exeWipeClose();
  }
};
class ProductStateTopMenuNrvWipeCloseTitle : public al::Nerve {
public:
  void execute(al::NerveKeeper *p) const override {
    p->getParent<ProductStateTopMenu>()->exeWipeClose();
  }
};
NERVE_DECL(ProductStateTopMenu, Destroy);
NERVE_DECL(ProductStateTopMenu, End);
const struct {
  ProductStateTopMenuNrvLoad Load;
  ProductStateTopMenuNrvLoadWithResult LoadWithResult;
} NrvProductStateTopMenu;
NERVES_MAKE_NOSTRUCT(ProductStateTopMenu, WaitLoadTitle, WipeCloseSingleMode,
                     WipeCloseTitle, End)
NERVES_MAKE_NOSTRUCT(ProductStateTopMenu, Wait, Destroy)
} // namespace
/** @brief Create the game-selection state and result transition. */
ProductStateTopMenu::ProductStateTopMenu(
    ProductSequence *pSequence, const al::SequenceInitInfo &rInfo,
    const al::LayoutInitInfo &rLayoutInfo, StageWipeKeeper *pWipeKeeper,
    ProductAsyncResourceLoader *pLoader, al::ScreenCaptureExecutor *pCapture,
    ControllerConnectChecker *pChecker, WipeCurtainResult *pResultWipe)
    : al::HostStateBase<ProductSequence>("TopMenu", pSequence),
      mpWipeKeeper(pWipeKeeper), mpResourceLoader(pLoader) {
  al::initSceneCreator(this, rInfo,
                       reinterpret_cast<al::GameDataHolderBase *>(
                           pSequence->getGameDataHolderBase()),
                       pSequence->getAudioDirector(), pCapture, nullptr);
  mpAudioSystemInfo = static_cast<al::AudioSystem *>(rInfo.mGameSystemInfo->_0)
                          ->getAudioSystemInfo();
  mpResultState = new StateResultAndSceneInit(
      this, rInfo, rLayoutInfo, pSequence->getGameDataHolder(), pResultWipe,
      pWipeKeeper, pChecker);
}
/** @brief Detach and release the owned top-menu scene. */
ProductStateTopMenu::~ProductStateTopMenu() {
  if (mpScene != nullptr) {
    if (getHost()->mCurrentScene == mpScene)
      getHost()->mCurrentScene = nullptr;
    delete mpScene;
    mpScene = nullptr;
  }
}
/** @brief Initialize the load step and its result-screen child state. */
void ProductStateTopMenu::init() {
  initNerve(&NrvProductStateTopMenu.Load, 1);
  al::initNerveState(this, mpResultState,
                     &NrvProductStateTopMenu.LoadWithResult, "LoadWithResult");
}
/** @brief Choose normal loading or the stage-clear result transition. */
void ProductStateTopMenu::appear() {
  al::NerveStateBase::appear();
  if (mEntry == ProductTopMenuEntry::StageClear)
    al::setNerve(this, &NrvProductStateTopMenu.LoadWithResult);
  else
    al::setNerve(this, &NrvProductStateTopMenu.Load);
}
/** @brief Load the top menu and activate it after pending wipes close. */
void ProductStateTopMenu::exeLoad() {
  if (al::isFirstStep(this)) {
    al::createSceneHeap(nullptr);
    al::SceneHeapSetter heapSetter;
    mpScene = new TopMenuScene(mpWipeKeeper);
    al::setSceneAndUseInitThread(this, mpScene, cSceneInitPriority, nullptr, -1,
                                 "", nullptr);
    if (al::isNerve(this, &NrvProductStateTopMenu.Load))
      rc::setTopMenuPlayerMode();
  }
  if (al::isNerve(this, &NrvProductStateTopMenu.Load)) {
    if (!al::tryEndSceneInitThread(this))
      return;
  } else if (!al::updateNerveState(this))
    return;
  if (mpWipeKeeper->isActiveNoResultWipe() &&
      !mpWipeKeeper->isCloseEndNoResultWipe())
    return;
  if (mpWipeKeeper->isActiveRetryWipe() && !mpWipeKeeper->isCloseEndRetryWipe())
    return;
  getHost()->offDrawScreenCapture();
  mpScene->appear(static_cast<TopMenuAppearState>(mEntry), mIsFirstAppear);
  mIsFirstAppear = false;
  getHost()->setNextScene(mpScene);
  if (al::isNerve(this, &NrvProductStateTopMenu.LoadWithResult))
    rc::setTopMenuPlayerMode();
  al::setNerve(this, &NrvProductStateTopMenuWait);
}
/** @brief Release previous resources and wait for the selected game. */
void ProductStateTopMenu::exeWait() {
  auto *loader = rc::AssetLoadingThread::sInstance;
  if (al::isFirstStep(this)) {
    loader->cancelAndDelete3DWorldStationed();
    loader->disableFastLoad(false);
    loader->startLoad(rc::AssetLoadingThread::LOAD_TYPE_SHARED);
    static_cast<al::GameFrameworkNx *>(Application::instance()->getFramework())
        ->mIsEnableDisplayStart = true;
    nn::oe::FinishStartupLogo();
  }
  if (mpScene->isSelectAny()) {
    bool isTitle = mpScene->isSelect3DWorld();
    getHost()->setGotoTitleFromTopMenu(isTitle);
    if (isTitle)
      al::setNerve(this, &NrvProductStateTopMenuWaitLoadTitle);
    else
      al::setNerve(this, &NrvProductStateTopMenuWipeCloseSingleMode);
  }
}
/** @brief Load 3D World title resources after shared assets finish. */
void ProductStateTopMenu::exeWaitLoadTitle() {
  auto *loader = rc::AssetLoadingThread::sInstance;
  if (al::isFirstStep(this)) {
    loader->waitLoadDone(rc::AssetLoadingThread::LOAD_TYPE_SHARED);
    rc::AssetLoadingThread::sInstance->cancelAndDeleteSingleModeStationed();
    loader->startLoad(rc::AssetLoadingThread::LOAD_TYPE_PRE_3D_WORLD_TITLE);
  }
  al::setNerve(this, &NrvProductStateTopMenuWipeCloseTitle);
}
/** @brief Close the selected game wipe and prepare its loading screen. */
void ProductStateTopMenu::exeWipeClose() {
  bool single = al::isNerve(this, &NrvProductStateTopMenuWipeCloseSingleMode);
  bool first = al::isFirstStep(this);
  if (single) {
    if (first) {
      rc::AssetLoadingThread::sInstance->startLoad(
          rc::AssetLoadingThread::LOAD_TYPE_SINGLE_MODE);
      if (mpScene->isStartSingleMode())
        mpWipeKeeper->closeNoResultWipeTitle();
      else
        mpWipeKeeper->closeNoResultWipeRestartTitle();
    }
    if (!mpWipeKeeper->isCloseEndNoResultWipe())
      return;
    rc::set2PAssistMode(false,
                        al::isPadTypeJoySingle(al::getMainControllerPort()));
    if (getHost()->getGameDataHolder()->getSingleFile()->isNewFile())
      getHost()->startLoadScreen(true, true);
    else
      getHost()->startLoadScreen(false, false);
  } else {
    if (first)
      mpWipeKeeper->closeBootWipe();
    if (!mpWipeKeeper->isCloseEndBootWipe())
      return;
  }
  al::setNerve(this, &NrvProductStateTopMenuDestroy);
}
/** @brief Wait for asset loading, attach audio pools and release the scene. */
void ProductStateTopMenu::exeDestroy() {
  if (al::isFirstStep(this)) {
    getHost()->mCurrentScene = nullptr;
    if (!rc::AssetLoadingThread::sInstance->isAllLoadingDone())
      al::setCpuBoost(true, false);
    return;
  }
  if (!rc::AssetLoadingThread::sInstance->isAllLoadingDone())
    return;
  al::setNerve(this, &NrvProductStateTopMenuEnd);
  if (mpAudioSystemInfo != nullptr) {
    alAudioSystemFunction::tryAttachMemoryPool(
        al::UMF_SE_STATIONED_1ST, mpAudioSystemInfo->getSeadAudioPlayerForSe());
    alAudioSystemFunction::tryAttachMemoryPool(
        al::UMF_SE_STATIONED_2ND, mpAudioSystemInfo->getSeadAudioPlayerForSe());
    alAudioSystemFunction::tryAttachMemoryPool(
        al::UMF_BGM_STATIONED_2ND,
        mpAudioSystemInfo->getSeadAudioPlayerForBgm());
  }
  kill();
  if (mpScene != nullptr) {
    if (getHost()->mCurrentScene == mpScene)
      getHost()->mCurrentScene = nullptr;
    delete mpScene;
    mpScene = nullptr;
  }
  al::destroySceneHeap(true);
}
/** @brief Remain idle after the top-menu transition. */
void ProductStateTopMenu::exeEnd() {}
/** @brief Check whether the result transition has reached its effect-drawing
 * step. */
bool ProductStateTopMenu::isEffectDraw() const {
  return al::isNerve(this, &NrvProductStateTopMenu.LoadWithResult) &&
         al::isGreaterStep(this, 10);
}

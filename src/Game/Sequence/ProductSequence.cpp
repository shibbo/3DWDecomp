#include "Sequence/ProductSequence.hpp"

#include <agl/common/aglRenderBuffer.h>
#include <agl/common/aglRenderTarget.h>
#include <gfx/seadGraphicsContext.h>
#include <gfx/seadViewport.h>
#include <nn/os.h>
#include <thread/seadThread.h>
#include "Course/LoadingLayout.hpp"
#include "Layout/WipeCurtainResult.hpp"
#include "Library/Application/ApplicationMessageReceiver.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/Layout/LayoutUtil.hpp"
#include "Library/Memory/HeapUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Library/System/SystemKit.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/PhaseBossScene.hpp"
#include "Sequence/ProductAsyncResourceLoader.hpp"
#include "Sequence/ProductStageStartParam.hpp"
#include "Sequence/ProductStateAfterEndingEvent.hpp"
#include "Sequence/ProductStateBoot.hpp"
#include "Sequence/ProductStateCourseSelect.hpp"
#include "Sequence/ProductStateEnding.hpp"
#include "Sequence/ProductStateLuigiBros.hpp"
#include "Sequence/ProductStateSingleMode.hpp"
#include "Sequence/ProductStateSingleModeEnding.hpp"
#include "Sequence/ProductStateSingleOpeningDemo.hpp"
#include "Sequence/ProductStateStage.hpp"
#include "Sequence/ProductStateTitle.hpp"
#include "Sequence/ProductStateTopMenu.hpp"
#include "Sequence/SequenceWindowKeeper.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Application.hpp"
#include "System/AssetLoadingThread.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/ControllerConnectChecker.hpp"

namespace {
NERVE_DECL(ProductSequence, TopMenu)
NERVE_DECL(ProductSequence, Boot)
NERVE_DECL(ProductSequence, Title)
NERVE_DECL(ProductSequence, SingleModeOpeningDemo)
NERVE_DECL(ProductSequence, SingleMode)
NERVE_DECL(ProductSequence, SingleModeEnding)
NERVE_DECL(ProductSequence, CourseSelect)
NERVE_DECL(ProductSequence, Stage)
NERVE_DECL(ProductSequence, Ending)
NERVE_DECL(ProductSequence, AfterEndingEvent)
NERVE_DECL(ProductSequence, LuigiBros)
NERVES_MAKE_STRUCT(ProductSequence, TopMenu, Boot, Title, SingleModeOpeningDemo, SingleMode,
                     SingleModeEnding, CourseSelect, Stage, Ending, AfterEndingEvent, LuigiBros)

/** Number of nerves of the product sequence. */
constexpr s32 cNerveNum = 11;

/** Extra scene resource heap size needed by the final Bowser's Fury phase stage. */
constexpr s32 cFinalPhaseResourceSizeExtra = 0x6e00000;

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}
}  // namespace

/**
 * Gets the last Bowser's Fury phase unlocked in the played file.
 * @return The unlocked phase.
 */
inline s32 ProductSequence::getUnlockedSingleModePhase() const {
    return SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder));
}

/**
 * Constructs the product sequence and registers it as the stage size adjuster.
 * @param pName Name of the sequence.
 */
ProductSequence::ProductSequence(const char* pName) : al::Sequence(pName) {
    PhaseBossScene::sCustomAlloc.setDisabled(false);
    al::setStageSizeAdjuster(this);
}

/**
 * Adjusts the scene resource heap size of a stage.
 * @param pStageName Name of the stage.
 * @param size Default resource heap size.
 * @param pAlloc Custom scene heap allocator of the stage, or nullptr.
 * @return The adjusted resource heap size.
 */
s32 ProductSequence::adjustStageSize(const char* pStageName, s32 size,
                                     al::MemorySceneHeapCustomAlloc* pAlloc) {
    if (pAlloc != nullptr) {
        return pAlloc->adjustStageResourceSize(size);
    }

    if (al::isEqualString(pStageName, rc::getSingleModePhaseName(7))) {
        if (getUnlockedSingleModePhase() == 7 || getUnlockedSingleModePhase() == 10) {
            size += cFinalPhaseResourceSizeExtra;
        }
    }

    return size;
}

/**
 * Cancels the stationed asset loads and waits until the loading thread is idle.
 */
void ProductSequence::deleteStationedAssets() {
    rc::AssetLoadingThread* thread = rc::AssetLoadingThread::sInstance;
    thread->cancelAndDelete3DWorldStationed();
    thread->cancelAndDeleteSingleModeStationed();

    while (!thread->isAllLoadingDone()) {
        nn::os::SleepThread(nn::TimeSpan::FromMilliSeconds(5));
    }
}

/**
 * Destroys the product sequence and the states it owns.
 */
ProductSequence::~ProductSequence() {
    al::setStageSizeAdjuster(nullptr);

    rc::AssetLoadingThread* thread = rc::AssetLoadingThread::sInstance;
    bool isDeletedAssets = false;

    if (thread != nullptr && !thread->isAllLoadingDone()) {
        deleteStationedAssets();
        isDeletedAssets = true;
    }

    delete mStateTopMenu;
    delete mStateTitle;
    delete mStateSingleOpeningDemo;
    delete mStateSingleMode;
    delete mStateSingleModeEnding;
    delete mStateCourseSelect;
    delete mStateStage;
    delete mStateEnding;
    delete mStateAfterEndingEvent;
    delete mLayoutKit;
    delete mScreenCaptureExecutor;
    delete mAsyncResourceLoader;
    mGameSystemInfo->getGamePadSystem()->setDelegate(nullptr);

    if (thread != nullptr && !isDeletedAssets) {
        deleteStationedAssets();
    }
}

/**
 * Checks whether the sequence can be destroyed.
 * @return Always true.
 */
bool ProductSequence::isDisposable() const {
    return true;
}

/**
 * Refreshes the debug menu (empty in the release build).
 */
void ProductSequence::refreshDebugMenu() {}

/**
 * Gets the game data holder.
 * @return The game data holder.
 */
GameDataHolder* ProductSequence::getGameDataHolderBase() const {
    return mGameDataHolder;
}

/**
 * Initializes the sequence: game data, layouts, screen captures and every sequence state.
 * @param rInfo Sequence init info.
 */
void ProductSequence::init(const al::SequenceInitInfo& rInfo) {
    al::setStageSizeAdjuster(this);
    const al::GameSystemInfo* systemInfo = rInfo.mGameSystemInfo;
    mGameSystemInfo = systemInfo;
    mGameDataHolder = new GameDataHolder(systemInfo->getNetworkSystem());
    PhaseBossScene::sCustomAlloc.setGameDataHolder(mGameDataHolder);
    mStageStartParam = new ProductStageStartParam(mGameDataHolder);
    mStageStartParam->init();
    initDrawSystemInfo(rInfo);
    initAudio(*mGameSystemInfo, nullptr, 30, 20, 1, "ProductSequence");
    al::initSceneCreator(this, rInfo, reinterpret_cast<al::GameDataHolderBase*>(mGameDataHolder),
                         mAudioDirector, mScreenCaptureExecutor, nullptr);

    al::LayoutInitInfo layoutInitInfo;
    mLayoutKit = new al::LayoutKit(mGameSystemInfo->getFontHolder());
    mLayoutKit->createExecuteDirector(0x80);
    mLayoutKit->setEffectSystem(mGameSystemInfo->getEffectSystem());
    mLayoutKit->setLayoutSystem(mGameSystemInfo->getLayoutSystem());
    mLayoutKit->setDrawContext(getFramework()->mDrawContext);
    al::initLayoutInitInfo(&layoutInitInfo, mLayoutKit, nullptr, mAudioDirector,
                           mGameSystemInfo->getLayoutSystem(), mGameSystemInfo->getMessageSystem(),
                           mGameSystemInfo->getGamePadSystem());

    mStageWipeKeeper = new StageWipeKeeper(layoutInitInfo, mGameSystemInfo->getNetworkSystem());
    mSequenceWindowKeeper = new SequenceWindowKeeper(layoutInitInfo);
    mWipeCurtainResult =
        new WipeCurtainResult(layoutInitInfo, mGameDataHolder, nullptr, nullptr, nullptr);
    mLoadingLayout = new LoadingLayout(layoutInitInfo, mGameDataHolder);
    mViewportTop = new sead::Viewport(*getFramework()->getMethodFrameBuffer(6));
    mViewportBottom = new sead::Viewport(*getFramework()->getMethodFrameBuffer(9));

    mScreenCaptureExecutor = new al::ScreenCaptureExecutor(3);
    const agl::RenderTargetColor* dockedColor =
        getFramework()->mDockedRenderBuffer->getRenderTargetColor();
    mScreenCaptureExecutor->createScreenCapture(dockedColor->getMipWidth(0),
                                                dockedColor->getMipHeight(0), 0);
    const agl::RenderTargetColor* handheldColor =
        getFramework()->mHandheldRenderBuffer->getRenderTargetColor();
    mScreenCaptureExecutor->createScreenCapture(handheldColor->getMipWidth(0),
                                                handheldColor->getMipHeight(0), 1);
    handheldColor = getFramework()->mHandheldRenderBuffer->getRenderTargetColor();
    mScreenCaptureExecutor->createScreenCapture(handheldColor->getMipWidth(0),
                                                handheldColor->getMipHeight(0), 2);
    mScreenCaptureExecutor->initBlur(1, *getFramework()->getCurrentRenderBuffer(), *mViewportTop,
                                     agl::utl::ImageFilter2D::cReduceScale_4);
    mScreenCaptureExecutor->initBlur(2, *getFramework()->getCurrentRenderBuffer(), *mViewportTop,
                                     agl::utl::ImageFilter2D::cReduceScale_4);

    mControllerConnectChecker = new ControllerConnectChecker(
        mGameDataHolder, mGameSystemInfo->getApplicationMessageReceiver(),
        mGameSystemInfo->getGamePadSystem());
    mGameDataHolder->createSaveDataAccessSequence(nullptr, nullptr, layoutInitInfo);
    mAsyncResourceLoader = new ProductAsyncResourceLoader(getAudioSystemInfo(), mGameDataHolder);

    mStateTopMenu = new ProductStateTopMenu(this, rInfo, layoutInitInfo, mStageWipeKeeper,
                                            mAsyncResourceLoader, mScreenCaptureExecutor,
                                            mControllerConnectChecker, mWipeCurtainResult);
    mStateBoot = new ProductStateBoot(this, rInfo, mStageWipeKeeper, mAsyncResourceLoader,
                                      mScreenCaptureExecutor);
    mStateTitle = new ProductStateTitle(this, rInfo, layoutInitInfo, mStageWipeKeeper,
                                        mAsyncResourceLoader, mScreenCaptureExecutor,
                                        mWipeCurtainResult);
    mStateSingleOpeningDemo = new ProductStateSingleOpeningDemo(this, mStageWipeKeeper, rInfo,
                                                                mControllerConnectChecker);
    mStateSingleMode = new ProductStateSingleMode(
        this, mStageStartParam, mStageWipeKeeper, rInfo, layoutInitInfo, mGameDataHolder,
        mControllerConnectChecker, mScreenCaptureExecutor);
    mStateSingleModeEnding = new ProductStateSingleModeEnding(
        this, rInfo, layoutInitInfo, mStageWipeKeeper, mScreenCaptureExecutor);
    mStateCourseSelect = new ProductStateCourseSelect(
        this, mStageStartParam, mStageWipeKeeper, rInfo, layoutInitInfo,
        mControllerConnectChecker, mScreenCaptureExecutor, mWipeCurtainResult);
    mStateStage = new ProductStateStage(this, mStageStartParam, mStageWipeKeeper, rInfo,
                                        layoutInitInfo, mGameDataHolder,
                                        mControllerConnectChecker, mScreenCaptureExecutor);
    mStateEnding = new ProductStateEnding(this, rInfo, layoutInitInfo, mStageWipeKeeper,
                                          mScreenCaptureExecutor);
    mStateAfterEndingEvent = new ProductStateAfterEndingEvent(
        this, mStageStartParam, mStageWipeKeeper, rInfo, layoutInitInfo, mScreenCaptureExecutor,
        mWipeCurtainResult);
    mStateLuigiBros = new ProductStateLuigiBros(this, mStageStartParam, rInfo, layoutInitInfo,
                                                mGameDataHolder, mStageWipeKeeper,
                                                mScreenCaptureExecutor, mControllerConnectChecker);

    initNerve(&NrvProductSequence.TopMenu, cNerveNum);
    al::initNerveState(this, mStateTopMenu, &NrvProductSequence.TopMenu, "TopMenu");
    al::initNerveState(this, mStateBoot, &NrvProductSequence.Boot, "Boot");
    al::initNerveState(this, mStateTitle, &NrvProductSequence.Title, "Title");
    al::initNerveState(this, mStateSingleOpeningDemo, &NrvProductSequence.SingleModeOpeningDemo,
                       "SingleModeOpening");
    al::initNerveState(this, mStateSingleMode, &NrvProductSequence.SingleMode, "SingleMode");
    al::initNerveState(this, mStateSingleModeEnding, &NrvProductSequence.SingleModeEnding,
                       "SingleModeEnding");
    al::initNerveState(this, mStateCourseSelect, &NrvProductSequence.CourseSelect,
                       "CourseSelect");
    al::initNerveState(this, mStateStage, &NrvProductSequence.Stage, "Stage");
    al::initNerveState(this, mStateEnding, &NrvProductSequence.Ending, "Ending");
    al::initNerveState(this, mStateAfterEndingEvent, &NrvProductSequence.AfterEndingEvent,
                       "AfterEndingEvent");
    al::initNerveState(this, mStateLuigiBros, &NrvProductSequence.LuigiBros, "LuigiBros");
    mLayoutKit->endInit();

    _198 = nullptr;
    mGameDataHolder->setPlayReportManager(nullptr);
    mGameDataHolder->initializePlayReport("");
    mGameDataHolder->updatePlayStyle();
}

/**
 * Updates the sequence: play report, controller connection, system error pause and layouts.
 */
void ProductSequence::update() {
    al::GameFrameworkNx* framework = getFramework();
    framework->mIsDocked =
        mGameSystemInfo->getApplicationMessageReceiver()->getCachedOperationMode() ==
        nn::oe::OperationMode_Docked;
    mGameDataHolder->updatePlayReport();

    // The home button menu state is computed, but the release build does not use it.
    bool isEnableHomeButtonMenu =
        SaveDataAccessFunction::isEnableHomeButtonMenu(mGameDataHolder);
    const sead::ThreadList& threadList = sead::ThreadMgr::instance()->getThreadList();
    for (auto it = threadList.begin(); it != threadList.end(); ++it) {
        if (!al::isEqualString((*it)->getName(), sead::SafeString("シーン初期化スレッド"))) {
            continue;
        }

        if (!(*it)->isDone()) {
            isEnableHomeButtonMenu = false;
            break;
        }
    }

    if (al::isNerve(this, &NrvProductSequence.CourseSelect) ||
        al::isNerve(this, &NrvProductSequence.Stage) ||
        al::isNerve(this, &NrvProductSequence.Title) ||
        al::isNerve(this, &NrvProductSequence.SingleModeOpeningDemo) ||
        al::isNerve(this, &NrvProductSequence.SingleMode) ||
        al::isNerve(this, &NrvProductSequence.TopMenu) ||
        al::isNerve(this, &NrvProductSequence.LuigiBros)) {
        mControllerConnectChecker->update();
    }

    if (mCurrentScene != nullptr) {
        mCurrentScene->mIsExecute = true;
    }

    mIsChangeScene = true;

    if (mIsRequestPauseBySystemError && mIsPauseBySystemError) {
        if (mCurrentScene != nullptr) {
            alAudioSystemFunction::pauseBySystemError(mCurrentScene->getAudioDirector(),
                                                      mAudioDirector, false, 0);
        } else {
            alAudioSystemFunction::pauseBySystemError(nullptr, mAudioDirector, false, 0);
        }

        mIsPauseBySystemError = false;
    }

    mIsRequestPauseBySystemError = false;
    al::Sequence::update();
    al::executeUpdate(mLayoutKit);

    const al::ApplicationMessageReceiver* receiver =
        mGameSystemInfo->getApplicationMessageReceiver();
    if (!receiver->isExitRequested() && receiver->isUpdatedOperationMode()) {
        mGameDataHolder->updatePlayStyle();
    }
}

/**
 * Draws the scene, the screen captures and the sequence layouts on the main screen.
 */
void ProductSequence::drawMain() const {
    if (al::isNerve(this, &NrvProductSequence.TopMenu) && mCurrentScene == nullptr) {
        getFramework()->clearFrameBuffer();
    }

    bool isForceDraw = false;
    if (mScreenCaptureExecutor->isActiveRequest(0)) {
        if (mCurrentScene != nullptr && !mCurrentScene->isAlive()) {
            mCurrentScene->stall();
        }

        isForceDraw = true;
    }

    doDrawScene(isForceDraw);

    const agl::RenderBuffer* renderBuffer = getFramework()->getCurrentRenderBuffer();
    if (mScreenCaptureExecutor->isDraw(0)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(), renderBuffer, 0);
    }

    mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(), renderBuffer, 0);
    mViewportTop->setByFrameBuffer(*getFramework()->getCurrentRenderBuffer());
    alSystemKitFunction::applyViewportTop(*mViewportTop);
    mLayoutKit->setFrameBuffer(renderBuffer, mViewportTop);

    al::LayoutKit* layoutKit = mLayoutKit;
    al::tryChangeShaderMode(al::GameFrameworkNx::getAglDrawContext(),
                            agl::cShaderMode_UniformRegister);
    sead::GraphicsContext graphicsContext;
    graphicsContext.setDepthEnable(false, false);
    graphicsContext.setCullingMode(0);
    graphicsContext.apply(al::GameFrameworkNx::getDrawContext());
    al::executeDraw(layoutKit, "２Ｄベース（メイン画面）");
    al::executeDraw(layoutKit, "2DDrawSequence");

    if ((al::isNerve(this, &NrvProductSequence.TopMenu) && mStateTopMenu->isEffectDraw()) ||
        (al::isNerve(this, &NrvProductSequence.AfterEndingEvent) &&
         mStateAfterEndingEvent->isEffectDraw())) {
        al::executeDrawEffect(mLayoutKit);
    }
}

/**
 * Draws the sub screen (nothing on this platform).
 */
void ProductSequence::drawSub() const {}

/**
 * Shows the loading screen.
 * @param isFadeIn Whether the loading screen fades in.
 * @param isShowTips Whether the loading screen shows tips.
 */
void ProductSequence::startLoadScreen(bool isFadeIn, bool isShowTips) {
    mLoadingLayout->startDisplay(isFadeIn, isShowTips);
}

/**
 * Requests the loading screen to close.
 */
void ProductSequence::requestLoadScreenEnd() {
    mLoadingLayout->requestEnd();
}

/**
 * Checks whether the loading screen is closed.
 * @return True when the loading screen is no longer displayed.
 */
bool ProductSequence::isLoadLayoutEnd() {
    return !mLoadingLayout->isDisplay();
}

/**
 * Nerve: top menu. Starts the title or Bowser's Fury once the menu ends.
 */
void ProductSequence::exeTopMenu() {
    al::isFirstStep(this);

    if (mStateTopMenu->isEffectDraw()) {
        al::executeUpdateEffect(mLayoutKit);
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mIsGotoTitleFromTopMenu) {
        mStateTitle->resetState();
        mIsShowResult = false;
        al::setNerve(this, &NrvProductSequence.Title);
        return;
    }

    mGameSystemInfo->getGamePadSystem()->disableControllerApplet(true);
    mGameSystemInfo->getGamePadSystem()->setAssistMode(false, false);
    mGameDataHolder->set2PAssistMode(false);
    al::setNerve(this, &NrvProductSequence.SingleMode);
}

/**
 * Nerve: boot. Goes to the title once the boot state ends.
 */
void ProductSequence::exeBoot() {
    al::isFirstStep(this);

    if (al::updateNerveState(this)) {
        mStageWipeKeeper->closeBootWipe();
        al::setNerve(this, &NrvProductSequence.Title);
    }
}

/**
 * Nerve: Super Mario 3D World title screen.
 */
void ProductSequence::exeTitle() {
    if (al::isFirstStep(this)) {
        mIsShowResult = false;
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateTitle->isGotoLuigiBros()) {
        al::setNerve(this, &NrvProductSequence.LuigiBros);
        return;
    }

    if (mStateTitle->isGotoTopMenu()) {
        mStateTopMenu->setEntry(ProductTopMenuEntry::Default);
        al::setNerve(this, &NrvProductSequence.TopMenu);
        return;
    }

    al::setNerve(this, &NrvProductSequence.Stage);
}

/**
 * Nerve: Bowser's Fury opening demo.
 */
void ProductSequence::exeSingleModeOpeningDemo() {
    if (al::isFirstStep(this)) {
        mGameDataHolder->setLastPlayedMode(GameMode_Single);
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvProductSequence.SingleMode);
        mStateSingleMode->setAfterOpeningDemo(true);
    }
}

/**
 * Nerve: Super Mario 3D World course select.
 */
void ProductSequence::exeCourseSelect() {
    if (al::isFirstStep(this)) {
        mGameDataHolder->setLastPlayedMode(GameMode_3DWorld);
    }

    if (al::updateNerveState(this)) {
        if (mStateCourseSelect->isGotoTitle()) {
            mStateTopMenu->setEntry(ProductTopMenuEntry::Default);
            al::setNerve(this, &NrvProductSequence.TopMenu);
            return;
        }

        bool isLoadGame = mStateCourseSelect->isLoadGame();
        mIsShowResult = false;
        if (!isLoadGame) {
            al::setNerve(this, &NrvProductSequence.Stage);
            return;
        }

        mStateTitle->setLoadGame(true);
        al::setNerve(this, &NrvProductSequence.Title);
    }

    if (mStateCourseSelect->isEffectDraw()) {
        al::executeUpdateEffect(mLayoutKit);
    }
}

/**
 * Nerve: Super Mario 3D World stage. Picks the next nerve from how the stage ended.
 */
void ProductSequence::exeStage() {
    if (al::isFirstStep(this)) {
        mGameDataHolder->setLastPlayedMode(GameMode_3DWorld);
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateStage->isClear() || mStateStage->isClearWithResult()) {
        mIsShowResult = mStateStage->isClearWithResult();

        if (GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(mGameDataHolder)) ==
            GameDataFunction::getLastKoopaCourseId(GameDataHolderAccessor(mGameDataHolder))) {
            mGameDataHolder->unlockLuigiBros();
            mStageWipeKeeper->closeEndingWipe();
            al::setNerve(this, &NrvProductSequence.Ending);
            return;
        }

        if (!mIsShowResult) {
            mStageWipeKeeper->closeNoResultWipe(mGameDataHolder, false);
        }

        mStateTopMenu->setEntry(ProductTopMenuEntry::StageClear);
        al::setNerve(this, &NrvProductSequence.TopMenu);
        return;
    }

    if (mStateStage->isClearWorldWarp()) {
        mStageWipeKeeper->closeNoResultWipe(mGameDataHolder, true);
        al::setNerve(this, &NrvProductSequence.Title);
        return;
    }

    if (mStateStage->isGameOver()) {
        mStageWipeKeeper->closeNoResultWipeGameOver();
        al::setNerve(this, &NrvProductSequence.Title);
        return;
    }

    if (mStateStage->isGameEnd()) {
        mIsShowResult = false;
        mStageWipeKeeper->closeNoResultWipeGameEnd();
        mStateTopMenu->setEntry(ProductTopMenuEntry::Default);
        al::setNerve(this, &NrvProductSequence.TopMenu);
        return;
    }

    if (mStateStage->isRetire()) {
        mStageWipeKeeper->closeNoResultWipe(mGameDataHolder, true);
        mStateTopMenu->setEntry(ProductTopMenuEntry::Default);
        al::setNerve(this, &NrvProductSequence.TopMenu);
        return;
    }

    if (mStateStage->isLoadGame()) {
        mStateTitle->setLoadGame(true);
        al::setNerve(this, &NrvProductSequence.Title);
        return;
    }

    al::setNerve(this, &NrvProductSequence.CourseSelect);
}

/**
 * Nerve: Bowser's Fury.
 */
void ProductSequence::exeSingleMode() {
    if (al::isFirstStep(this)) {
        mGameDataHolder->setLastPlayedMode(GameMode_Single);
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateSingleMode->isGameChange()) {
        mStageWipeKeeper->closeBootWipe();
        al::setNerve(this, &NrvProductSequence.Title);
        return;
    }

    if (mStateSingleMode->isGameEnd()) {
        mStageWipeKeeper->closeEndingWipe();
        al::setNerve(this, &NrvProductSequence.SingleModeEnding);
        return;
    }

    if (mStateSingleMode->isLoadGame()) {
        if (mGameDataHolder->isNewFile()) {
            al::setNerve(this, &NrvProductSequence.SingleModeOpeningDemo);
            return;
        }

        al::setNerve(this, &NrvProductSequence.SingleMode);
        return;
    }

    mStateTopMenu->setEntry(ProductTopMenuEntry::SingleMode);
    al::setNerve(this, &NrvProductSequence.TopMenu);
}

/**
 * Nerve: Super Mario 3D World ending.
 */
void ProductSequence::exeEnding() {
    if (!al::updateNerveState(this)) {
        return;
    }

    if (mStateEnding->isEndFirstTime()) {
        al::setNerve(this, &NrvProductSequence.AfterEndingEvent);
        return;
    }

    mIsShowResult = true;
    al::setNerve(this, &NrvProductSequence.Title);
}

/**
 * Nerve: Bowser's Fury ending. Marks the ending as seen and goes back to the top menu.
 */
void ProductSequence::exeSingleModeEnding() {
    al::isFirstStep(this);

    if (al::updateNerveState(this)) {
        SingleModeDataFunction::setHasSeenEnding(GameDataHolderAccessor(mGameDataHolder), true);
        SingleModeDataFunction::setUnlockedPhase(GameDataHolderAccessor(mGameDataHolder), 8);
        SingleModeDataFunction::setPhaseEnd(GameDataHolderAccessor(mGameDataHolder), false);
        mStateTopMenu->setEntry(ProductTopMenuEntry::SingleModeEnding);
        al::setNerve(this, &NrvProductSequence.TopMenu);
    }
}

/**
 * Nerve: event shown after the first Super Mario 3D World ending.
 */
void ProductSequence::exeAfterEndingEvent() {
    if (al::updateNerveState(this)) {
        mIsShowResult = false;
        mStageWipeKeeper->closeNoResultWipeTitle();
        al::setNerve(this, &NrvProductSequence.Title);
        return;
    }

    if (mStateAfterEndingEvent->isEffectDraw()) {
        al::executeUpdateEffect(mLayoutKit);
    }
}

/**
 * Nerve: Luigi Bros. Goes back to the title once the game ends.
 */
void ProductSequence::exeLuigiBros() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvProductSequence.Title);
    }
}

/**
 * Requests a capture of the screen without drawing it.
 */
void ProductSequence::requestCaptureTopBottom() {
    mScreenCaptureExecutor->requestCapture(false, 0, false);
}

/**
 * Stops drawing the screen capture.
 */
void ProductSequence::offDrawScreenCapture() {
    mScreenCaptureExecutor->offDraw(0);
}

/**
 * Sets the stage the kiosk demo starts.
 * @param worldId World of the stage.
 * @param stageId Stage in the world.
 */
void ProductSequence::setKioskStageStartParam(s32 worldId, s32 stageId) {
    mStageStartParam->setWorldId(worldId);
    mStageStartParam->setStageId(stageId);
}

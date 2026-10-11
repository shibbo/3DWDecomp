#include "Scene/TopMenuScene.hpp"

#include <common/aglRenderBuffer.h>
#include <gfx/seadViewport.h>
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/RCS_SaveDataLayout.hpp"
#include "Layout/TopMenu.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/PadRumbleDirector.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/SaveData/SaveDataFunction.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/System/SystemKit.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/SceneObjFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Application.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/PlayLogFunction.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/InputUtil.hpp"

/**
 * Declares a TopMenuScene nerve whose execute function has a different name than the nerve.
 * @param Action The nerve name.
 * @param Func The TopMenuScene::exe* function the nerve runs.
 */
#define TOP_MENU_SCENE_NERVE(Action, Func)                                                         \
    class TopMenuSceneNrv##Action : public al::Nerve {                                             \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<TopMenuScene>()->exe##Func();                                       \
        }                                                                                          \
    };

namespace {
NERVE_DECL(TopMenuScene, Init)
NERVE_DECL(TopMenuScene, TopMenu)
NERVE_DECL(TopMenuScene, FileSelect)
NERVE_DECL(TopMenuScene, NewGameWarning)
NERVE_DECL(TopMenuScene, TopMenuEnd)
TOP_MENU_SCENE_NERVE(TopMenuEndSingleMode, TopMenuEnd)
NERVE_DECL(TopMenuScene, SingleModeTitle)
NERVE_DECL(TopMenuScene, InitError)
NERVE_DECL(TopMenuScene, LoadSaveData)
TOP_MENU_SCENE_NERVE(LoadSaveDataAfterWrite, LoadSaveData)
NERVE_DECL(TopMenuScene, WriteSaveData)
NERVE_DECL(TopMenuScene, WaitTextFade)
NERVE_DECL(TopMenuScene, DeleteFile)
NERVES_MAKE_NOSTRUCT(TopMenuScene, Init, TopMenu, FileSelect, NewGameWarning, TopMenuEnd,
                     TopMenuEndSingleMode, SingleModeTitle, InitError, LoadSaveData,
                     LoadSaveDataAfterWrite, WriteSaveData, WaitTextFade, DeleteFile)

/** GameMode value of Bowser's Fury. */
constexpr u32 cGameModeSingle = 1;

/**
 * Gets the application's game framework.
 * @return The game framework.
 */
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}
}  // namespace

/**
 * Constructs the top menu scene.
 * @param pStageWipeKeeper The wipes shared with the stage scenes.
 */
TopMenuScene::TopMenuScene(StageWipeKeeper* pStageWipeKeeper)
    : al::Scene("TopMenu"), mStageWipeKeeper(pStageWipeKeeper) {}

/**
 * Destroys the top menu scene, switching the controllers back to 3D World mode.
 */
TopMenuScene::~TopMenuScene() {
    rc::set3dWorldPlayerMode();
    mScreenCaptureExecutor->offDraw(1);
    mScreenCaptureExecutor->offDraw(2);

    if (mLiveActorKit != nullptr) {
        mLiveActorKit->getEffectSystem()->endScene();
    }
}

/**
 * Initializes the scene: audio, kits, the menu layouts and the save data windows.
 * @param rInfo The scene init info.
 */
void TopMenuScene::init(const al::SceneInitInfo& rInfo) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mSceneObjHolder = SceneObjFactory::createSceneObjHolder();
    mSceneObjHolder->setSceneObj(mGameDataHolder, SceneObjID_GameDataHolder);
    mViewport = new sead::Viewport(*Application::instance()->getFramework()->getMethodFrameBuffer(6));
    mScreenCaptureExecutor = rInfo.mScreenCaptureExecutor;
    initSceneAudio(rInfo, "TitleDemo01Stage", 60, 30, 1, false, "Scene", 20, 1.0f);
    initAudioKeeper("TopMenuScene");
    initLiveActorKit(rInfo, 8, 1, 1, 0);
    initLayoutKit(rInfo);

    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    mTopMenu = new TopMenu(this, layoutInfo, mGameDataHolder, mStageWipeKeeper, nullptr);
    mWindowConfirm = new WindowConfirm(WindowConfirmType_Double, layoutInfo, nullptr, false);
    mSaveDataLayout = new RCS_SaveDataLayout(layoutInfo, mGameDataHolder,
                                             rInfo.mScreenCaptureExecutor, mControlGuideBar, false);
    mControlGuideBar = new RCSControlGuideBar(layoutInfo);
    mTopMenu->setGuideBar(mControlGuideBar);
    mSaveDataLayout->setGuideBar(mControlGuideBar);

    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    endInit(actorInfo, nullptr);
    initNerve(&NrvTopMenuSceneInit, 0);
}

/**
 * Starts the scene.
 * @param state How the top menu shows up.
 * @param isFirstAppear Whether the scene is shown for the first time since boot.
 */
void TopMenuScene::appear(TopMenuAppearState state, bool isFirstAppear) {
    mAppearState = state;
    mIsFirstAppear = isFirstAppear;
    al::Scene::appear();

    if (mAppearState == TopMenuAppearState_SingleModeTitle) {
        al::setNerve(this, &NrvTopMenuSceneTopMenu);
    } else {
        al::setNerve(this, &NrvTopMenuSceneInit);
    }

    mScreenCaptureExecutor->offDraw(1);
    mScreenCaptureExecutor->offDraw(2);
    mIsStartSingleMode = false;
}

/**
 * Updates the scene.
 */
void TopMenuScene::control() {
    mLiveActorKit->getPadRumbleDirector()->update();

    if (!al::isNerve(this, &NrvTopMenuSceneFileSelect) ||
        al::isNerve(this, &NrvTopMenuSceneNewGameWarning)) {
        al::updateKitList(this, "２Ｄ");
    }

    al::updateKitList(this, "２Ｄ（ポーズ無視）");
    al::updateEffectLayout(this);
}

/**
 * Draws the menu layouts and the captured screens to the main screen.
 */
void TopMenuScene::drawMain_() const {
    mViewport->setByFrameBuffer(*getFramework()->getCurrentRenderBuffer());
    alSystemKitFunction::applyViewportTop(*mViewport);
    mLayoutKit->setFrameBuffer(getFramework()->getCurrentRenderBuffer(), mViewport);

    if (!al::isNerve(this, &NrvTopMenuSceneFileSelect)) {
        al::drawKit(this, "２Ｄベース（メイン画面）");
    }

    bool isDrawCapture1 = mScreenCaptureExecutor->isDraw(1);

    if (mScreenCaptureExecutor->isDraw(2)) {
        mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                     getFramework()->getCurrentRenderBuffer(), 2);
    } else {
        if (isDrawCapture1) {
            mScreenCaptureExecutor->draw(al::GameFrameworkNx::getAglDrawContext(),
                                         getFramework()->getCurrentRenderBuffer(), 1);
        }

        al::drawKit(this, "2DDrawAboveBlur1");
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           getFramework()->getCurrentRenderBuffer(), 2);
    }

    if (!isDrawCapture1) {
        mScreenCaptureExecutor->tryCapture(al::GameFrameworkNx::getAglDrawContext(),
                                           getFramework()->getCurrentRenderBuffer(), 1);
    }

    al::drawKit(this, "2DDrawAboveBlur2");
    al::tryChangeShaderMode(al::GameFrameworkNx::getAglDrawContext(),
                            agl::cShaderMode_UniformBlock);
}

/**
 * Gets whether a game was chosen and the scene is ending.
 * @return true if 3D World or Bowser's Fury was chosen.
 */
bool TopMenuScene::isSelectAny() {
    return al::isNerve(this, &NrvTopMenuSceneTopMenuEnd) ||
           al::isNerve(this, &NrvTopMenuSceneTopMenuEndSingleMode);
}

/**
 * Opens the Bowser's Fury title menu.
 */
void TopMenuScene::startSingleModeTitleMenu() {
    mMode = TopMenuSceneMode_SingleModeTitle;
    al::startSequenceBgm(this, "SingleModeTitle", -1, 0);
    al::startSe(this, "Storm");
    al::setNerve(this, &NrvTopMenuSceneSingleModeTitle);
}

/**
 * Leaves the Bowser's Fury title menu back to the top menu.
 */
void TopMenuScene::cancelSingleModeTitleMenu() {
    mMode = TopMenuSceneMode_TopMenu;
    mAppearState = TopMenuAppearState_SingleMode;
    mControlGuideBar->hide();
    al::setNerve(this, &NrvTopMenuSceneTopMenu);
}

/**
 * Fades out the music and the storm sound of the Bowser's Fury title menu.
 * @param fadeFrames The fade out length of the storm sound.
 */
void TopMenuScene::cancelSingleModeTitleMenuAudio(s32 fadeFrames) {
    al::stopSequenceBgm(this, "SingleModeTitle", 30);
    al::stopAllSeId(this, "Storm", fadeFrames, nullptr);
}

/**
 * Initializes the save data.
 */
void TopMenuScene::exeInit() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataInit(mGameDataHolder);
        mIsSaveInit = true;
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, mIsSaveInit)) {
        mIsSaveInit = false;
        al::setNerve(this, &NrvTopMenuSceneLoadSaveData);
        return;
    }

    if (SaveDataAccessFunction::isWaitShowError(mGameDataHolder)) {
        al::setNerve(this, &NrvTopMenuSceneInitError);
    }
}

/**
 * Waits for the save data error to be closed.
 */
void TopMenuScene::exeInitError() {
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        mIsSaveDataLoaded = true;
        al::setNerve(this, &NrvTopMenuSceneTopMenu);
    }
}

/**
 * Reads the save data on the first appearance, writing it back if it could not be read.
 */
void TopMenuScene::exeLoadSaveData() {
    if (!mIsFirstAppear) {
        al::setNerve(this, &NrvTopMenuSceneTopMenu);
        return;
    }

    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataRead(mGameDataHolder, false);
    }

    if (!SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        return;
    }

    if (!al::isSuccessSaveDataSequence() &&
        !al::isNerve(this, &NrvTopMenuSceneLoadSaveDataAfterWrite)) {
        al::setNerve(this, &NrvTopMenuSceneWriteSaveData);
        return;
    }

    PlayLogFunction::setPlayStart(GameDataHolderWriter(mGameDataHolder));
    mIsSaveDataLoaded = true;
    al::setNerve(this, &NrvTopMenuSceneTopMenu);
}

/**
 * Writes the save data, then reads it again.
 */
void TopMenuScene::exeWriteSaveData() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWriteNoWindow(mGameDataHolder, false, false);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::setNerve(this, &NrvTopMenuSceneLoadSaveDataAfterWrite);
    }
}

/**
 * Shows the top menu and waits for a game to be chosen.
 */
void TopMenuScene::exeTopMenu() {
    if (al::isFirstStep(this)) {
        mStageWipeKeeper->tryOpenFadeBlack();
        mStageWipeKeeper->tryOpenStartOrRetryWipe();

        if (mStageWipeKeeper->isActiveNoResultWipe()) {
            mStageWipeKeeper->tryOpenNoResultWipe();
        }

        if (mIsFirstAppear) {
            bool isLastSingleMode =
                mGameDataHolder->getCommon()->getLastPlayedMode() == cGameModeSingle;
            mIsFirstAppear = false;
            mAppearState =
                isLastSingleMode ? TopMenuAppearState_SingleMode : TopMenuAppearState_3DWorld;
        } else if (mAppearState == TopMenuAppearState_Reset) {
            mAppearState = TopMenuAppearState_3DWorld;
        }

        mTopMenu->appear(mAppearState);

        if (mAppearState != TopMenuAppearState_SingleModeTitle) {
            al::startSequenceBgm(this, "MainTitle", -1, 0);
        }
    }

    if (mTopMenu->isEnd()) {
        al::setNerve(this, &NrvTopMenuSceneTopMenuEnd);
    }
}

/**
 * Ends the scene once the chosen game starts, entering the player of a Bowser's Fury game.
 */
void TopMenuScene::exeTopMenuEnd() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvTopMenuSceneTopMenuEndSingleMode)) {
        SingleModeData* singleFile = mGameDataHolder->getSingleFile();
        PlayerEntryFunction::entryPlayer(GameDataHolderWriter(mGameDataHolder), 0, 0);

        if (singleFile->isNewFile()) {
            singleFile->startOpening();
        }
    }

    if (al::isLessStep(this, 2) || mStageWipeKeeper->isActiveNoResultWipe()) {
        return;
    }

    mScreenCaptureExecutor->offDraw(1);
    mScreenCaptureExecutor->offDraw(2);
}

/**
 * Appearance of the Bowser's Fury title menu.
 */
void TopMenuScene::exeSingleModeTitleAppear() {}

/**
 * Runs the Bowser's Fury title menu: new game or resume.
 */
void TopMenuScene::exeSingleModeTitle() {
    al::isFirstStep(this);

    if (mTopMenu->isDecideSingleModeNewGame()) {
        s32 fileId = GameDataFunction::getFirstNewFileId(GameDataHolderAccessor(mGameDataHolder));

        if (fileId < 0) {
            mScreenCaptureExecutor->requestCapture(false, 1, true);
            al::setNerve(this, &NrvTopMenuSceneNewGameWarning);
            return;
        }

        mGameDataHolder->setSingleModePlayingFileID(fileId, true);
        al::startSe(mTopMenu, "GameStart");
        al::stopSequenceBgm(this, "SingleModeTitle", 60);
        al::tryStopSe(this, "Storm");
    } else if (mTopMenu->isDecideSingleModeResume()) {
        if (GameDataFunction::getNewFileNum(GameDataHolderAccessor(mGameDataHolder)) != 3) {
            al::setNerve(this, &NrvTopMenuSceneWaitTextFade);
            return;
        }

        mGameDataHolder->setSingleModePlayingFileID(
            mGameDataHolder->getLastSingleModePlayingFileID(), false);
        SingleModeData* singleFile = mGameDataHolder->getSingleModeDataFile(
            mGameDataHolder->getLastSingleModePlayingFileID());
        singleFile->clearIslandGraffitiVandalized();

        if (singleFile->getUnlockedPhase() == 7 || singleFile->getUnlockedPhase() == 10) {
            singleFile->setPhaseFromPlessieChase();
        }

        al::stopSequenceBgm(this, "SingleModeTitle", 60);
        al::tryStopSe(this, "Storm");
        mIsStartSingleMode = true;
    } else {
        return;
    }

    al::setNerve(this, &NrvTopMenuSceneTopMenuEndSingleMode);
}

/**
 * Warns that every save file is in use before starting a new game.
 */
void TopMenuScene::exeNewGameWarning() {
    if (al::isFirstStep(this)) {
        mWindowConfirm->appearWithSystemMessage("WindowConfirmTitleScene", "NewFileWarning",
                                                al::getMainControllerPort(), "Rボタン");
        al::startAction(mTopMenu, "Hide", "Visibility");
        mControlGuideBar->hide();
        al::updateKitList(this, "２Ｄ");
    }

    if (mWindowConfirm->isAlive()) {
        return;
    }

    if (mWindowConfirm->isDecideLeftEnd()) {
        mTopMenu->returnFromFileSelect("NewGame", false);
        mScreenCaptureExecutor->offDraw(1);
        al::setNerve(this, &NrvTopMenuSceneSingleModeTitle);
        al::startAction(mTopMenu, "Show", "Visibility");
        mControlGuideBar->showTitle();
    } else if (mWindowConfirm->isDecideRightEnd()) {
        al::setNerve(this, &NrvTopMenuSceneDeleteFile);
    }
}

/**
 * Lets the player choose the Bowser's Fury save file to resume.
 */
void TopMenuScene::exeFileSelect() {
    if (al::isFirstStep(this) && !mSaveDataLayout->isAlive()) {
        mSaveDataLayout->appearLoad(al::getMainControllerPort());
    }

    if (mSaveDataLayout->isEndBack()) {
        mTopMenu->returnFromFileSelect("Resume", true);
        mScreenCaptureExecutor->offDraw(1);
        al::startAction(mTopMenu, "Show", "Visibility");
        al::setNerve(this, &NrvTopMenuSceneSingleModeTitle);
    } else if (mSaveDataLayout->isNeedLoad()) {
        al::stopSequenceBgm(this, "SingleModeTitle", 60);
        al::tryStopSe(this, "Storm");
        mIsStartSingleMode = true;
        al::setNerve(this, &NrvTopMenuSceneTopMenuEndSingleMode);
    }
}

/**
 * Lets the player delete a save file when every file is in use.
 */
void TopMenuScene::exeDeleteFile() {
    if (al::isFirstStep(this)) {
        mSaveDataLayout->appearDelete(al::getMainControllerPort(), true);
    }

    if (mSaveDataLayout->isNeedLoad()) {
        al::stopSequenceBgm(this, "SingleModeTitle", 60);
        al::tryStopSe(this, "Storm");
        al::setNerve(this, &NrvTopMenuSceneTopMenuEndSingleMode);
    }

    if (mSaveDataLayout->isAlive()) {
        return;
    }

    al::startAction(mTopMenu, "Show", "Visibility");
    mTopMenu->returnFromFileSelect(nullptr, true);
    mScreenCaptureExecutor->offDraw(1);
    mControlGuideBar->showTitle();
    al::setNerve(this, &NrvTopMenuSceneSingleModeTitle);
}

/**
 * Fades to black and ends the scene.
 */
void TopMenuScene::exeEndFade() {
    if (al::isFirstStep(this)) {
        mStageWipeKeeper->closeWipeFadeBlack(-1);
        return;
    }

    if (mStageWipeKeeper->isCloseEndFadeBlack()) {
        al::stopSequenceBgm(this, "SingleModeTitle", 60);
        al::tryStopSe(this, "Storm");
        kill();
    }
}

/**
 * Waits for the guide bar text to change before opening the file select.
 */
void TopMenuScene::exeWaitTextFade() {
    if (al::isFirstStep(this)) {
        mControlGuideBar->changeTextTitle(RCSControlGuideBar::GuideBarMsgType_FileSelect,
                                          al::getMainControllerPort());
        return;
    }

    if (mControlGuideBar->isChangingText()) {
        return;
    }

    mScreenCaptureExecutor->requestCapture(false, 1, true);
    al::setNerve(this, &NrvTopMenuSceneFileSelect);
}

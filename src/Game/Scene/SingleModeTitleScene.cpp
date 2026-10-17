#include "Scene/SingleModeTitleScene.hpp"
#include "Layout/SingleModeTitle.hpp"
#include <gfx/seadViewport.h>
#include <common/aglRenderBuffer.h>
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/RCS_SaveDataLayout.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKit.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/System/SystemKit.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Stage/StageWipeKeeper.hpp"
#include "System/Application.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/SaveDataAccessFunction.hpp"

namespace {
NERVE_DECL(SingleModeTitleScene, Load)
NERVE_DECL(SingleModeTitleScene, Kill)
NERVE_DECL(SingleModeTitleScene, Title)
NERVE_DECL(SingleModeTitleScene, AppearWait)
NERVE_DECL(SingleModeTitleScene, NewGameWarning)
NERVE_DECL(SingleModeTitleScene, WipeClose)
NERVE_DECL(SingleModeTitleScene, FileSelect)
NERVE_DECL(SingleModeTitleScene, DeleteFile)
NERVES_MAKE_NOSTRUCT(SingleModeTitleScene, Load, Kill, Title, AppearWait, NewGameWarning, WipeClose,
                     FileSelect, DeleteFile)
al::GameFrameworkNx* getFramework() {
    return static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
}
} // namespace

SingleModeTitleScene::SingleModeTitleScene(StageWipeKeeper* wipe)
    : al::Scene("SingleModeTitleScene"), mWipe(wipe) {}
SingleModeTitleScene::~SingleModeTitleScene() {
    if (mLiveActorKit)
        mLiveActorKit->getEffectSystem()->endScene();
}
void SingleModeTitleScene::init(const al::SceneInitInfo& info) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(info.mGameDataHolder);
    mGameDataHolder->setSingleMode(true);
    mViewport = new sead::Viewport(*getFramework()->getMethodFrameBuffer(6));
    initSceneAudio(info, "TitleDemo01Stage", 60, 30, 1, false, "Scene", 20, 1.0f);
    initAudioKeeper(nullptr);
    initLiveActorKit(info, 8, 1, 1, 0);
    initLayoutKit(info);
    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, info);
    mTitle = new SingleModeTitle(layoutInfo, mGameDataHolder);
    mConfirm = new WindowConfirm(WindowConfirmType_Double, layoutInfo, nullptr, false);
    mFileSelect = new RCS_SaveDataLayout(layoutInfo, mGameDataHolder, nullptr, nullptr, false);
    al::PlacementInfo placementInfo;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placementInfo, &layoutInfo, false);
    endInit(actorInfo, nullptr);
    initNerve(&NrvSingleModeTitleSceneLoad, 0);
}
void SingleModeTitleScene::appear() {
    al::Scene::appear();
    al::setNerve(this, &NrvSingleModeTitleSceneLoad);
    getFramework()->mIsClearRenderBuffer = false;
}
void SingleModeTitleScene::kill() { al::setNerve(this, &NrvSingleModeTitleSceneKill); }
void SingleModeTitleScene::control() {
    al::updateKitList(this, "２Ｄ");
    al::updateEffectLayout(this);
}
void SingleModeTitleScene::drawMain_() const {
    mViewport->setByFrameBuffer(*getFramework()->getCurrentRenderBuffer());
    alSystemKitFunction::applyViewportTop(*mViewport);
    mLayoutKit->setFrameBuffer(getFramework()->getCurrentRenderBuffer(), mViewport);
    al::drawKit(this, "２Ｄベース（メイン画面）");
}
bool SingleModeTitleScene::isReadyShow() const {
    if (al::isNerve(this, &NrvSingleModeTitleSceneTitle))
        return true;
    if (al::isNerve(this, &NrvSingleModeTitleSceneAppearWait))
        return al::isGreaterEqualStep(this, 1);
    return false;
}
bool SingleModeTitleScene::isReturnToTopMenu() const { return mTitle->isReturnToTopMenu(); }
void SingleModeTitleScene::exeLoad() {
    if (al::isFirstStep(this)) {
        if (mGameDataHolder->isSaveDataRead()) {
            al::setNerve(this, &NrvSingleModeTitleSceneAppearWait);
            return;
        }
        SaveDataAccessFunction::startSaveDataRead(mGameDataHolder, false);
    }
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false))
        al::setNerve(this, &NrvSingleModeTitleSceneAppearWait);
}
void SingleModeTitleScene::exeAppearWait() {
    if (al::isFirstStep(this))
        mTitle->appear();
    al::startSequenceBgm(this, "SingleModeTitle", -1, false);
    al::setNerve(this, &NrvSingleModeTitleSceneTitle);
}
void SingleModeTitleScene::exeTitle() {
    if (mTitle->isStartNewGame()) {
        s32 file = GameDataFunction::getFirstNewFileId(GameDataHolderAccessor(mGameDataHolder));
        if (file < 0)
            al::setNerve(this, &NrvSingleModeTitleSceneNewGameWarning);
        else {
            mGameDataHolder->setSingleModePlayingFileID(file, false);
            al::setNerve(this, &NrvSingleModeTitleSceneWipeClose);
        }
    } else if (mTitle->isReturnToTopMenu()) {
        al::setNerve(this, &NrvSingleModeTitleSceneWipeClose);
    } else if (mTitle->isEnd()) {
        al::setNerve(this, &NrvSingleModeTitleSceneFileSelect);
    }
}
void SingleModeTitleScene::exeNewGameWarning() {
    if (al::isFirstStep(this))
        mConfirm->appearWithSystemMessage("WindowConfirmTitleScene", "NewFileWarning",
                                          al::getMainControllerPort(), "R");
    if (!mConfirm->isAlive()) {
        if (mConfirm->isDecideLeftEnd()) {
            mTitle->returnFromMenu("NewGame");
            al::setNerve(this, &NrvSingleModeTitleSceneTitle);
        } else if (mConfirm->isDecideRightEnd()) {
            al::setNerve(this, &NrvSingleModeTitleSceneDeleteFile);
        }
    }
}
void SingleModeTitleScene::exeFileSelect() {
    if (al::isFirstStep(this))
        mFileSelect->appearLoad(al::getMainControllerPort());
    if (mFileSelect->isEndBack()) {
        mTitle->returnFromMenu("Resume");
        al::setNerve(this, &NrvSingleModeTitleSceneTitle);
    } else if (mFileSelect->isNeedLoad()) {
        al::setNerve(this, &NrvSingleModeTitleSceneWipeClose);
    }
}
void SingleModeTitleScene::exeDeleteFile() {
    if (al::isFirstStep(this))
        mFileSelect->appearDelete(al::getMainControllerPort(), true);
    if (!mFileSelect->isAlive()) {
        mTitle->returnFromMenu(nullptr);
        al::setNerve(this, &NrvSingleModeTitleSceneTitle);
    }
}
void SingleModeTitleScene::exeWipeClose() {
    if (!mTitle->isReturnToTopMenu()) {
        const GameDataFileBase* file = mGameDataHolder->getSingleFile();
        bool newFile = file->isNewFile();
        GameDataHolder* holder = mGameDataHolder;
        if (newFile)
            PlayerEntryFunction::entryPlayer(GameDataHolderWriter(holder), 0, 0);
        else
            PlayerEntryFunction::entryPlayer(GameDataHolderWriter(holder), 0,
                                             file->getMainPlayerCharacterType());
    }
    mTitle->kill();
    kill();
}
void SingleModeTitleScene::exeKill() {
    if (al::isFirstStep(this))
        mWipe->closeWipeFadeBlack(-1);
    else if (mWipe->isCloseEndFadeBlack()) {
        al::stopSequenceBgm(this, "Title", 60);
        al::Scene::kill();
    }
}

#include "Scene/BootScene.hpp"

#include <common/aglRenderBuffer.h>
#include <gfx/seadViewport.h>
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Shader/DeferredRendering/SamplerLocation.hpp"
#include "Library/System/SystemKit.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "System/Application.hpp"
#include "System/GameDataFunction.hpp"
#include "System/PlayLogFunction.hpp"
#include "System/SaveDataAccessFunction.hpp"

namespace {
NERVE_DECL(BootScene, Init)
NERVE_DECL(BootScene, LoadEnd)
NERVE_DECL(BootScene, InitError)
NERVE_DECL(BootScene, LoadSaveData)
NERVE_DECL(BootScene, WaitLoadDoneResource)
NERVES_MAKE_NOSTRUCT(BootScene, Init, LoadEnd, InitError, LoadSaveData, WaitLoadDoneResource)
}

BootScene::BootScene() : al::Scene("起動シーン") {}

BootScene::~BootScene() {
    if (mLiveActorKit)
        mLiveActorKit->getEffectSystem()->endScene();
}

void BootScene::init(const al::SceneInitInfo& rInfo) {
    mGameData = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    mMainViewport = new sead::Viewport(*Application::instance()->getFramework()->getMethodFrameBuffer(6));
    mSubViewport = new sead::Viewport(*Application::instance()->getFramework()->getMethodFrameBuffer(9));
    initLiveActorKit(rInfo, 8, 1, 1, 0);
    initLayoutKit(rInfo);
    al::LayoutInitInfo layoutInfo;
    al::initLayoutInitInfo(&layoutInfo, this, rInfo);
    al::PlacementInfo placement;
    al::ActorInitInfo actorInfo;
    al::initActorInitInfo(&actorInfo, this, &placement, &layoutInfo, false);
    mWipe = new al::WipeSimple("赤カーテン", "WipeCurtainOpening", layoutInfo, nullptr);
    mWindowBoot = new al::SimpleLayoutAppearWaitEnd("起動画面", "WindowBoot", layoutInfo, nullptr, false);
    mControllerGuide = new al::SimpleLayoutAppearWaitEnd("操作ガイド", "WindowControllerGuide", layoutInfo, nullptr, false);
    endInit(actorInfo, nullptr);
    initNerve(&NrvBootSceneInit, 0);
}

void BootScene::appear() {
    al::Scene::appear();
    mWindowBoot->appear();
    mGuideFrames = 0;
    mFrames = 0;
    al::setNerve(this, &NrvBootSceneInit);
}

void BootScene::control() {
    ++mFrames;
    al::updateKit(this);
}

void BootScene::drawMain_() const {
    static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework())->clearFrameBuffer();
    auto* framework = static_cast<al::GameFrameworkNx*>(Application::instance()->getFramework());
    mMainViewport->setByFrameBuffer(*framework->getCurrentRenderBuffer());
    alSystemKitFunction::applyViewportTop(*mMainViewport);
    al::drawKit(this, "２Ｄベース（メイン画面）");
    al::tryChangeShaderMode(al::GameFrameworkNx::getAglDrawContext(), agl::cShaderMode_UniformBlock);
}

void BootScene::drawSub_() const {}

bool BootScene::tryEnd() {
    if (mGuideFrames < 240)
        return false;
    al::setNerve(this, &NrvBootSceneLoadEnd);
    return true;
}

void BootScene::updateGuide() {
    if (!mWindowBoot->isAlive()) {
        if (!mControllerGuide->isAlive()) {
            mControllerGuide->appear();
            mGuideFrames = 0;
        }
        ++mGuideFrames;
    }
}

void BootScene::exeInit() {
    if (al::isFirstStep(this)) {
        mWipe->startCloseEnd();
        mWindowBoot->end();
        SaveDataAccessFunction::startSaveDataInit(mGameData);
        mIsSaveInit = true;
    }
    updateGuide();
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameData, mIsSaveInit)) {
        mIsSaveInit = false;
        al::setNerve(this, &NrvBootSceneLoadSaveData);
        return;
    }
    if (SaveDataAccessFunction::isWaitShowError(mGameData) && mControllerGuide->isWait())
        al::setNerve(this, &NrvBootSceneInitError);
}

void BootScene::exeInitError() {
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameData, false)) {
        mIsSaveDataLoaded = true;
        al::setNerve(this, &NrvBootSceneWaitLoadDoneResource);
    }
}

void BootScene::exeLoadSaveData() {
    if (al::isFirstStep(this))
        SaveDataAccessFunction::startSaveDataRead(mGameData, false);
    updateGuide();
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameData, false)) {
        PlayLogFunction::setPlayStart(GameDataHolderWriter(mGameData));
        mIsSaveDataLoaded = true;
        al::setNerve(this, &NrvBootSceneWaitLoadDoneResource);
    }
}

void BootScene::exeWaitLoadDoneResource() {
    updateGuide();
}

void BootScene::exeLoadEnd() {
    if (al::isFirstStep(this) && mControllerGuide->isAlive())
        mControllerGuide->end();
    if (!mControllerGuide->isAlive())
        kill();
}

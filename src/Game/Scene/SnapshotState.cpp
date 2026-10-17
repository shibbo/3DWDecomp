#include "Scene/SnapshotState.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "MapObj/DrcAssistDirectorList.hpp"
#include "MapObj/StampDirector.hpp"
#include <gfx/seadProjection.h>
#include "Layout/Switch/SnapshotLayout.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/SnapShotCameraCtrl.hpp"
#include "Library/Clipping/ClippingDirectorBase.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Draw/ViewRenderer.hpp"
#include "Library/Execute/ExecuteDirector.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Project/LiveActor/ActorExecuteFunction.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/PostProcessing/PostProcessingFilter.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/Stamp.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Draw/GraphicsStressDirector.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
namespace {
NERVE_DECL(SnapshotState, Wait)
NERVE_DECL(SnapshotState, Finish)
NERVES_MAKE_NOSTRUCT(SnapshotState, Wait, Finish)
} // namespace
SnapshotState::SnapshotState(SnapshotLayout* layout, al::Scene* scene,
                             DrcAssistDirectorList* assist, rc::StampDirector* stamps,
                             PlayerAliveWatcher* players)
    : al::NerveStateBase("SnapshotState"), mScene(scene), mLayout(layout), mAssist(assist),
      mStamps(stamps), mPlayers(players) {
    stamps->setAudioKeeperUser(layout);
    mStamps->setSnapshotLayout(mLayout);
}
void SnapshotState::init() { initNerve(&NrvSnapshotStateWait, 0); }
void SnapshotState::appear() {
    mIsDead = false;
    mStamps->start();
    al::LiveActorKit* kit = mScene->getLiveActorKit();
    al::GraphicsSystemInfo* graphics = kit->getGraphicsSystemInfo();
    graphics->getGraphicsStressDirector()->setForceDisable(true);
    graphics->getViewRenderer()->setForceFilterAA(true);
    graphics->getGraphicsStressDirector()->setForceStressOff(true);
    al::pauseJointControllers();
    kit->getEffectSystem()->setStopCalc(true);
    al::setNerve(this, &NrvSnapshotStateWait);
}
void SnapshotState::kill() {
    mIsDead = true;
    al::LiveActorKit* kit = mScene->getLiveActorKit();
    al::GraphicsSystemInfo* graphics = kit->getGraphicsSystemInfo();
    graphics->getGraphicsStressDirector()->setForceDisable(false);
    graphics->getViewRenderer()->setForceFilterAA(false);
    graphics->getGraphicsStressDirector()->setForceStressOff(false);
    kit->getEffectSystem()->setStopCalc(false);
    al::resumeJointControllers();
    mStamps->finish();
}
void SnapshotState::exeWait() {
    al::LiveActorKit* kit = mScene->getLiveActorKit();
    if (al::isFirstStep(this)) {
        if (kit->getCameraDirector_RS())
            kit->getCameraDirector_RS()->startSnapShotMode(false);
        else
            kit->getCameraDirector()->startSnapshotMode();
        mLayout->appear();
        alAudioSystemFunction::pauseSystem(mScene->getAudioDirector(), mScene, true, 0);
        mAssist->startSnapshotMode();
    }
    al::PostProcessingFilter* filter =
        mScene->getLiveActorKit()->getGraphicsSystemInfo()->getPostProcessingFilter();
    if (al::isFirstStep(this)) {
        mDefaultFilter = 0;
        mLayout->updateFilterText(filter->getPresetId());
        filter->validate();
    }
    s32 port = al::getMainControllerPort();
    f32 roll = 0.0f;
    if (auto* camera = kit->getCameraDirector_RS()) {
        camera->enableSnapShotRoll(!mAssist->isPlayerHoldingStamp());
        if (camera->getCurrentTicket() && camera->getCurrentTicket()->getPoser() &&
            camera->getCurrentTicket()->getPoser()->getSnapShotCtrl())
            roll = camera->getCurrentTicket()->getPoser()->getSnapShotCtrl()->getRollDegree();
    } else if (kit->getCameraDirector()) {
        roll = kit->getCameraDirector()->getSnapshotRollDeg();
    }
    mLayout->updateLogoAngle(roll);
    rc::StampIncrementState increment = rc::StampIncrementState_None;
    rc::StampRotateState rotate = rc::StampRotateState_None;
    if (mAssist->isPlayerHoldingStamp()) {
        if (al::isPadTriggerZL(port) || al::isPadHoldZL(port)) {
            if (!(al::isPadTriggerZR(port) || al::isPadHoldZR(port)))
                rotate = rc::StampRotateState_Left;
        } else {
            rotate = (al::isPadTriggerZR(port) || al::isPadHoldZR(port))
                         ? rc::StampRotateState_Right
                         : rc::StampRotateState_None;
        }
        if (al::isPadTriggerLeft(port) || al::isPadRepeatLeft(port))
            increment = rc::StampIncrementState_Decrement;
        else if (al::isPadTriggerRight(port) || al::isPadRepeatRight(port))
            increment = rc::StampIncrementState_Increment;
    } else {
        bool change = false;
        if (al::isPadTriggerLeft(port)) {
            filter->decrementPreset();
            change = true;
        } else if (al::isPadTriggerRight(port)) {
            filter->incrementPreset();
            change = true;
        }
        if (change) {
            if (filter->getPresetId() == mDefaultFilter)
                al::startSe(mLayout, "PgFilterSelectNone");
            else
                al::startSe(mLayout, "PgFilterSelect");
            mLayout->updateFilterText(filter->getPresetId());
        }
    }
    if (mStamps->update(rotate, increment)) {
        mLayout->updatePreviewImage(mStamps->getActiveStamp());
        mLayout->rotatePreviewImage(mStamps->getActiveStamp()->getUIRotation() * -360.0f);
    } else if (mStamps->getActiveStamp()) {
        mLayout->rotatePreviewImage(mStamps->getActiveStamp()->getUIRotation() * -360.0f);
    }
    if (al::isPadTriggerB(port) || al::isPadTriggerDown(port)) {
        if (kit->getCameraDirector_RS())
            kit->getCameraDirector_RS()->endSnapShotMode();
        else
            kit->getCameraDirector()->endSnapshotMode();
        filter->invalidate();
        mLayout->end();
        mStamps->hideAllStamps();
        alAudioSystemFunction::pauseSystem(mScene->getAudioDirector(), mScene, false, 0);
        mAssist->endSnapshotMode();
        al::setNerve(this, &NrvSnapshotStateFinish);
        updateGraphics();
    } else {
        updateGraphics();
        filter->update();
        if (al::isPadTriggerTouch(rc::calcTouchPanelPortByPortNum(port))) {
            const sead::Projection* projection =
                kit->getCameraDirector_RS() ? &kit->getCameraDirector_RS()->getProjectionMain()
                                            : kit->getCameraDirector()->getProjection();
            sead::Vector3f position;
            projection->screenPosToCameraPos(&position, sead::Vector2f(640.0f, 360.0f));
        }
    }
}
void SnapshotState::updateGraphics() {
    al::LiveActorKit* kit = mScene->getLiveActorKit();
    s32 count = al::getPlayerNumMax(kit->getPlayerHolder());
    for (s32 i = 0; i < count; i++)
        static_cast<PlayerActor*>(al::getPlayerActor(kit->getPlayerHolder(), i))
            ->getInput()
            ->invalidateFrame(2);
    if (kit->getCameraDirector_RS())
        kit->getCameraDirector_RS()->executePaused();
    else {
        al::CameraDirector* camera = kit->getCameraDirector();
        camera->update(mAssist->isPlayerHoldingStamp());
        camera->_154 = mPlayers ? mPlayers->isEnableGyroCamera() : true;
        for (s32 i = 0; i < rc::getControlUserNumMax(); i++) {
            s32 port = rc::getPadPortByUserId(i);
            if (rc::isActiveControlUser(mScene, i))
                camera->validUserCameraControlByPort(port);
            else
                camera->invalidUserCameraControlByPort(port);
        }
    }
    rc::updateDrcAssistDirector(mScene);
    kit->clearGraphicsRequest();
    al::updateKitList(mScene, "OceanWaterRequest");
    al::updateKitList(mScene, "Clipping");
    al::updateKitListPaused(mScene, "ModelUpdate");
    al::updateKitListPaused(mScene, "コリジョン地形");
    al::updateKitListPaused(mScene, "コリジョン地形装飾");
    al::updateKitListPaused(mScene, "コリジョン地形オブジェ");
    al::updateKitListPaused(mScene, "コリジョンアイテム");
    al::updateKitListPaused(mScene, "地形オブジェ");
    al::updateKitListPaused(mScene, "地形オブジェ[Movement]");
    al::updateKitListPaused(mScene, "地形オブジェ装飾");
    al::updateKitListPaused(mScene, "乗り物");
    al::updateKitListPaused(mScene, "乗り物装飾");
    al::updateKitListPaused(mScene, "敵");
    al::updateKitListPaused(mScene, "遠景");
    al::updateKitListPaused(mScene, "ＮＰＣ");
    al::updateKitListPaused(mScene, "ＮＰＣ装飾");
    al::updateKitListPaused(mScene, "アイテム");
    al::updateKitListPaused(mScene, "アイテム装飾");
    al::updateKitList(mScene, "DRCアシスト");
    al::updateKitList(mScene, "DRCアシスト同期グループ");
    if (al::ClippingDirectorBase::sCollisionForcedOn && al::isNerve(this, &NrvSnapshotStateWait) &&
        al::isStep(this, 2))
        al::updateKitList(mScene, "コリジョンディレクター");
    al::updateKitListPaused(mScene, "エフェクトオブジェ");
    al::executeUpdateListPaused(kit, "シャドウマスク");
    al::executeUpdateListPaused(kit, "グラフィックス要求者");
    al::updateKitList(mScene, "Stamp");
    kit->updateGraphics(true);
    if (kit->getAreaObjDirector())
        kit->getAreaObjDirector()->update();
    al::updateKitList(mScene, "２Ｄ（ポーズ無視）");
    al::updateKitListPaused(mScene, "Water");
    al::updateEffect(mScene);
    kit->getExecuteDirector()->finishExecute();
    kit->getClippingDirector()->executeRequestAsyncUpdate();
    kit->updateReducedBufferEffect();
}
void SnapshotState::exeFinish() {
    updateGraphics();
    if (!mLayout->isAlive())
        kill();
}

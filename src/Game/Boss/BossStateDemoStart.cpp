#include "Boss/BossStateDemoStart.hpp"
#include "Demo/DemoPlayerController.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Scene/SceneEventMessageSender.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

namespace rc {
void disappearCameraChangeLayout(const al::IUseSceneObjHolder* pHolder);
void appearCameraChangeLayout(const al::IUseSceneObjHolder* pHolder);
void stopAndHideGhostPlayerAll(const al::IUseSceneObjHolder* pHolder);
void restartGhostPlayerAll(const al::IUseSceneObjHolder* pHolder);
bool requestStartDemoPlayer(const al::LiveActor* pActor);
void requestEndDemoPlayer(const al::LiveActor* pActor);
void hideDemoPlayerAll(const al::LiveActor* pActor);
void showDemoPlayerAll(const al::LiveActor* pActor);
void replaceDemoPlayerAll(const al::LiveActor* pActor, const sead::Vector3f& rTrans,
                          const sead::Quatf& rQuat, f32 offset);
s32 getDemoPlayerNum(const al::LiveActor* pActor);
DemoPlayerController* getDemoPlayer(const al::LiveActor* pActor, s32 index);
bool tryCancelBossDemo(const al::LiveActor* pActor);
void setAlreadyShowBossDemo(const al::LiveActor* pActor);
}  // namespace rc

namespace {
NERVE_DECL(BossStateDemoStart, DemoStart);
NERVE_DECL(BossStateDemoStart, Demo);
NERVE_DECL(BossStateDemoStart, Fade);
NERVE_DECL(BossStateDemoStart, Cancel);
NERVES_MAKE_NOSTRUCT(BossStateDemoStart, DemoStart, Demo, Fade, Cancel)
}  // namespace

/**
 * @brief Stores the demo camera parameters.
 * @param pCameraInfo Animation camera to play.
 * @param pActor Boss actor the camera is attached to.
 * @param pActionName Camera animation name.
 * @param cancelFrame Step from which the demo can be cancelled, or 0 to disallow cancelling.
 * @param pBaseMtx Camera base matrix, or nullptr to use the actor's base matrix.
 */
BossDemoStartInfo::BossDemoStartInfo(al::CameraInfo* pCameraInfo, al::LiveActor* pActor,
                                     const char* pActionName, s32 cancelFrame,
                                     const sead::Matrix34f* pBaseMtx)
    : mCameraInfo(pCameraInfo), mActor(pActor), mActionName(pActionName), _18(-1),
      mCancelFrame(cancelFrame), mBaseMtx(pBaseMtx) {}

/**
 * @brief Creates the boss start demo state, its skip prompt and its fade wipe.
 * @param pActor Boss actor that owns the state.
 * @param rInfo Actor initialization data.
 * @param pInfo Demo camera parameters.
 */
BossStateDemoStart::BossStateDemoStart(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                                       BossDemoStartInfo* pInfo)
    : al::ActorStateBase("ボス用デモ開始状態", pActor), mInfo(pInfo),
      mStartRecorderId(""), mEndRecorderId("") {
    initNerve(&NrvBossStateDemoStartDemoStart, 0);
    mAudioDirector = al::getAudioDirector(rInfo);

    if (al::calcLinkChildNum(rInfo, "PlayerPos") >= 1) {
        al::tryGetLinksQT(&mReturnQuat, &mReturnTrans, rInfo, "PlayerPos");
    } else {
        al::tryGetTrans(&mReturnTrans, rInfo);
        mReturnTrans += sead::Vector3f(0.0f, 0.0f, 1000.0f);
        al::tryGetQuat(&mReturnQuat, rInfo);
    }

    al::PlacementId placementId;
    al::tryGetPlacementID(&placementId, rInfo);
    const char* zoneId = placementId.mZoneID != nullptr ? placementId.mZoneID : "";
    mStartRecorderId.format("%s%sS", placementId.mPlacementID, zoneId);
    zoneId = placementId.mZoneID != nullptr ? placementId.mZoneID : "";
    mEndRecorderId.format("%s%sE", placementId.mPlacementID, zoneId);

    mSkipLayout = new DemoSkipLayout(al::getLayoutInitInfo(rInfo), false);
    mWipe = new al::WipeSimple("黒フェードワイプ", "WipeFadeBlack", al::getLayoutInitInfo(rInfo),
                               "Demo");
}

/** @brief Shows the skip prompt and starts the demo, hiding the camera change layout. */
void BossStateDemoStart::appear() {
    al::NerveStateBase::appear();
    mSkipLayout->appear();
    al::setNerve(this, &NrvBossStateDemoStartDemoStart);
    al::requestCaptureScreenCover(mHostActor, 4);
    rc::disappearCameraChangeLayout(mHostActor);
}

/**
 * @brief Sets where the players are placed when the demo ends.
 * @param rTrans Return position.
 */
void BossStateDemoStart::setReturnTrans(const sead::Vector3f& rTrans) {
    mReturnTrans = rTrans;
}

/**
 * @brief Sets where and how the players are placed when the demo ends.
 * @param rTrans Return position.
 * @param rQuat Return orientation.
 */
void BossStateDemoStart::setReturnTransAndQuat(const sead::Vector3f& rTrans,
                                               const sead::Quatf& rQuat) {
    mReturnTrans = rTrans;
    mReturnQuat = rQuat;
}

/** @brief Pauses the ghost players and waits until the players enter demo mode. */
void BossStateDemoStart::exeDemoStart() {
    if (al::isFirstStep(this)) {
        rc::stopAndHideGhostPlayerAll(mHostActor);
        rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mStartRecorderId.cstr(), true);
    }

    if (rc::requestStartDemoPlayer(mHostActor)) {
        al::setNerve(this, &NrvBossStateDemoStartDemo);
    }
}

/** @brief Plays the demo camera and waits for its end, a skip, or a cancel. */
void BossStateDemoStart::exeDemo() {
    if (al::isFirstStep(this)) {
        const al::CameraInfo* cameraInfo = mInfo->mCameraInfo;
        al::LiveActor* actor = mInfo->mActor;
        const sead::Matrix34f* baseMtx = mInfo->mBaseMtx;
        const char* actionName = mInfo->mActionName;
        const al::IUseCamera* camera = actor;
        al::startAnimCamera(camera, cameraInfo, actionName,
                            baseMtx != nullptr ? baseMtx : actor->getBaseMtx(), 0);
        rc::hideDemoPlayerAll(mHostActor);
        al::tryOnStageSwitch(mHostActor, "SwitchDemoStartOn");
        rc::sendSceneEventMessageStartDemoBossStart(mHostActor);

        s32 playerNum = rc::getDemoPlayerNum(mHostActor);
        for (s32 i = 0; i < playerNum; i++) {
            rc::getDemoPlayer(mHostActor, i)->stopSklAnimAndDeleteEffect();
        }
    }

    auto ports = rc::getActiveInputPortList(GameDataHolderAccessor(mWipe));
    if (mSkipLayout->isSkip(sead::BitFlag<u16>(ports))) {
        al::setNerve(this, &NrvBossStateDemoStartFade);
        return;
    }

    if (al::isEndAnimCamera(mInfo->mCameraInfo)) {
        endDemo(false);
        return;
    }

    s32 cancelFrame = mInfo->mCancelFrame;
    if (cancelFrame >= 1 && al::isGreaterEqualStep(this, cancelFrame) &&
        rc::tryCancelBossDemo(mHostActor)) {
        al::requestCaptureScreenCover(mHostActor, 3);
        al::setNerve(this, &NrvBossStateDemoStartCancel);
    }
}

/**
 * @brief Ends the demo camera and returns the players and ghost players to gameplay.
 * @param isSkip Whether the demo was skipped or cancelled instead of played to the end.
 */
void BossStateDemoStart::endDemo(bool isSkip) {
    _44 = isSkip;
    if (mInfo->mCancelFrame >= 1) {
        rc::setAlreadyShowBossDemo(mHostActor);
    }

    al::endCamera(mInfo->mActor, mInfo->mCameraInfo, isSkip ? 0 : mInfo->_18);
    al::tryOffStageSwitch(mHostActor, "SwitchDemoStartOn");
    mSkipLayout->kill();
    if (isDead()) {
        return;
    }

    rc::showDemoPlayerAll(mHostActor);
    rc::replaceDemoPlayerAll(mHostActor, mReturnTrans, mReturnQuat, 150.0f);
    rc::requestEndDemoPlayer(mHostActor);
    rc::restartGhostPlayerAll(mHostActor);
    rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr(), false);
    rc::tryStartFromObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr());
    rc::appearCameraChangeLayout(mHostActor);
    kill();
}

/** @brief Fades out after a skip, ends the demo and fades back in. */
void BossStateDemoStart::exeFade() {
    if (al::isFirstStep(this)) {
        mWipe->startClose(-1);
    }

    if (mWipe->isCloseEnd()) {
        endDemo(true);
        mWipe->startOpen(-1);
        mSkipLayout->kill();
    }
}

/** @brief Ends a cancelled demo immediately. */
void BossStateDemoStart::exeCancel() {
    endDemo(true);
    rc::appearCameraChangeLayout(mHostActor);
    kill();
}

#include "Library/Play/Placement/PlacementInfo.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Boss/GateKeeperStateDemo.hpp"
#include "Boss/Bunbun.hpp"
#include "Boss/Punpun.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/ShardsWatcher.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
NERVE_DECL(GateKeeperStateDemo, InitOnGround);
NERVE_DECL(GateKeeperStateDemo, Demo);
NERVE_DECL(GateKeeperStateDemo, DemoRequest);
NERVES_MAKE_NOSTRUCT(GateKeeperStateDemo, InitOnGround, Demo, DemoRequest)
} // namespace
/** @brief Creates an opening cutscene with its camera and recorder IDs. @param pHost Boss actor.
 * @param rInfo Initialization data. @param pParam Cutscene settings. @param pCamera Cutscene
 * camera. */
GateKeeperStateDemo::GateKeeperStateDemo(al::LiveActor *pHost, const al::ActorInitInfo &rInfo,
                                         const GateKeeperStateDemoParam *pParam,
                                         al::CameraInfo *pCamera)
    : ActorStateBase("ゲートキーパー用デモ開始状態", pHost), mParam(pParam), mCameraInfo(pCamera),
      mStartRecorderId(""), mEndRecorderId(""), mCancelPlayerKoura(false) {
    initNerve(&NrvGateKeeperStateDemoInitOnGround, 0);
    al::PlacementId id;
    al::tryGetPlacementID(&id, rInfo);
    mStartRecorderId.format("%s%sS", id.mPlacementID, id.mZoneID ? id.mZoneID : "");
    mEndRecorderId.format("%s%sE", id.mPlacementID, id.mZoneID ? id.mZoneID : "");
}
/** @brief Creates an opening cutscene with its camera and recorder IDs. @param pHost Boss actor.
 * @param rInfo Initialization data. @param pParam Cutscene settings. @param pCamera Cutscene
 * camera. */
GateKeeperStateDemo::GateKeeperStateDemo(al::LiveActor *pHost, const al::ActorInitInfo &rInfo,
                                         const GateKeeperStateDemoParam *pParam,
                                         al::CameraTicket *pCamera)
    : ActorStateBase("ゲートキーパー用デモ開始状態", pHost), mParam(pParam), mCameraTicket(pCamera),
      mStartRecorderId(""), mEndRecorderId("") {
    initNerve(&NrvGateKeeperStateDemoInitOnGround, 0);
    if (pHost->mActorSceneInfo && pHost->mActorSceneInfo->isSingleMode &&
        al::calcLinkChildNum(rInfo, "PlayerPos") > 0) {
        al::PlacementInfo info;
        al::getLinksInfoByIndex(&info, rInfo, "PlayerPos", 0);
        mHasPlayerPos = true;
        al::tryGetTrans(&mPlayerPos, info);
        al::tryGetFront(&mPlayerFront, info);
    }
    al::PlacementId id;
    al::tryGetPlacementID(&id, rInfo);
    mStartRecorderId.format("%s%sS", id.mPlacementID, id.mZoneID ? id.mZoneID : "");
    mEndRecorderId.format("%s%sE", id.mPlacementID, id.mZoneID ? id.mZoneID : "");
}
/** @brief Destroys the opening cutscene state. */
GateKeeperStateDemo::~GateKeeperStateDemo() = default;
/** @brief Suspends ghost players and disables bubble revival for the cutscene. */
void GateKeeperStateDemo::appear() {
    mCancelPlayerKoura = false;
    mIsDead = false;
    rc::setDisableReviveBubbleForAllPlayer(mHostActor);
    rc::stopAndHideGhostPlayerAll(mHostActor);
    rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mStartRecorderId.cstr(), true);
    if (mCameraInfo)
        rc::disappearCameraChangeLayout(mHostActor);
}
/** @brief Restores bubble revival and camera-change UI. */
void GateKeeperStateDemo::kill() {
    mIsDead = true;
    rc::resetDisableReviveBubbleForAllPlayer(mHostActor);
    if (mCameraInfo)
        rc::appearCameraChangeLayout(mHostActor);
}
/** @brief Checks whether the opening demo is starting. @return Whether this is its first step. */
bool GateKeeperStateDemo::isDemoFirstStep() const {
    return al::isNerve(this, &NrvGateKeeperStateDemoDemo) &&
           (al::isNewNerve(this) || al::isFirstStep(this));
}
/** @brief Waits for players to reach a safe cutscene position. */
void GateKeeperStateDemo::exeInitOnGround() {
    if (mHasPlayerPos && ShardsWatcher::sIsFinalShardGetPending)
        return;
    for (int i = 0; i < al::getPlayerNumMax(mHostActor); i++) {
        auto *player = al::getPlayerActor(mHostActor, i);
        if (rc::isPlayerDeadOrBubble(player))
            continue;
        if (mHasPlayerPos) {
            if (rc::isPlayerInKoura(player)) {
                mCancelPlayerKoura = true;
                continue;
            }
            if (rc::isPlayerBinded(player) || !rc::isPlayerOnGroundOrWater(player))
                return;
        } else if (rc::isPlayerBinded(player) || !rc::isPlayerOnGround(player))
            return;
    }
    if (mHasPlayerPos)
        al::requestCaptureScreenCover(mHostActor, 5);
    if (mIsControlPlayer)
        al::setNerve(this, &NrvGateKeeperStateDemoDemoRequest);
    else {
        auto *punpun = static_cast<Punpun *>(mHostActor);
        if (punpun && punpun->getBunbun() && punpun->getBunbun()->isDemoStarted())
            al::setNerve(this, &NrvGateKeeperStateDemoDemoRequest);
    }
}
/** @brief Requests player control and pauses the Fury encounter when needed. */
void GateKeeperStateDemo::exeDemoRequest() {
    if (mHasPlayerPos && ShardsWatcher::sIsFinalShardGetPending)
        return;
    if (mCancelPlayerKoura) {
        for (int i = 0; i < al::getPlayerNumMax(mHostActor); i++) {
            auto *player = al::getPlayerActor(mHostActor, i);
            if (!rc::isPlayerDeadOrBubble(player) && rc::isPlayerInKoura(player))
                rc::cancelPlayerInKoura(player);
        }
        mCancelPlayerKoura = false;
    }
    if (mIsControlPlayer) {
        if (mHasPlayerPos) {
            auto *controller = DisasterModeController::tryGetController(mHostActor);
            if (controller && !controller->isReadyForDemo())
                return;
            if (!rc::requestStartDemoPlayer(mHostActor))
                return;
            if (controller)
                controller->pauseWithSuperBowser();
            rc::addDemoActor(mHostActor);
        } else if (!rc::requestStartDemoPlayer(mHostActor))
            return;
    }
    al::setNerve(this, &NrvGateKeeperStateDemoDemo);
}
/** @brief Runs the opening camera and actions, then restores normal play. */
void GateKeeperStateDemo::exeDemo() {
    if (al::isFirstStep(this)) {
        if (mCameraInfo)
            al::startAnimCamera(mHostActor, mCameraInfo, mParam->mCameraAction,
                                mParam->mCameraStartFrames);
        else if (mCameraTicket && mIsControlPlayer) {
            if (mHasPlayerPos) {
                auto *player = rc::findNearestActivePlayerActor(mHostActor);
                if (player) {
                    rc::setPlayerTrans(player, mPlayerPos);
                    al::setTrans(player, mPlayerPos);
                    rc::setPlayerFrontVec(player, mPlayerFront);
                }
            }
            al::startAnimCamera_RS(mHostActor, mCameraTicket, mParam->mCameraAction,
                                   mParam->mCameraStartFrames);
            al::setAnimCameraLookAtOffset(mCameraTicket, mLookAtOffset);
            if (mIsCameraTicket) {
                auto *bunbun = static_cast<Bunbun *>(mHostActor);
                if (bunbun && bunbun->getPunpun())
                    rc::addDemoActor(bunbun->getPunpun());
            }
            mIsDemoStarted = true;
        }
        al::startAction(mHostActor, mParam->mActionName);
        rc::startActionDemoPlayerAll(mHostActor, mParam->mPlayerAction);
        al::tryOnStageSwitch(mHostActor, "SwitchDemoStartOn");
    }
    if (al::isStep(this, 1)) {
        al::showModelIfHide(mHostActor);
        if (mHasPlayerPos) {
            auto *junior = PlayerKoopaJr::tryGetPlayerKoopaJr(mHostActor);
            if (junior)
                junior->startMidBossDemo();
        }
    }
    if (al::isActionEnd(mHostActor)) {
        kill();
        if (mCameraInfo) {
            al::endCamera(mHostActor, mCameraInfo, mParam->mCameraEndFrames);
            rc::requestEndDemoPlayer(mHostActor);
        } else if (mCameraTicket && mIsControlPlayer) {
            al::endCamera_RS(mHostActor, mCameraTicket, mParam->mCameraEndFrames, false);
            bool hasPlayerPos = mHasPlayerPos;
            rc::requestEndDemoPlayer(mHostActor);
            if (hasPlayerPos) {
                auto *controller = DisasterModeController::tryGetController(mHostActor);
                if (controller)
                    controller->resumeWithSuperBowser();
            }
        }
        al::tryOffStageSwitch(mHostActor, "SwitchDemoStartOn");
        rc::restartGhostPlayerAll(mHostActor);
        rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr(), false);
        rc::tryStartFromObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr());
    }
}
/** @brief Sets the opening actions and camera transition lengths. */
GateKeeperStateDemoParam::GateKeeperStateDemoParam(const char *pActionName, s32 cameraStartFrames)
    : mActionName(pActionName), mPlayerAction("WaitBattle"), mCameraAction("DemoBattleStart"),
      mCameraStartFrames(cameraStartFrames), mCameraEndFrames(60) {}

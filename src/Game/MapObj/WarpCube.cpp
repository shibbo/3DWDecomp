#include "MapObj/WarpCube.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>

#include "Layout/CounterWarpCube.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/ActorStateDemoCamera.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "MapObj/WarpCubeBindPuppeteer.hpp"
#include "MapObj/WarpCubeLockedPiece.hpp"
#include "MapObj/WarpObjUtil.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(WarpCube, Wait)
NERVE_DECL(WarpCube, EndPieceDemo)
NERVE_DECL(WarpCube, WaitPiece)
NERVE_DECL(WarpCube, AppearWithCamera)

/**
 * @brief Appearance started by placement or by a stage switch.
 */
class WarpCubeNrvAppear : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the warp box.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<WarpCube>()->exeAppear();
    }
};

/**
 * @brief Appearance during Bowser's chase.
 */
class WarpCubeNrvKoopaChaseAppear : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the warp box.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<WarpCube>()->exeAppear();
    }
};

NERVE_DECL(WarpCube, PlayerIn)
NERVE_DECL(WarpCube, AllBindForceInit)

/**
 * @brief Appearance once every locked piece was collected.
 */
class WarpCubeNrvUnlockAppear : public al::Nerve {
public:
    /**
     * @brief Run the nerve.
     * @param pKeeper The nerve keeper of the warp box.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<WarpCube>()->exeAppear();
    }
};

NERVE_DECL(WarpCube, Controlled)
NERVE_DECL(WarpCube, Disappear)
NERVE_DECL(WarpCube, OutSignWithCamera)
NERVE_DECL(WarpCube, OutSign)
NERVE_DECL(WarpCube, WaitStartWarp)
NERVE_DECL(WarpCube, AllBindForce)
NERVE_DECL(WarpCube, AllBindForceIn)
NERVE_DECL(WarpCube, Out)
NERVE_DECL(WarpCube, OutSignWithCameraWait)

NERVES_MAKE_NOSTRUCT(WarpCube, Wait, EndPieceDemo, WaitPiece, AppearWithCamera, Appear,
                     KoopaChaseAppear, PlayerIn, AllBindForceInit, UnlockAppear, Controlled,
                     Disappear, OutSignWithCamera, OutSign, WaitStartWarp, AllBindForce,
                     AllBindForceIn, Out, OutSignWithCameraWait)

/// How the bind of the players ends when they jump out of the destination box.
PlayerBindEndParam sWarpEndParam = {{}, 0, 30, true, true, true, 0, 1.2f, 30};

/// Offset from the destination box where the warped players are placed.
const sead::Vector3f cDestOffset(0.0f, 100.0f, 0.0f);

/// Shadow mask size of a box while players go in or come out of it.
const sead::Vector3f cShadowMaskSizeWarp(150.0f, 500.0f, 150.0f);

/**
 * @brief Blend the shadow mask size of an actor over a range of action frames.
 * @param pActor The actor whose shadow is updated.
 * @param rFrom Size at the start of the range.
 * @param rTo Size at the end of the range.
 * @param actionFrame Current action frame.
 * @param start First frame of the range.
 * @param end Last frame of the range.
 */
ALWAYS_INLINE void lerpShadowMaskSize(al::LiveActor* pActor, const sead::Vector3f& rFrom,
                                      const sead::Vector3f& rTo, f32 actionFrame, s32 start,
                                      s32 end) {
    s32 frame = static_cast<s32>(actionFrame);

    if (frame >= start && frame <= end) {
        sead::Vector3f size = {1.0f, 1.0f, 1.0f};
        f32 rate = al::normalize(static_cast<f32>(frame), static_cast<f32>(start),
                                 static_cast<f32>(end));
        al::lerpVec(&size, rFrom, rTo, rate);
        al::setShadowMaskSize(pActor, "シャドウマスク", size);
    }
}
}  // namespace

/**
 * @brief Construct a warp box.
 * @param pName Name of the actor.
 */
WarpCube::WarpCube(const char* pName) : al::LiveActor(pName), mPlacementId(new al::PlacementId()) {}

/**
 * @brief Initialize the box, its destination, its locked pieces and its cameras.
 * @param rInfo The actor init info.
 */
void WarpCube::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvWarpCubeWait, 2);
    mDestCube = this;
    al::tryAddDisplayOffset(this, rInfo);
    mPuppeteerGroup = new BindPuppeteerGroup("ワープキューブバインド操作グループ",
                                             al::getPlayerNumMax(this));

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        mPuppeteerGroup->registerPuppeteer(
            new WarpCubeBindPuppeteer("ワープキューブバインド操作", rInfo));
    }

    mBindPuppeteers.allocBuffer(mPuppeteerGroup->getPuppeteerNumMax(), nullptr);

    if (al::calcLinkChildNum(rInfo, "DestCube") > 0) {
        auto* destCube = new WarpCube("ワープキューブ");
        al::initLinksActor(destCube, rInfo, "DestCube", 0);
        destCube->mDestCube = this;
        mDestCube = destCube;
    }

    al::tryGetArg(reinterpret_cast<s32*>(&mType), rInfo, "Type");
    al::tryGetArg(&mIsOutSpeedZero, rInfo, "IsOutSpeedZero");

    if (!al::tryGetArg(&mIsBgmChangeWhenPieceComplete, rInfo, "IsBgmChangeWhenPieceComplete")) {
        mIsBgmChangeWhenPieceComplete = false;
    }

    al::tryGetPlacementID(mPlacementId, rInfo);
    s32 pieceNum = al::calcLinkChildNum(rInfo, "WarpCubeLockedPiece");

    if (pieceNum >= 1) {
        mLockedModel = new al::LiveActor("ワープキューブ(ロック)");
        al::initActorWithArchiveNameWithPlacementInfo(mLockedModel, rInfo, "WarpCubeLocked",
                                                      nullptr);
        al::tryAddDisplayOffset(mLockedModel, rInfo);
        al::startAction(mLockedModel, "Wait");
        mLockedModel->makeActorAppeared();
        mCounter = new CounterWarpCube(al::getLayoutInitInfo(rInfo), this, pieceNum);

        for (s32 i = 0; i < pieceNum; i++) {
            auto* piece = new WarpCubeLockedPiece("ワープキューブのかけら");
            al::initLinksActor(piece, rInfo, "WarpCubeLockedPiece", i);
            piece->setCounter(mCounter);
        }

        bool isUseGetPieceCamera = false;

        if (al::tryGetArg(&isUseGetPieceCamera, rInfo, "IsUseGetPieceCamera") &&
            isUseGetPieceCamera) {
            mEndPieceDemoParam = new ActorStateDemoCameraParam(
                sead::Mathi::max(static_cast<s32>(al::getActionFrameMax(this, "Appear")), 115), 75,
                60, al::getTransPtr(this), al::getTransPtr(this));
            mEndPieceDemo = new ActorStateDemoCamera(this, rInfo, "GetPiece", mEndPieceDemoParam,
                                                     false);
            al::initNerveState(this, mEndPieceDemo, &NrvWarpCubeEndPieceDemo,
                               "かけら集め終了デモ");
        }

        al::setNerve(this, &NrvWarpCubeWaitPiece);
    }

    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    bool isUseObjectCamera = false;
    al::tryGetArg(&isUseObjectCamera, rInfo, "IsUseObjectCamera");

    if (isUseObjectCamera) {
        if (mIsSingleMode) {
            mObjectCameraTicket = al::initObjectCamera_RS(this, rInfo, nullptr);
        } else {
            mObjectCamera = al::initObjectCamera(this, rInfo, nullptr);
            al::tryChangeSingleCameraMode(mObjectCamera);
        }

        al::tryGetArg(&mOutCameraPlayStep, rInfo, "OutCameraPlayStep");
    }

    mMtxConnector = al::tryCreateMtxConnector(this, rInfo);

    if (mType == Type_OutOnly) {
        makeActorDead();
    } else if (al::listenStageSwitchOnAppear(this, al::Functor(this, &WarpCube::appearBySwitch))) {
        bool isUseSwitchAppearCamera = false;

        if (al::tryGetArg(&isUseSwitchAppearCamera, rInfo, "IsUseSwitchAppearCamera") &&
            isUseSwitchAppearCamera) {
            mSwitchAppearDemoParam = new ActorStateDemoCameraParam(
                120, 0, 60, al::getTransPtr(this), al::getTransPtr(this));
            mSwitchAppearDemo = new ActorStateDemoCamera(this, rInfo, "SwitchAppear",
                                                         mSwitchAppearDemoParam, false);
            al::initNerveState(this, mSwitchAppearDemo, &NrvWarpCubeAppearWithCamera,
                               "スイッチ出現デモ");
        }

        makeActorDead();
    } else {
        makeActorAppeared();
    }

    al::calcShadowMaskSize(&mShadowMaskSize, this, "シャドウマスク");
}

/**
 * @brief Make the box appear when its appear switch turns on.
 */
void WarpCube::appearBySwitch() {
    appear();
    al::hideModelIfShow(this);

    if (mSwitchAppearDemo != nullptr) {
        al::setNerve(this, &NrvWarpCubeAppearWithCamera);
    }
}

/**
 * @brief Attach the box to the collision below it, if it is connected to one.
 */
void WarpCube::initAfterPlacement() {
    if (mMtxConnector != nullptr) {
        al::attachMtxConnectorToCollision(mMtxConnector, this, false);
    }
}

/**
 * @brief Appear and play the appear animation.
 */
void WarpCube::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvWarpCubeAppear);
}

/**
 * @brief Appear during Bowser's chase.
 */
void WarpCube::appearByKoopaChase() {
    appear();
    al::setNerve(this, &NrvWarpCubeKoopaChaseAppear);
}

/**
 * @brief Follow the collision the box is connected to.
 */
void WarpCube::control() {
    if (mMtxConnector != nullptr) {
        al::connectPoseQT(this, mMtxConnector);
    }
}

/**
 * @brief Handle the binds of the players entering the box.
 * @param pMsg The received message.
 * @param pOther The sensor of the sender.
 * @param pSelf The sensor of the box.
 * @return Whether the message was handled.
 */
bool WarpCube::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvWarpCubeAppear) || al::isNerve(this, &NrvWarpCubeAppearWithCamera) ||
        al::isNerve(this, &NrvWarpCubeUnlockAppear) ||
        al::isNerve(this, &NrvWarpCubeKoopaChaseAppear) ||
        al::isNerve(this, &NrvWarpCubeWaitPiece) || al::isNerve(this, &NrvWarpCubeEndPieceDemo)) {
        return false;
    }

    if (al::isMsgBindGiant(pMsg)) {
        return true;
    }

    if (al::isMsgBindStart(pMsg)) {
        if (al::isNerve(this, &NrvWarpCubeWait)) {
            WarpObjUtil::stopStageTimer(this);
        }

        if (mWarpStep <= 60 && !al::isNerve(this, &NrvWarpCubePlayerIn)) {
            al::setNerve(this, &NrvWarpCubePlayerIn);
        }

        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        auto* puppeteer =
            mPuppeteerGroup->getPuppeteerByPlayerIndex<WarpCubeBindPuppeteer>(pOther);

        if (al::isNerve(this, &NrvWarpCubeAllBindForceInit)) {
            puppeteer->startBindForce(pOther, pSelf, al::getTrans(this));
            mBindForceNum++;
        } else {
            puppeteer->startBindInStart(pOther, pSelf);
        }

        al::sendMsgWarpStart(pOther, pSelf);
        mBindPuppeteers.pushBack(puppeteer);
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
            if (mBindPuppeteers[i]->tryCancelBind(pMsg, pOther)) {
                mBindPuppeteers.erase(i);

                if (mBindPuppeteers.isEmpty()) {
                    al::setNerve(this, &NrvWarpCubeWait);
                    WarpObjUtil::restartStageTimer(this);
                    rc::cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(this, pSelf);
                    mWarpStep = 0;
                    mDestCube->startOutEnd(true);
                }

                return true;
            }
        }

        return false;
    }

    return false;
}

/**
 * @brief Finish the exit of the players from this (destination) box.
 * @param isKill Whether an exit-only box is killed at once instead of playing its disappearance.
 */
void WarpCube::startOutEnd(bool isKill) {
    al::validateClipping(this);

    if (mType == Type_OutOnly) {
        if (isKill) {
            kill();
        } else {
            al::setNerve(this, &NrvWarpCubeDisappear);
        }
    } else {
        al::validateHitSensors(this);
        al::setNerve(this, &NrvWarpCubeWait);
    }
}

/**
 * @brief Notify that the last locked piece was collected.
 * @param pSensor The sensor of the player that collected it.
 */
void WarpCube::endGetPiece(al::HitSensor* pSensor) {
    mPieceSensor = pSensor;

    if (!al::isNerve(this, &NrvWarpCubeWaitPiece)) {
        return;
    }

    al::tryOnStageSwitch(this, "SwitchCollectPieceEndOn");

    if (mEndPieceDemo != nullptr) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvWarpCubeEndPieceDemo);
        return;
    }

    mLockedModel->kill();
    al::showModel(this);
    rc::addScore(mLockedModel, mPieceSensor, 100.0f, 0);
    al::setNerve(this, &NrvWarpCubeUnlockAppear);
}

/**
 * @brief Check whether the box can only be exited.
 * @return Whether the box is exit only.
 */
bool WarpCube::isTypeOutOnly() const {
    return mType == Type_OutOnly;
}

/**
 * @brief Let the entrance box drive this (destination) box with an action.
 * @param pActionName The action to play.
 */
void WarpCube::startControl(const char* pActionName) {
    if (!al::isAlive(this)) {
        al::LiveActor::appear();
    }

    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::startAction(this, pActionName);
    al::setNerve(this, &NrvWarpCubeControlled);
}

/**
 * @brief Check whether the box is driven by its entrance box.
 * @return Whether the box is controlled.
 */
bool WarpCube::isControlled() const {
    return al::isAlive(this) && al::isNerve(this, &NrvWarpCubeControlled);
}

/**
 * @brief Start the exit of the players from the destination box.
 */
void WarpCube::startOutSign() {
    rc::releaseAllTouchPointerHoldItem(this);

    if (rc::isExistGhostPlayerRecorder(this)) {
        rc::setPlacementIdObjGhostPlayerRecorder(this, mPlacementId, false);
        rc::tryStartFromObjGhostPlayerRecorder(this, mPlacementId);
    }

    if (isUseCameraTicket() || mDestCube->mObjectCamera != nullptr) {
        setPlayerTransToDest();
        al::requestResetUserCameraControl(this);

        if (isUseCameraTicket()) {
            if (!rc::requestStartDemoPlayerCutscene(this)) {
                return;
            }

            al::startCamera_RS(this, mDestCube->mObjectCameraTicket, -1);
            rc::addDemoActor(this);
            rc::addDemoActor(mDestCube);
        } else {
            al::startCamera(this, mDestCube->mObjectCamera, -1);
            al::tryOnStageSwitch(mDestCube, "SwitchObjectCameraKeepOn");
        }

        al::setNerve(this, &NrvWarpCubeOutSignWithCamera);
        return;
    }

    al::requestCaptureScreenCover(this, 4);
    al::setNerve(this, &NrvWarpCubeOutSign);
}

/**
 * @brief Move every bound player into the destination box.
 */
void WarpCube::setPlayerTransToDest() {
    sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
    al::calcTransLocalOffset(&trans, mDestCube, cDestOffset);

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (mPuppeteerGroup->getPuppeteer(i)->isBind()) {
            rc::setPuppetTrans(mPuppeteerGroup->getPuppeteer(i)->getPlayerPuppet(), trans);
        }
    }
}

/**
 * @brief Make the warped players jump out of the destination box.
 */
void WarpCube::endWarp() {
    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, mDestCube);
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    al::makeMtxFrontUp(&mtx, front, sead::Vector3f::ey);

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        IUsePlayerPuppet* puppet = mBindPuppeteers[i]->getPlayerPuppet();
        sead::Vector3f velocity = {0.0f, 0.0f, 0.0f};
        sead::Vector3f localVelocity = {0.0f, 0.0f, 0.0f};

        if (mDestCube->mIsOutSpeedZero) {
            sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
            WarpObjUtil::getJumpOutLocalTrans(&trans, i, mBindPuppeteers.size());
            al::calcTransLocalOffset(&trans, mDestCube, trans);
            rc::setPuppetTrans(puppet, trans);
        } else {
            WarpObjUtil::getJumpOutLocalVelocity(&localVelocity, i, mBindPuppeteers.size());
            velocity.setMul(mtx, localVelocity);
        }

        rc::startPuppetAction(puppet, "DokanJump");

        if (rc::isPuppetHidden(puppet)) {
            rc::showPuppet(puppet);
        }

        rc::showPuppetSilhouette(puppet);
        rc::setPuppetUpVec(puppet, sead::Vector3f::ey);
        rc::setPuppetFrontVec(puppet, front);
        rc::setPuppetVelocity(puppet, velocity);
    }

    endBindAllPuppet();
}

/**
 * @brief Release every bound player.
 */
void WarpCube::endBindAllPuppet() {
    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        WarpCubeBindPuppeteer* puppeteer = mBindPuppeteers[i];
        IUsePlayerPuppet* puppet = puppeteer->getPlayerPuppet();
        al::sendMsgWarpEnd(rc::getPuppetSensor(puppet), al::getHitSensor(this, nullptr));

        if (rc::isPuppetHidden(puppet)) {
            rc::showPuppet(puppet);
        }

        rc::resetPuppetDynamics(puppet);
        puppeteer->endBind(&sWarpEndParam);
    }

    mBindPuppeteers.clear();
    mWarpStep = 0;
}

/**
 * @brief Grow the shadow while the players go in.
 * @param isDest Whether the shadow of the destination box is updated instead of this one.
 */
void WarpCube::updateShadowScalePlayerIn(bool isDest) {
    al::LiveActor* actor = isDest ? mDestCube : this;
    lerpShadowMaskSize(actor, mShadowMaskSize, cShadowMaskSizeWarp, al::getActionFrame(actor),
                       10, 25);
}

/**
 * @brief Play the appear animation.
 */
void WarpCube::exeAppear() {
    if (al::isFirstStep(this)) {
        const char* actionName;

        if (al::isNerve(this, &NrvWarpCubeUnlockAppear)) {
            actionName = "UnlockAppear";
        } else {
            actionName =
                al::isNerve(this, &NrvWarpCubeKoopaChaseAppear) ? "KoopaChaseAppear" : "Appear";
        }

        al::startAction(this, actionName);
    }

    if (al::isStep(this, 1)) {
        al::showModelIfHide(this);
    }

    if (!al::isNerve(this, &NrvWarpCubeKoopaChaseAppear)) {
        // The result is unused in the game as well.
        al::isNerve(this, &NrvWarpCubeAppear);
        sead::Vector3f startSize(1.0f, mShadowMaskSize.y, 1.0f);
        lerpShadowMaskSize(this, startSize, mShadowMaskSize, al::getActionFrame(this), 0, 15);
    }

    al::setNerveAtActionEnd(this, &NrvWarpCubeWait);
}

/**
 * @brief Appear with the camera demo of the appear switch.
 */
void WarpCube::exeAppearWithCamera() {
    sead::Vector3f startSize(1.0f, mShadowMaskSize.y, 1.0f);

    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::setShadowMaskSize(this, "シャドウマスク", startSize);
    }

    if (mSwitchAppearDemo->getPlayStep() == 60) {
        al::startAction(this, "Appear");
    }

    if (mSwitchAppearDemo->getPlayStep() == 61) {
        al::showModelIfHide(this);
    }

    if (mSwitchAppearDemo->isGreaterEqualPlayStep(61)) {
        lerpShadowMaskSize(this, startSize, mShadowMaskSize, al::getActionFrame(this), 0, 15);
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvWarpCubeWait);
        al::validateClipping(this);
    }
}

/**
 * @brief Wait for players.
 */
void WarpCube::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OpenWait");
        al::validateClipping(this);
        al::setShadowMaskSize(this, "シャドウマスク", mShadowMaskSize);
    }
}

/**
 * @brief Swallow the players and prepare the destination box.
 */
void WarpCube::exePlayerIn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "In");
        al::invalidateClipping(this);

        if (!mDestCube->isControlled()) {
            if (mDestCube->isTypeOutOnly()) {
                mDestCube->startControl("ClosePreOut");
            } else {
                mDestCube->startControl("OpenPreOut");
            }
        }

        al::tryPauseBgmIfDifferBgmArea(this, al::getTrans(mDestCube), 30);
    }

    updateShadowScalePlayerIn(false);
    updateShadowScalePlayerIn(true);
    mWarpStep++;
    al::setNerveAtActionEnd(this, &NrvWarpCubeWaitStartWarp);
}

/**
 * @brief Wait a moment, then warp if every player is bound or bind the missing ones by force.
 */
void WarpCube::exeWaitStartWarp() {
    if (mWarpStep++ >= 60) {
        if (rc::checkAllPlayerBindedAndDisableReviveBubble(this, al::getHitSensor(this, nullptr))) {
            startOutSign();
        } else {
            al::setNerve(this, &NrvWarpCubeAllBindForceInit);
        }
    }
}

/**
 * @brief Request every player to be bound by the box.
 */
void WarpCube::exeAllBindForceInit() {
    if (al::isFirstStep(this)) {
        mBindForceNum = 0;
    }

    rc::requestBindAllPlayer(this, al::getHitSensor(this, nullptr));

    if (rc::isAllPlayerBinded(this, al::getHitSensor(this, nullptr))) {
        al::setNerve(this, &NrvWarpCubeAllBindForce);
    }
}

/**
 * @brief Wait for the forced binds to be ready.
 */
void WarpCube::exeAllBindForce() {
    mPuppeteerGroup->update();

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        WarpCubeBindPuppeteer* puppeteer = mBindPuppeteers.unsafeAt(i);

        if (puppeteer->isBind() && !puppeteer->isEnableStart()) {
            return;
        }
    }

    if (mBindForceNum > 0) {
        al::setNerve(this, &NrvWarpCubeAllBindForceIn);
    } else {
        startOutSign();
    }
}

/**
 * @brief Swallow the players that were bound by force.
 */
void WarpCube::exeAllBindForceIn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "In");
    }

    updateShadowScalePlayerIn(false);

    if (al::isActionEnd(this) && al::isGreaterEqualStep(this, 60)) {
        startOutSign();
    }
}

/**
 * @brief Open the destination box without a camera.
 */
void WarpCube::exeOutSign() {
    if (al::isFirstStep(this)) {
        al::startAction(mDestCube, "OutSign");
        setPlayerTransToDest();
        al::requestCancelInterpole(this);
    }

    if (al::isActionEnd(mDestCube)) {
        al::setNerve(this, &NrvWarpCubeOut);
    }
}

/**
 * @brief Stop the object camera of the destination started by the exit.
 */
inline void WarpCube::endOutCamera() {
    if (mIsSingleMode) {
        al::endCamera_RS(this, mDestCube->mObjectCameraTicket, -1, false);
        rc::requestEndDemoPlayerCutscene(this);
    } else {
        al::endCamera(this, mDestCube->mObjectCamera, -1);
    }

    al::tryOffStageSwitch(mDestCube, "SwitchObjectCameraKeepOn");
    al::setNerve(this, &NrvWarpCubeOut);
}

/**
 * @brief Open the destination box while the object camera moves to it.
 */
void WarpCube::exeOutSignWithCamera() {
    if (al::isFirstStep(this) && mIsBgmChangeWhenPieceComplete) {
        al::stopBgm(this, "Zigzag", 90, -1);
        al::startBgm(this, "Stage", 45, 0, -1, -1);
    }

    s32 interpoleStep = mIsSingleMode ?
                            al::getCameraInterpoleStep(mDestCube->mObjectCameraTicket) :
                            al::getCameraInterpoleFrame(mDestCube->mObjectCamera);
    s32 frameMax = static_cast<s32>(al::getActionFrameMax(mDestCube, "OutSign"));
    s32 outSignStep;

    if (mDestCube->mOutCameraPlayStep > 0) {
        s32 restStep = mDestCube->mOutCameraPlayStep - frameMax;
        outSignStep = restStep < 0 ? restStep + interpoleStep : -1;
    } else {
        outSignStep = sead::Mathi::max(interpoleStep - frameMax, 0);
    }

    if (outSignStep >= 0 && al::isStep(this, outSignStep)) {
        if (!al::isAlive(mDestCube)) {
            mDestCube->appear();
        }

        al::startAction(mDestCube, "OutSign");
    }

    if (al::isGreaterStep(this, outSignStep) && al::isGreaterStep(this, interpoleStep) &&
        al::isActionEnd(mDestCube)) {
        if (mDestCube->mOutCameraPlayStep > 0) {
            al::setNerve(this, &NrvWarpCubeOutSignWithCameraWait);
            return;
        }

        endOutCamera();
    }
}

/**
 * @brief Keep the object camera on the destination box for the placed play step.
 */
void WarpCube::exeOutSignWithCameraWait() {
    s32 frameMax = static_cast<s32>(al::getActionFrameMax(mDestCube, "OutSign"));
    s32 restStep = mDestCube->mOutCameraPlayStep - frameMax;
    s32 outSignStep = sead::Mathi::max(restStep, -1);

    if (outSignStep >= 0 && al::isStep(this, outSignStep)) {
        if (al::isDead(mDestCube)) {
            mDestCube->appear();
        }

        al::startAction(mDestCube, "OutSign");
    }

    if (al::isGreaterStep(this, mDestCube->mOutCameraPlayStep)) {
        endOutCamera();
    }
}

/**
 * @brief Make the players jump out of the destination box.
 */
void WarpCube::exeOut() {
    if (al::isFirstStep(this)) {
        rc::resetDisableReviveBubbleForAllPlayer(this);
        al::startAction(mDestCube, "Out");
        WarpObjUtil::restartStageTimer(this);
        endWarp();
    }

    WarpCube* destCube = mDestCube;
    lerpShadowMaskSize(destCube, cShadowMaskSizeWarp, mShadowMaskSize,
                       al::getActionFrame(destCube), 0, 10);

    if (al::isActionEnd(mDestCube)) {
        mDestCube->startOutEnd(false);
        mWarpStep = 0;

        if (mType == Type_OneWay) {
            kill();
        } else {
            al::setNerve(this, &NrvWarpCubeWait);
        }
    }
}

/**
 * @brief Stay hidden until every locked piece is collected.
 */
void WarpCube::exeWaitPiece() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
    }
}

/**
 * @brief Play the camera demo of the unlock after the last piece was collected.
 */
void WarpCube::exeEndPieceDemo() {
    sead::Vector3f startSize(1.0f, mShadowMaskSize.y, 1.0f);

    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::tryOnStageSwitch(this, "SwitchCollectPieceEndDemoKeepOn");
    }

    if (mEndPieceDemo->getPlayStep() == 59) {
        mLockedModel->kill();
    }

    if (mEndPieceDemo->getPlayStep() == 60) {
        al::showModel(this);
        al::startAction(this, "UnlockAppear");
        rc::addScore(mLockedModel, mPieceSensor, 100.0f, 0);
        al::startSe(this, "Unlock");
    }

    if (mEndPieceDemo->isGreaterEqualPlayStep(60)) {
        lerpShadowMaskSize(this, startSize, mShadowMaskSize, al::getActionFrame(this), 0, 15);
    }

    if (al::updateNerveState(this)) {
        al::validateClipping(this);
        al::tryOffStageSwitch(this, "SwitchCollectPieceEndDemoKeepOn");
        al::setNerve(this, &NrvWarpCubeWait);
    }
}

/**
 * @brief Being driven by the entrance box.
 */
void WarpCube::exeControlled() {}

/**
 * @brief Play the disappearance of an exit-only box.
 */
void WarpCube::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
    }

    if (al::isActionEnd(this)) {
        al::validateHitSensors(this);
        al::validateClipping(this);
        kill();
    }
}

/**
 * @brief Destroy the box.
 */
WarpCube::~WarpCube() = default;

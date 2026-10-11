#include "NPC/GhostPlayer.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Layout/GhostMiiNameplate.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/PoseHistoryPath.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "NPC/GhostPlayerLoaderBase.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "NPC/GhostPresentBox.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(GhostPlayer, Wait)
NERVE_DECL(GhostPlayer, SupportFreeze)
NERVE_DECL(GhostPlayer, Play)
NERVE_DECL(GhostPlayer, WaitStart)
NERVE_DECL(GhostPlayer, WaitRestartPlay)
NERVE_DECL(GhostPlayer, Stop)

/** @brief Stopped while still waiting to start; resumes into Wait on restart. */
class GhostPlayerNrvStopWait : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GhostPlayer>()->exeStop();
    }
};

const struct {
    GhostPlayerNrvWait Wait;
    GhostPlayerNrvSupportFreeze SupportFreeze;
} NrvGhostPlayer;
NERVES_MAKE_NOSTRUCT(GhostPlayer, Play, WaitStart, WaitRestartPlay, Stop, StopWait)

const char* const cArchiveName = "GhostPlayer1";
const char* const cTimeAttackEffectName = "GhostPlayerTimeAttack";
const char* const cTraceEffectName = "Trace";
const sead::Vector3f cNameplateOffset(0.0f, 120.0f, 0.0f);
}  // namespace

/**
 * @brief Constructs a ghost player.
 * @param pName Actor name.
 * @param index Ghost slot index (staggers the start timing).
 * @param isTimeAttack Whether this is a time attack rival ghost.
 * @param isEnablePresent Whether this ghost may carry a present box.
 */
GhostPlayer::GhostPlayer(const char* pName, s32 index, bool isTimeAttack, bool isEnablePresent)
    : al::LiveActor(pName), mIndex(index), mIsTimeAttack(isTimeAttack),
      mIsEnablePresent(isEnablePresent), mActionName("") {}

/**
 * @brief Initializes the model, name plate, pose history, optional present box and nerves.
 * @param rInfo Actor initialization info.
 */
void GhostPlayer::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, cArchiveName, nullptr);
    al::initActorEffectKeeper(this, rInfo,
                              mIsTimeAttack ? cTimeAttackEffectName : cArchiveName, false);
    al::setSeSeqLocalVariableDefault(this, 0, 7);
    mNameplate = new GhostMiiNameplate(this, al::getLayoutInitInfo(rInfo));
    mPoseHistoryPath = new al::PoseHistoryPath(64);

    s32 presentCounter = GameDataFunction::getGhostPresentCounter(this);
    if (mIsEnablePresent && presentCounter % 3 == 0) {
        mPresentBox = new GhostPresentBox("ゴーストプレゼントボックス", presentCounter / 3);
        mPresentBox->setPoseHistoryPath(mPoseHistoryPath);
        al::initCreateActorNoPlacementInfo(mPresentBox, rInfo);
    }

    al::initNerve(this, &NrvGhostPlayer.Wait, 1);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, nullptr);
    al::initNerveState(this, mStateSupportFreeze, &NrvGhostPlayer.SupportFreeze, "DRC拘束");
    makeActorDead();
}

/** @brief Places the ghost at the first recorded pose and starts playing or waiting. */
void GhostPlayer::appear() {
    al::LiveActor::appear();
    if (mPlayData != nullptr && mFrame == 0 &&
        GhostPlayerFunction::getGhostPlayDataNum(mPlayData) >= 1) {
        GhostPlayerFunction::calcGhostPlayDataTrans(al::getTransPtr(this), mPlayData, 0, 1000.0f);
        GhostPlayerFunction::calcGhostPlayDataRotate(al::getRotatePtr(this), mPlayData, 0);
    }

    if (mIsTimeAttack) {
        al::setNerve(this, &NrvGhostPlayerPlay);
    } else {
        al::setNerve(this, &NrvGhostPlayerWaitStart);
    }
}

/** @brief Makes the actor appear along with its name plate. */
void GhostPlayer::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    mNameplate->appear();
}

/** @brief Kills the name plate and present box, then the actor. */
void GhostPlayer::makeActorDead() {
    mNameplate->kill();
    if (mPresentBox != nullptr && al::isAlive(mPresentBox)) {
        mPresentBox->stop(false);
    }

    al::LiveActor::makeActorDead();
}

/** @brief Kills the ghost, playing the death reaction unless it vanished into a pipe. */
void GhostPlayer::kill() {
    if (al::isDead(this)) {
        return;
    }

    bool isInDokan = al::isNerve(this, &NrvGhostPlayer.Wait) &&
                     al::isEqualString("DokanIn", mActionName.cstr());
    if (!isInDokan && !al::isNerve(this, &NrvGhostPlayerWaitStart) &&
        !al::isNerve(this, &NrvGhostPlayerWaitRestartPlay)) {
        al::startHitReactionDeath(this);
    }

    al::LiveActor::kill();
}

/** @brief Hides the name plate while clipped. */
void GhostPlayer::startClipped() {
    al::LiveActor::startClipped();
    mNameplate->kill();
}

/**
 * @brief Loads the recorded play data for this ghost.
 * @param pLoader Loader providing the play data.
 * @param dataIndex Data index to load, or -1 to use the ghost index.
 * @return Whether valid play data was loaded.
 */
bool GhostPlayer::initGhostPlayData(GhostPlayerLoaderBase* pLoader, s32 dataIndex) {
    if (dataIndex == -1) {
        dataIndex = mIndex;
    }

    u32 size = GhostPlayerFunction::getGhostPlayerRecorderBufferSize();
    mPlayData = new (0x40) u8[size];
    if (!pLoader->load(&mPlayData, size, mIndex, dataIndex)) {
        return false;
    }

    if (!GhostPlayerFunction::isValidGhostPlayerDataVersion(mPlayData)) {
        return false;
    }

    mDisplayInfo = pLoader->getDisplayInfo(mIndex);
    if (!mDisplayInfo->mIsValid) {
        return false;
    }

    mNameplate->init(mDisplayInfo->mUserName.cstr(), mDisplayInfo->mIsMii, mIsTimeAttack);
    return true;
}

/** @brief Updates animations. */
void GhostPlayer::calcAnim() {
    al::LiveActor::calcAnim();
}

/** @brief Updates the name plate visibility and records the pose history for followers. */
void GhostPlayer::control() {
    if (mState == State::EnterDoor) {
        return;
    }

    GhostMiiNameplateFunction::updateShowHideFromCameraDistance(
        mNameplate,
        al::isJudgedToClipFrustum(this, al::getTrans(this) + cNameplateOffset, 160.0f, 300.0f));
    sead::Quatf quat;
    al::calcQuat(&quat, this);
    if (al::isNerve(this, &NrvGhostPlayerPlay) &&
        !al::isEqualString(sead::SafeString(""), mActionName)) {
        mPoseHistoryPath->addHistory(quat, al::getTrans(this), al::getActionName(this), 30.0f);
    } else {
        mPoseHistoryPath->addHistory(quat, al::getTrans(this), 30.0f);
    }
}

/** @brief Waits hidden for a staggered delay, then starts playing. */
void GhostPlayer::exeWaitStart() {
    if (al::isFirstStep(this)) {
        hideGhost(true);
    }

    if (al::isGreaterEqualStep(this, getStartWaitStep())) {
        if (rc::isFlagShakeAfterGhostPlayerRecorder(this)) {
            tryStartFromCheckpointFlag(rc::tryGetPlacementIdFlagGhostPlayerRecorder(this));
        }

        al::setNerve(this, &NrvGhostPlayerPlay);
    }
}

/**
 * @brief Hides the ghost model, name plate and effects.
 * @param isForce Hide immediately without the disappear reaction and stop all effects.
 */
void GhostPlayer::hideGhost(bool isForce) {
    mIsHidden = true;
    sead::Vector3f nameplatePos = al::getTrans(this) + cNameplateOffset;
    bool isSilent = al::isJudgedToClipFrustum(this, nameplatePos, 160.0f, 300.0f) || isForce;
    if (!isSilent) {
        al::startHitReactionDisappear(this);
    }

    al::hideModelIfShow(this);
    if (mNameplate->isAlive()) {
        mNameplate->kill();
    }

    if (mPresentBox != nullptr) {
        mPresentBox->stop(!isSilent);
    }

    if (mIsTimeAttack) {
        al::tryDeleteEffect(this, cTraceEffectName);
    }

    if (isForce) {
        al::tryDeleteEmitterAndParticleAll(this);
        al::offCalcAndDrawEffect(this);
    }
}

/**
 * @brief Skips ahead to the recorded frame of the checkpoint flag the player touched.
 * @param pPlacementId Placement ID of the checkpoint flag.
 */
void GhostPlayer::tryStartFromCheckpointFlag(const al::PlacementId* pPlacementId) {
    if (al::isDead(this) || pPlacementId->mPlacementID == nullptr || mIsTimeAttack) {
        return;
    }

    for (s32 i = 0; i < GhostPlayerFunction::getWarpObjDataNum(mPlayData); i++) {
        al::StringTmp<64> objName(
            "%s%s", pPlacementId->mPlacementID,
            pPlacementId->mZoneID != nullptr ? pPlacementId->mZoneID : "");
        if (al::isEqualString(objName.cstr(),
                              GhostPlayerFunction::getWarpObjData(mPlayData, i)->mName)) {
            const GhostWarpObjData* pData = GhostPlayerFunction::getWarpObjData(mPlayData, i);
            if (pData == nullptr) {
                break;
            }

            mFrame = pData->mFrame;
            al::setNerve(this, &NrvGhostPlayerWaitStart);
            return;
        }
    }

    kill();
}

/** @brief Does nothing while waiting. */
void GhostPlayer::exeWait() {}

/** @brief Waits for a staggered delay, then resumes playing. */
void GhostPlayer::exeWaitRestartPlay() {
    if (al::isGreaterEqualStep(this, getStartWaitStep())) {
        al::setNerve(this, &NrvGhostPlayerPlay);
    }
}

/** @brief Replays one frame of the recorded ghost data. */
void GhostPlayer::exePlay() {
    if (al::isFirstStep(this)) {
        if (al::isEqualString(sead::SafeString(""), mActionName)) {
            al::startAction(this, "Wait");
        }

        mPoseHistoryPath->clear();
    }

    if (al::isStep(this, 240) && mPresentBox != nullptr) {
        mPresentBox->start();
    }

    if (mPresentStartDelay > 0) {
        mPresentStartDelay--;
        if (mPresentStartDelay == 0 && mPresentBox != nullptr) {
            mPresentBox->start();
        }
    }

    if (GhostPlayerFunction::getGhostPlayDataNum(mPlayData) * 2 <= mFrame) {
        al::validateClipping(this);
        if (mPresentBox != nullptr) {
            mPresentBox->stop(true);
        }

        al::setNerve(this, &NrvGhostPlayer.Wait);
        return;
    }

    if (mShowDelay > 0) {
        mShowDelay--;
    }

    for (s32 i = 0; i < GhostPlayerFunction::getWarpObjDataNum(mPlayData); i++) {
        const GhostWarpObjData* pData = GhostPlayerFunction::getWarpObjData(mPlayData, i);
        if (pData->mFrame != mFrame) {
            continue;
        }

        if (!mIsTimeAttack) {
            if (pData->mIsRestartPoint) {
                stopAndHide(false);
                return;
            }

            continue;
        }

        if (!pData->mIsRestartPoint) {
            continue;
        }

        for (s32 frame = pData->mFrame + 1;
             frame < GhostPlayerFunction::getGhostPlayDataNum(mPlayData) * 2; frame++) {
            for (s32 j = 0; j < GhostPlayerFunction::getWarpObjDataNum(mPlayData); j++) {
                if (frame == GhostPlayerFunction::getWarpObjData(mPlayData, j)->mFrame) {
                    mFrame = frame;
                    hideGhost(false);
                    mShowDelay = 5;
                    return;
                }
            }
        }
    }

    sead::Vector3f prevTrans = al::getTrans(this);
    sead::Vector3f prevRotate = al::getRotate(this);
    GhostPlayerFunction::calcGhostPlayDataTrans(al::getTransPtr(this), mPlayData, mFrame, 1000.0f);
    GhostPlayerFunction::calcGhostPlayDataRotate(al::getRotatePtr(this), mPlayData, mFrame);
    if ((al::getTrans(this) - prevTrans).length() > 1000.0f) {
        mPoseHistoryPath->clear();
    }

    const char* pActionName = nullptr;
    GhostPlayerFunction::calcGhostPlayDataActionName(&pActionName, mPlayData, mFrame);
    al::StringTmp<64> ghostAction(al::getActionName(this));
    if (!al::isEqualString(pActionName, mActionName.cstr())) {
        mActionName = pActionName;
        GhostPlayerFunction::makeGhostActionName(&ghostAction, pActionName, this);
        if (al::isEqualString(ghostAction.cstr(), "Jump") ||
            al::isEqualString(ghostAction.cstr(), "Jump2") ||
            al::isEqualString(ghostAction.cstr(), "Jump3") ||
            al::isEqualString(ghostAction.cstr(), "JumpReverse") ||
            al::isEqualString(ghostAction.cstr(), "ClimbJump") ||
            al::isEqualString(ghostAction.cstr(), "ClimbJump2") ||
            al::isEqualString(ghostAction.cstr(), "ClimbJump3") ||
            al::isEqualString(ghostAction.cstr(), "ClimbJumpReverse")) {
            mState = State::Jump;
        } else if (al::isEqualString(ghostAction.cstr(), "DoorIn") ||
                   al::isEqualString(ghostAction.cstr(), "DokanIn") ||
                   al::isEqualString(ghostAction.cstr(), "DokanSideIn")) {
            mState = State::EnterDoor;
            if (mNameplate->isAlive()) {
                mNameplate->end();
            }

            if (mPresentBox != nullptr) {
                mPresentBox->stop(true);
            }
        } else if (al::isEqualString(ghostAction.cstr(), "DoorOut") ||
                   al::isEqualSubString(ghostAction.cstr(), "DokanOut") ||
                   al::isEqualString(ghostAction.cstr(), "DokanSideOut")) {
            mState = State::ExitDoor;
            mPoseHistoryPath->clear();
        } else {
            mState = State::Normal;
        }

        if (mState != State::EnterDoor && mIsActionEnd) {
            mShowDelay = 5;
            mIsActionEnd = false;
            mPresentStartDelay = 15;
        }
    }

    if (mIsHidden) {
        if (!mIsActionEnd && mShowDelay <= 0) {
            showGhost(true);
        }
    } else if (mIsActionEnd || mShowDelay > 0) {
        hideGhost(true);
    }

    if (mState != State::KouraRiding) {
        const char* pCurAction = al::getActionName(this);
        if (pCurAction != nullptr && al::isEqualString(pCurAction, ghostAction.cstr())) {
            f32 frame = 0.0f;
            GhostPlayerFunction::calcGhostPlayDataSklAnimFrame(&frame, mPlayData, mFrame,
                                                               al::isSklAnimOneTime(this, 0));
            if (al::isEqualString(ghostAction.cstr(), "KouraIn") &&
                al::isNearZero(frame - al::getSklAnimFrame(this, 0), 0.001f)) {
                mState = State::KouraRiding;
                al::startAction(this, "KouraRiding");
            } else {
                bool isMoving = mState == State::Jump || mState == State::EnterDoor ||
                                mState == State::ExitDoor;
                if (!isMoving || !(frame < al::getSklAnimFrame(this, 0))) {
                    al::setSklAnimFrame(this, frame, 0);
                }
            }

            if (mState == State::EnterDoor) {
                if (al::getSklAnimFrameMax(this, 0) + -2.0f < frame) {
                    mIsActionEnd = true;
                    if (!mIsHidden) {
                        hideGhost(true);
                    }
                }

                sead::Vector3f moved = al::getTrans(this) - prevTrans;
                if (!al::isNearZero(moved, 0.001f) && al::getSklAnimFrame(this, 0) >= 10.0f) {
                    al::setTrans(this, prevTrans);
                    al::setRotate(this, prevRotate);
                }
            }
        } else {
            al::startAction(this, ghostAction.cstr());
        }
    }

    mFrame++;
}

/**
 * @brief Hides the ghost and stops replaying.
 * @param isForce Hide immediately without the disappear reaction.
 */
void GhostPlayer::stopAndHide(bool isForce) {
    if (al::isNerve(this, &NrvGhostPlayerStop) || al::isNerve(this, &NrvGhostPlayerStopWait)) {
        return;
    }

    hideGhost(isForce);
    if (al::isNerve(this, &NrvGhostPlayer.Wait)) {
        al::setNerve(this, &NrvGhostPlayerStopWait);
    } else {
        al::setNerve(this, &NrvGhostPlayerStop);
    }
}

/**
 * @brief Shows the ghost model and name plate again.
 * @param isEnableEffect Turn effect drawing back on immediately; otherwise only delay the
 * present box restart.
 */
void GhostPlayer::showGhost(bool isEnableEffect) {
    mIsHidden = false;
    al::showModelIfHide(this);
    if (!mNameplate->isAlive()) {
        mNameplate->appear();
    }

    if (isEnableEffect) {
        al::onCalcAndDrawEffect(this);
    } else {
        mPresentStartDelay = 15;
    }

    al::startHitReactionAppear(this);
    if (mIsTimeAttack) {
        al::tryEmitEffect(this, cTraceEffectName, nullptr);
    }
}

/** @brief Runs the support-freeze state, then resumes playing. */
void GhostPlayer::exeSupportFreeze() {
    al::updateNerveStateAndNextNerve(this, &NrvGhostPlayerPlay);
}

/** @brief Does nothing while stopped. */
void GhostPlayer::exeStop() {}

/**
 * @brief Skips ahead to the recorded frame where the ghost used the given warp object.
 * @param pObjName Warp object name.
 */
void GhostPlayer::tryStartFromObj(const char* pObjName) {
    if (al::isDead(this) || mIsTimeAttack || !al::isNerve(this, &NrvGhostPlayerPlay)) {
        return;
    }

    for (s32 i = 0; i < GhostPlayerFunction::getWarpObjDataNum(mPlayData); i++) {
        if (al::isEqualString(pObjName,
                              GhostPlayerFunction::getWarpObjData(mPlayData, i)->mName)) {
            const GhostWarpObjData* pData = GhostPlayerFunction::getWarpObjData(mPlayData, i);
            if (pData == nullptr) {
                return;
            }

            mFrame = pData->mFrame;
            if (!mIsHidden) {
                mShowDelay = 5;
                hideGhost(false);
            }

            al::setNerve(this, &NrvGhostPlayerWaitRestartPlay);
            return;
        }
    }
}

/** @brief Resumes a stopped ghost. */
void GhostPlayer::restart() {
    if (al::isNerve(this, &NrvGhostPlayerStopWait)) {
        showGhost(true);
        al::setNerve(this, &NrvGhostPlayer.Wait);
        return;
    }

    al::setNerve(this, &NrvGhostPlayerPlay);
    mFrame++;
}

/**
 * @brief Lets the ghost push the player along route pipes it travels through.
 * @param pSelf This actor's sensor.
 * @param pOther The touched sensor.
 */
void GhostPlayer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isNerve(this, &NrvGhostPlayerPlay)) {
        return;
    }

    if (al::isNerve(this, &NrvGhostPlayerPlay) && al::isGreaterStep(this, 2)) {
        const char* pActionName = al::getActionName(this);
        if (pActionName != nullptr && al::isEqualSubString(pActionName, "RouteDokanMove")) {
            sead::Vector3f front;
            al::calcFrontDir(&front, this);
            rc::sendMsgRouteDokanPlayerTouch(pOther, pSelf, front);
        }
    }
}

/**
 * @brief Handles touch-screen pointing (support freeze) while playing.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Pointed target.
 * @return Whether the message was handled.
 */
bool GhostPlayer::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, &NrvGhostPlayerPlay) &&
        !al::isNerve(this, &NrvGhostPlayer.SupportFreeze)) {
        return false;
    }

    if (mIsTimeAttack) {
        return false;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (al::isNerve(this, &NrvGhostPlayerPlay)) {
            al::setNerve(this, &NrvGhostPlayer.SupportFreeze);
        }

        return true;
    }

    return false;
}

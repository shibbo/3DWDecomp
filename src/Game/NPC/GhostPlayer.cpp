#include "NPC/GhostPlayer.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "NPC/GhostPlayerLoaderBase.hpp"
#include "NPC/GhostPlayerDisplayInfo.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include <basis/seadNew.h>
#include "Library/Play/Placement/PlacementId.hpp"
#include "Layout/GhostMiiNameplate.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "NPC/GhostPresentBox.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Library/Movement/PoseHistoryPath.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(GhostPlayer, Wait)
NERVE_DECL(GhostPlayer, SupportFreeze)
NERVE_DECL(GhostPlayer, Play)
NERVE_DECL(GhostPlayer, WaitStart)
NERVE_DECL(GhostPlayer, WaitRestartPlay)
NERVE_DECL(GhostPlayer, Stop)
class GhostPlayerNrvStopWait : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GhostPlayer>()->exeStop();
    }
};
NERVES_MAKE_STRUCT(GhostPlayer, Wait, SupportFreeze, Play, WaitStart, WaitRestartPlay,
                    Stop, StopWait)
}

/** @brief Creates a replay actor with its player slot and playback options. */
GhostPlayer::GhostPlayer(const char* pName, int playerIndex, bool isShadow, bool hasPresent)
    : al::LiveActor(pName), mPlayerIndex(playerIndex), mIsShadow(isShadow),
      mHasPresent(hasPresent), mActionName("") {}

/** @brief Creates the model, nameplate, optional present, and freeze state. */
void GhostPlayer::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "GhostPlayer1", nullptr);
    al::initActorEffectKeeper(this, rInfo, mIsShadow ? "GhostPlayerTimeAttack" : "GhostPlayer1", false);
    al::setSeSeqLocalVariableDefault(this, 0, 7);
    mNameplate = new GhostMiiNameplate(this, al::getLayoutInitInfo(rInfo));
    mPoseHistory = new al::PoseHistoryPath(64);
    int presentCount = GameDataFunction::getGhostPresentCounter(GameDataHolderAccessor(this));
    if (mHasPresent && presentCount % 3 == 0) {
        mPresentBox = new GhostPresentBox("ゴーストプレゼントボックス", presentCount / 3);
        mPresentBox->setPoseHistoryPath(mPoseHistory);
        al::initCreateActorNoPlacementInfo(mPresentBox, rInfo);
    }
    al::initNerve(this, &NrvGhostPlayer.Wait, 1);
    mSupportFreeze = new ActorStateSupportFreeze(this, nullptr);
    al::initNerveState(this, mSupportFreeze, &NrvGhostPlayer.SupportFreeze, "DRC拘束");
    makeActorDead();
}

/** @brief Places a new replay at its first pose and selects its starting state. */
void GhostPlayer::appear() {
    al::LiveActor::appear();
    if (mPlayData && mFrame == 0 && GhostPlayerFunction::getGhostPlayDataNum(mPlayData) > 0) {
        GhostPlayerFunction::calcGhostPlayDataTrans(al::getTransPtr(this), mPlayData, 0, 1000.0f);
        GhostPlayerFunction::calcGhostPlayDataRotate(al::getRotatePtr(this), mPlayData, 0);
    }
    if (mIsShadow)
        al::setNerve(this, &NrvGhostPlayer.Play);
    else
        al::setNerve(this, &NrvGhostPlayer.WaitStart);
}

void GhostPlayer::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    mNameplate->appear();
}

void GhostPlayer::makeActorDead() {
    mNameplate->kill();
    if (mPresentBox && al::isAlive(mPresentBox))
        mPresentBox->stop(false);
    al::LiveActor::makeActorDead();
}

/** @brief Suppresses the death reaction while waiting or entering a pipe. */
void GhostPlayer::kill() {
    if (al::isDead(this))
        return;
    if (!(al::isNerve(this, &NrvGhostPlayer.Wait) &&
          al::isEqualString("DokanIn", mActionName.cstr())) &&
        !al::isNerve(this, &NrvGhostPlayer.WaitStart) &&
        !al::isNerve(this, &NrvGhostPlayer.WaitRestartPlay))
        al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

void GhostPlayer::startClipped() {
    al::LiveActor::startClipped();
    mNameplate->kill();
}

/** @brief Loads replay data and initializes its available display name. */
bool GhostPlayer::initGhostPlayData(GhostPlayerLoaderBase* pLoader, int index) {
    if (index == -1)
        index = mPlayerIndex;
    u32 size = GhostPlayerFunction::getGhostPlayerRecorderBufferSize();
    mPlayData = new (64) u8[size];
    if (!pLoader->load(&mPlayData, size, mPlayerIndex, index) ||
        !GhostPlayerFunction::isValidGhostPlayerDataVersion(mPlayData))
        return false;
    mDisplayInfo = pLoader->getDisplayInfo(mPlayerIndex);
    if (!mDisplayInfo->mIsValid)
        return false;
    mNameplate->init(mDisplayInfo->mUserName.cstr(), mDisplayInfo->mIsFriend, mIsShadow);
    return true;
}

void GhostPlayer::calcAnim() { al::LiveActor::calcAnim(); }

/** @brief Updates nameplate clipping and the present box's pose trail. */
void GhostPlayer::control() {
    if (mActionType != 3) {
        GhostMiiNameplateFunction::updateShowHideFromCameraDistance(
            mNameplate, al::isJudgedToClipFrustum(
                            this, al::getTrans(this) + sead::Vector3f(0.0f, 120.0f, 0.0f),
                            160.0f, 300.0f));
    }
    sead::Quatf quat;
    al::calcQuat(&quat, this);
    if (al::isNerve(this, &NrvGhostPlayer.Play) &&
        !al::isEqualString(sead::SafeString(""), mActionName))
        mPoseHistory->addHistory(quat, al::getTrans(this), al::getActionName(this), 30.0f);
    else
        mPoseHistory->addHistory(quat, al::getTrans(this), 30.0f);
}

void GhostPlayer::exeWaitStart() {
    if (al::isFirstStep(this))
        hideGhost(true);
    if (al::isGreaterEqualStep(this, 90 + 45 * (mPlayerIndex % 3))) {
        if (rc::isFlagShakeAfterGhostPlayerRecorder(this))
            tryStartFromCheckpointFlag(rc::tryGetPlacementIdFlagGhostPlayerRecorder(this));
        al::setNerve(this, &NrvGhostPlayer.Play);
    }
}

/** @brief Hides the model, label, present, and optionally all lingering effects. */
void GhostPlayer::hideGhost(bool immediate) {
    mIsHidden = true;
    bool isClipped = al::isJudgedToClipFrustum(
        this, al::getTrans(this) + sead::Vector3f(0.0f, 120.0f, 0.0f), 160.0f, 300.0f);
    bool skipReaction = isClipped | immediate;
    if (!skipReaction)
        al::startHitReactionDisappear(this);
    al::hideModelIfShow(this);
    if (mNameplate->isAlive())
        mNameplate->kill();
    if (mPresentBox)
        mPresentBox->stop(!skipReaction);
    if (mIsShadow)
        al::tryDeleteEffect(this, "Trace");
    if (immediate) {
        al::tryDeleteEmitterAndParticleAll(this);
        al::offCalcAndDrawEffect(this);
    }
}

/** @brief Seeks to the checkpoint marker, killing replays without that marker. */
void GhostPlayer::tryStartFromCheckpointFlag(const al::PlacementId* pId) {
    if (al::isDead(this) || !pId->mPlacementID || mIsShadow)
        return;
    for (int i = 0; i < GhostPlayerFunction::getWarpObjDataNum(mPlayData); ++i) {
        al::StringTmp<64> name("%s%s", pId->mPlacementID, pId->mZoneID ? pId->mZoneID : "");
        if (!al::isEqualString(name.cstr(), GhostPlayerFunction::getWarpObjData(mPlayData, i)->name))
            continue;
        const auto* pWarp = GhostPlayerFunction::getWarpObjData(mPlayData, i);
        if (pWarp) {
            mFrame = pWarp->frame;
            al::setNerve(this, &NrvGhostPlayer.WaitStart);
            return;
        }
        break;
    }
    kill();
}

void GhostPlayer::exeWait() {}

/** @brief Staggers replay restarts across three player slots. */
void GhostPlayer::exeWaitRestartPlay() {
    if (al::isGreaterEqualStep(this, 90 + 45 * (mPlayerIndex % 3)))
        al::setNerve(this, &NrvGhostPlayer.Play);
}

/** @brief Advances replay poses, warp markers, animation frames, and visibility. */
void GhostPlayer::exePlay() {
    if (al::isFirstStep(this)) {
        if (al::isEqualString(sead::SafeString(""), mActionName))
            al::startAction(this, "Wait");
        mPoseHistory->clear();
    }
    if (al::isStep(this, 240) && mPresentBox)
        mPresentBox->start();
    if (mAppearTimer > 0) {
        --mAppearTimer;
        if (mAppearTimer == 0 && mPresentBox)
            mPresentBox->start();
    }
    if (GhostPlayerFunction::getGhostPlayDataNum(mPlayData) * 2 <= mFrame) {
        al::validateClipping(this);
        if (mPresentBox)
            mPresentBox->stop(true);
        al::setNerve(this, &NrvGhostPlayer.Wait);
        return;
    }
    if (mHideTimer > 0)
        --mHideTimer;
    for (int i = 0; i < GhostPlayerFunction::getWarpObjDataNum(mPlayData); ++i) {
        const auto* pWarp = GhostPlayerFunction::getWarpObjData(mPlayData, i);
        if (pWarp->frame != mFrame)
            continue;
        if (mIsShadow) {
            if (pWarp->isStart) {
                for (int frame = pWarp->frame + 1;
                     frame < GhostPlayerFunction::getGhostPlayDataNum(mPlayData) * 2; ++frame) {
                    for (int j = 0; j < GhostPlayerFunction::getWarpObjDataNum(mPlayData); ++j) {
                        if (frame == GhostPlayerFunction::getWarpObjData(mPlayData, j)->frame) {
                            mFrame = frame;
                            hideGhost(false);
                            mHideTimer = 5;
                            return;
                        }
                    }
                }
            }
        } else if (pWarp->isStart) {
            stopAndHide(false);
            return;
        }
    }

    sead::Vector3f oldTrans = al::getTrans(this);
    sead::Vector3f oldRotate = al::getRotate(this);
    GhostPlayerFunction::calcGhostPlayDataTrans(al::getTransPtr(this), mPlayData, mFrame, 1000.0f);
    GhostPlayerFunction::calcGhostPlayDataRotate(al::getRotatePtr(this), mPlayData, mFrame);
    if ((al::getTrans(this) - oldTrans).length() > 1000.0f)
        mPoseHistory->clear();

    const char* pRecordedAction = nullptr;
    GhostPlayerFunction::calcGhostPlayDataActionName(&pRecordedAction, mPlayData, mFrame);
    al::StringTmp<64> action(al::getActionName(this));
    if (!al::isEqualString(pRecordedAction, mActionName.cstr())) {
        mActionName = pRecordedAction;
        GhostPlayerFunction::makeGhostActionName(&action, pRecordedAction, this);
        if (al::isEqualString(action.cstr(), "Jump") ||
            al::isEqualString(action.cstr(), "Jump2") ||
            al::isEqualString(action.cstr(), "Jump3") ||
            al::isEqualString(action.cstr(), "JumpReverse") ||
            al::isEqualString(action.cstr(), "ClimbJump") ||
            al::isEqualString(action.cstr(), "ClimbJump2") ||
            al::isEqualString(action.cstr(), "ClimbJump3") ||
            al::isEqualString(action.cstr(), "ClimbJumpReverse")) {
            mActionType = 2;
        } else if (al::isEqualString(action.cstr(), "DoorIn") ||
                   al::isEqualString(action.cstr(), "DokanIn") ||
                   al::isEqualString(action.cstr(), "DokanSideIn")) {
            mActionType = 3;
            if (mNameplate->isAlive())
                mNameplate->end();
            if (mPresentBox)
                mPresentBox->stop(true);
        } else if (al::isEqualString(action.cstr(), "DoorOut") ||
                   al::isEqualSubString(action.cstr(), "DokanOut") ||
                   al::isEqualString(action.cstr(), "DokanSideOut")) {
            mActionType = 4;
            mPoseHistory->clear();
        } else {
            mActionType = 0;
        }
        if (mActionType != 3 && mIsInWarp) {
            mHideTimer = 5;
            mIsInWarp = false;
            mAppearTimer = 15;
        }
    }

    if (mIsHidden) {
        if (!mIsInWarp && mHideTimer <= 0)
            showGhost(true);
    } else if (mIsInWarp || mHideTimer > 0) {
        hideGhost(true);
    }

    if (mActionType != 1) {
        const char* pCurrentAction = al::getActionName(this);
        if (pCurrentAction && al::isEqualString(pCurrentAction, action.cstr())) {
            float frame = 0.0f;
            GhostPlayerFunction::calcGhostPlayDataSklAnimFrame(
                &frame, mPlayData, mFrame, al::isSklAnimOneTime(this, 0));
            if (al::isEqualString(action.cstr(), "KouraIn") &&
                al::isNearZero(frame - al::getSklAnimFrame(this, 0), 0.001f)) {
                mActionType = 1;
                al::startAction(this, "KouraRiding");
            } else if ((mActionType != 2 && mActionType != 3 && mActionType != 4) ||
                       !(frame < al::getSklAnimFrame(this, 0))) {
                al::setSklAnimFrame(this, frame, 0);
            }
            if (mActionType == 3) {
                if (al::getSklAnimFrameMax(this, 0) - 2.0f < frame) {
                    mIsInWarp = true;
                    if (!mIsHidden)
                        hideGhost(true);
                }
                if (!al::isNearZero(al::getTrans(this) - oldTrans, 0.001f) &&
                    al::getSklAnimFrame(this, 0) >= 10.0f) {
                    al::setTrans(this, oldTrans);
                    al::setRotate(this, oldRotate);
                }
            }
        } else {
            al::startAction(this, action.cstr());
        }
    }
    ++mFrame;
}

/** @brief Hides the replay and remembers whether it was waiting or playing. */
void GhostPlayer::stopAndHide(bool immediate) {
    if (al::isNerve(this, &NrvGhostPlayer.Stop) || al::isNerve(this, &NrvGhostPlayer.StopWait))
        return;
    hideGhost(immediate);
    if (al::isNerve(this, &NrvGhostPlayer.Wait))
        al::setNerve(this, &NrvGhostPlayer.StopWait);
    else
        al::setNerve(this, &NrvGhostPlayer.Stop);
}

/** @brief Restores the replay and starts its appearance reaction. */
void GhostPlayer::showGhost(bool immediate) {
    mIsHidden = false;
    al::showModelIfHide(this);
    if (!mNameplate->isAlive())
        mNameplate->appear();
    if (immediate)
        al::onCalcAndDrawEffect(this);
    else
        mAppearTimer = 15;
    al::startHitReactionAppear(this);
    if (mIsShadow)
        al::tryEmitEffect(this, "Trace", nullptr);
}

void GhostPlayer::exeSupportFreeze() {
    al::updateNerveStateAndNextNerve(this, &NrvGhostPlayer.Play);
}

void GhostPlayer::exeStop() {}

/** @brief Seeks to a recorded pipe exit before resuming playback. */
void GhostPlayer::tryStartFromObj(const char* pName) {
    if (al::isDead(this) || mIsShadow || !al::isNerve(this, &NrvGhostPlayer.Play))
        return;
    for (int i = 0; i < GhostPlayerFunction::getWarpObjDataNum(mPlayData); ++i) {
        if (!al::isEqualString(pName, GhostPlayerFunction::getWarpObjData(mPlayData, i)->name))
            continue;
        const auto* pWarp = GhostPlayerFunction::getWarpObjData(mPlayData, i);
        if (!pWarp)
            return;
        mFrame = pWarp->frame;
        if (!mIsHidden) {
            mHideTimer = 5;
            hideGhost(false);
        }
        al::setNerve(this, &NrvGhostPlayer.WaitRestartPlay);
        return;
    }
}

/** @brief Resumes the state remembered by stopAndHide. */
void GhostPlayer::restart() {
    if (al::isNerve(this, &NrvGhostPlayer.StopWait)) {
        showGhost(true);
        al::setNerve(this, &NrvGhostPlayer.Wait);
    } else {
        al::setNerve(this, &NrvGhostPlayer.Play);
        ++mFrame;
    }
}

/** @brief Replays pipe contact only after the ghost has begun moving. */
void GhostPlayer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isNerve(this, &NrvGhostPlayer.Play))
        return;
    if (al::isNerve(this, &NrvGhostPlayer.Play) && al::isGreaterStep(this, 2)) {
        const char* pAction = al::getActionName(this);
        if (pAction && al::isEqualSubString(pAction, "RouteDokanMove")) {
            sead::Vector3f front;
            al::calcFrontDir(&front, this);
            rc::sendMsgRouteDokanPlayerTouch(pOther, pSelf, front);
        }
    }
}

/** @brief Delegates pointer interaction to the support-freeze state. */
bool GhostPlayer::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                       al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, &NrvGhostPlayer.Play) &&
        !al::isNerve(this, &NrvGhostPlayer.SupportFreeze))
        return false;
    if (mIsShadow)
        return false;
    if (!mSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget))
        return false;
    if (al::isNerve(this, &NrvGhostPlayer.Play))
        al::setNerve(this, &NrvGhostPlayer.SupportFreeze);
    return true;
}

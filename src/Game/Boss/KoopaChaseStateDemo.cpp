#include <cstring>
#include "Boss/KoopaChaseStateDemo.hpp"
#include "Boss/KoopaChaseDemoInfo.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Demo/DemoPlayerController.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(KoopaChaseStateDemo, Wait);
NERVE_DECL(KoopaChaseStateDemo, Start);
NERVE_DECL(KoopaChaseStateDemo, Action);
NERVE_DECL(KoopaChaseStateDemo, Fade);
NERVE_DECL(KoopaChaseStateDemo, FadeDone);
NERVES_MAKE_STRUCT(KoopaChaseStateDemo, Wait, Start)
NERVES_MAKE_NOSTRUCT(KoopaChaseStateDemo, Action, Fade, FadeDone)
} // namespace
/** @brief Creates the chase cutscene, player return positions, and skip UI. @param pName State
 * name. @param pHost Chase actor. @param rInfo Initialization data. @param pDemoInfo Cutscene
 * settings. */
KoopaChaseStateDemo::KoopaChaseStateDemo(const char *pName, al::LiveActor *pHost,
                                         const al::ActorInitInfo &rInfo,
                                         KoopaChaseDemoInfo *pDemoInfo)
    : ActorStateBase(pName, pHost), mInfo(pDemoInfo), mStartRecorderId(""), mEndRecorderId("") {
    initNerve(&NrvKoopaChaseStateDemo.Wait, 0);
    if (al::calcLinkChildNum(rInfo, "PlayerPos") > 0) {
        al::PlacementInfo info;
        al::getLinksInfo(&info, al::getPlacementInfo(rInfo), "PlayerPos");
        al::tryGetTrans(&mReturnPos, info);
        al::tryGetQuat(&mReturnQuat, info);
    } else {
        al::tryGetTrans(&mReturnPos, rInfo);
        mReturnPos += sead::Vector3f(0, 0, 1000);
        al::tryGetQuat(&mReturnQuat, rInfo);
    }
    mActors.allocBuffer(8, nullptr);
    al::PlacementId id;
    al::tryGetPlacementID(&id, rInfo);
    mStartRecorderId.format("%s%sS", id.mPlacementID, id.mZoneID ? id.mZoneID : "");
    mEndRecorderId.format("%s%sE", id.mPlacementID, id.mZoneID ? id.mZoneID : "");
    mSkipLayout = new DemoSkipLayout(al::getLayoutInitInfo(rInfo), false);
    mWipe = new al::WipeSimple("黒フェードワイプ", "WipeFadeBlack", al::getLayoutInitInfo(rInfo),
                               nullptr);
}
/** @brief Destroys the chase cutscene state. */
KoopaChaseStateDemo::~KoopaChaseStateDemo() = default;
/** @brief Suspends ghost players and requests cutscene control. */
void KoopaChaseStateDemo::appear() {
    mIsDead = false;
    mSkipLayout->appear();
    rc::disappearCameraChangeLayout(mHostActor);
    rc::stopAndHideGhostPlayerAll(mHostActor);
    rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mStartRecorderId.cstr(), true);
    PlayerAliveWatcher::tryGetPlayerAliveWatcher(mHostActor)->addBubbleDelayTime(100);
    if (rc::requestStartDemoPlayer(mHostActor))
        al::setNerve(this, &NrvKoopaChaseStateDemo.Start);
    else
        al::setNerve(this, &NrvKoopaChaseStateDemo.Wait);
}
/** @brief Sets the player return position. */
void KoopaChaseStateDemo::setReturnTrans(const sead::Vector3f &rTrans) { mReturnPos.e = rTrans.e; }
/** @brief Sets the player return position and orientation. */
void KoopaChaseStateDemo::setReturnTransAndQuat(const sead::Vector3f &rTrans,
                                                const sead::Quatf &rQuat) {
    mReturnPos.e = rTrans.e;
    std::memcpy(&mReturnQuat, &rQuat, sizeof(mReturnQuat));
}
/** @brief Registers an actor animated by the cutscene. */
void KoopaChaseStateDemo::entryDemoActor(al::LiveActor *pActor) { mActors.pushBack(pActor); }
/** @brief Ends the camera and restores normal play. @param skipped Whether the cutscene was
 * skipped. */
void KoopaChaseStateDemo::end(bool skipped) {
    mSkipLayout->kill();
    al::requestCaptureScreenCover(mHostActor, 2);
    rc::appearCameraChangeLayout(mHostActor);
    mIsSkipped = skipped;
    if (mInfo->mValue20 > 0)
        rc::setAlreadyShowBossDemo(mHostActor);
    auto *info = mInfo;
    if (mIsSkipped)
        al::endCamera(info->mActor, info->mCameraInfo, 0);
    else
        al::endCamera(info->mActor, info->mCameraInfo, info->mValue1C);
    if (mInfo->mName28)
        al::tryOffStageSwitch(mHostActor, mInfo->mName28);
    if (!mIsDead) {
        rc::requestEndDemoPlayer(mHostActor);
        rc::restartGhostPlayerAll(mHostActor);
        rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr(), false);
        rc::tryStartFromObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr());
    }
    kill();
}
/** @brief Immediately ends the cutscene as skipped. */
void KoopaChaseStateDemo::forceKill() { end(true); }
/** @brief Delays player bubbles while waiting for demo control. */
void KoopaChaseStateDemo::exeWait() {
    PlayerAliveWatcher::tryGetPlayerAliveWatcher(mHostActor)->addBubbleDelayTime(100);
    if (rc::requestStartDemoPlayer(mHostActor))
        al::setNerve(this, &NrvKoopaChaseStateDemo.Start);
}
/** @brief Lines up the players and starts the camera. */
void KoopaChaseStateDemo::exeStart() {
    placePlayersToReturnPos();
    int count = rc::getDemoPlayerNum(mHostActor);
    for (int i = 0; i < count; i++) {
        auto *player = rc::getDemoPlayer(mHostActor, i);
        const auto &front = player->getFrontVec();
        const auto &up = player->getUpVec();
        sead::Vector3f pos;
        if (count == 1)
            pos.e = mReturnPos.e;
        else {
            sead::Vector3f side;
            sead::Vector3f frontCopy = front;
            sead::Vector3f upCopy = up;
            side.setCross(frontCopy, upCopy);
            switch (count) {
            case 2:
                switch (i) {
                case 0:
                    pos = mReturnPos - side * 150.0f * 0.5f;
                    break;
                case 1:
                    pos = mReturnPos + side * 150.0f * 0.5f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    pos = mReturnPos - side * 150.0f;
                    break;
                case 1:
                    pos.e = mReturnPos.e;
                    break;
                case 2:
                    pos = mReturnPos + side * 150.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    pos = mReturnPos - side * 150.0f * 1.5f;
                    break;
                case 1:
                    pos = mReturnPos - side * 150.0f * 0.5f;
                    break;
                case 2:
                    pos = mReturnPos + side * 150.0f * 0.5f;
                    break;
                case 3:
                    pos = mReturnPos + side * 150.0f * 1.5f;
                    break;
                }
                break;
            }
        }
        player->setTrans(pos);
    }
    al::startAnimCamera(mInfo->mActor, mInfo->mCameraInfo, mInfo->mName, mInfo->mValue18);
    al::setNerve(this, &NrvKoopaChaseStateDemoAction);
}
/** @brief Restores player positions and orientations. */
void KoopaChaseStateDemo::placePlayersToReturnPos() {
    rc::showDemoPlayerAll(mHostActor);
    int count = rc::getDemoPlayerNum(mHostActor);
    sead::Vector3f side(0, 0, 0);
    al::calcQuatSide(&side, mReturnQuat);
    side.negate();
    for (int i = 0; i < count; i++) {
        auto *player = rc::getDemoPlayer(mHostActor, i);
        float offset = count == 1 ? 0.0f : (i - (count - 1) * 0.5f) * 150.0f;
        sead::Vector3f pos = mReturnPos;
        pos += offset * side;
        player->setTrans(pos);
        player->setQuat(mReturnQuat);
    }
}
/** @brief Runs the cutscene actors and handles skip requests. */
void KoopaChaseStateDemo::exeAction() {
    if (al::isFirstStep(this)) {
        if (mInfo->mName28)
            al::tryOnStageSwitch(mHostActor, mInfo->mName28);
        al::showModelIfHide(mHostActor);
        al::showModelIfHide(KoopaChaseFunction::getKoopa(static_cast<KoopaChase *>(mHostActor)));
        for (auto it = mActors.begin(); it != mActors.end(); ++it) {
            if (al::isDead(&*it))
                it->appear();
            al::startAction(&*it, mInfo->mName);
        }
        rc::hideDemoPlayerAll(mHostActor);
        for (int i = 0, count = rc::getDemoPlayerNum(mHostActor); i < count; i++)
            rc::getDemoPlayer(mHostActor, i)->stopSklAnimAndDeleteEffect();
    }
    if (mInfo->mValue20 > 0 && al::isGreaterEqualStep(this, mInfo->mValue20) &&
        rc::tryCancelBossDemo(mHostActor)) {
        end(true);
        return;
    }
    sead::BitFlag16 ports(rc::getActiveInputPortList(GameDataHolderAccessor(mSkipLayout)));
    if (mSkipLayout->isSkip(ports))
        al::setNerve(this, &NrvKoopaChaseStateDemoFade);
    else if (al::isEndAnimCamera(mInfo->mCameraInfo))
        end(false);
}
/** @brief Closes the screen before skipping. */
void KoopaChaseStateDemo::exeFade() {
    if (al::isFirstStep(this)) {
        if (mInfo->mFlag30)
            rc::forceEndPlayerDeadAnim(mHostActor);
        mWipe->startClose(-1);
    }
    if (mWipe->isCloseEnd()) {
        al::setNerve(this, &NrvKoopaChaseStateDemoFadeDone);
        al::requestCaptureScreenCover(mHostActor, 3);
    }
}
/** @brief Ends the skipped cutscene and reopens the screen. */
void KoopaChaseStateDemo::exeFadeDone() {
    mSkipLayout->kill();
    mWipe->kill();
    end(true);
    mWipe->startOpen(-1);
}

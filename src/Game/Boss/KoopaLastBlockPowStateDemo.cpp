#include "Boss/KoopaLastBlockPowStateDemo.hpp"
#include "Scene/ProjectActorFactoryTypes.hpp"
#include "Demo/DemoPlayerController.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(KoopaLastBlockPowStateDemoEnd, Start);
NERVE_DECL(KoopaLastBlockPowStateDemoEnd, DamageWait);
NERVE_DECL(KoopaLastBlockPowStateDemoEnd, Demo);
NERVE_DECL(KoopaLastBlockPowStateDemoEnd, FadeOut);
NERVE_DECL(KoopaLastBlockPowStateDemoEnd, FadeIn);
NERVE_DECL(KoopaLastBlockPowStateDemoEnd, End);
NERVES_MAKE_NOSTRUCT(KoopaLastBlockPowStateDemoEnd, Start, DamageWait, Demo, FadeOut, FadeIn, End)
}
/** @brief Creates the ending demo camera, recorder IDs, and wipe. @param pHost POW block. @param pDefender Bowser actor. @param rInfo Initialization data. */
KoopaLastBlockPowStateDemoEnd::KoopaLastBlockPowStateDemoEnd(KoopaLastBlockPow* pHost, KoopaLastDefendeBlockPow* pDefender, const al::ActorInitInfo& rInfo)
    : al::ActorStateBase("終了デモ", pHost), mBlock(pHost), mDefender(pDefender),
      mStartRecorderId("EndKoopaLastStr"), mEndRecorderId("EndKoopaLastEnd") {
    initNerve(&NrvKoopaLastBlockPowStateDemoEndStart, 0);
    mCameraInfo = al::initAnimCamera(pDefender, rInfo);
    al::tryGetLinksQT(&mTargetQuat, &mTargetPos, rInfo, "TargetPos");
    mWipe = new al::WipeSimple("白フェードワイプ", "WipeFadeWhite", al::getLayoutInitInfo(rInfo), nullptr);
}
/** @brief Destroys the ending demo state. */
KoopaLastBlockPowStateDemoEnd::~KoopaLastBlockPowStateDemoEnd() = default;
/** @brief Starts the ending-demo sequence. */
void KoopaLastBlockPowStateDemoEnd::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvKoopaLastBlockPowStateDemoEndStart);
}
/** @brief Stops ghost players and waits for demo player control. */
void KoopaLastBlockPowStateDemoEnd::exeStart() {
    if (al::isFirstStep(this)) {
        rc::stopAndHideGhostPlayerAll(mHostActor);
        rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mStartRecorderId.cstr(), true);
    }
    if (rc::requestStartDemoPlayer(mHostActor)) {
        int count = rc::getDemoPlayerNum(mHostActor);
        for (int i = 0; i < count; i++) rc::getDemoPlayer(mHostActor, i)->stopSklAnimAndDeleteEffect();
        al::setNerve(this, &NrvKoopaLastBlockPowStateDemoEndDamageWait);
    }
}
/** @brief Waits for the damage animation before starting the ending camera. */
void KoopaLastBlockPowStateDemoEnd::exeDamageWait() {
    if (al::isActionEnd(mDefender)) {
        al::requestCaptureScreenCover(mHostActor, 2);
        rc::disappearCameraChangeLayout(mHostActor);
        al::setNerve(this, &NrvKoopaLastBlockPowStateDemoEndDemo);
    }
}
/** @brief Runs the ending camera and hides demo players. */
void KoopaLastBlockPowStateDemoEnd::exeDemo() {
    if (al::isFirstStep(this)) {
        al::startAnimCamera(mDefender, mCameraInfo, "DemoBattleLastKoopaEnd", mDefender->getBaseMtx(), 0);
        mDefender->startEndDemo();
        rc::hideDemoPlayerAll(mHostActor);
        al::tryOnStageSwitch(mHostActor, "SwitchDemoEndOn");
    }
    if (al::isEndAnimCamera(mCameraInfo)) al::setNerve(this, &NrvKoopaLastBlockPowStateDemoEndFadeOut);
}
/** @brief Fades out, restores players, and moves them to the target pose. */
void KoopaLastBlockPowStateDemoEnd::exeFadeOut() {
    if (al::isFirstStep(this)) mWipe->startClose(60);
    if (mWipe->isCloseEnd()) {
        al::endCamera(mDefender, mCameraInfo, 0);
        al::tryOffStageSwitch(mHostActor, "SwitchDemoEndOn");
        rc::showDemoPlayerAll(mHostActor);
        rc::replaceDemoPlayerAll(mHostActor, mTargetPos, mTargetQuat, 150.0f);
        rc::startActionDemoPlayerAll(mHostActor, "Wait");
        int count = rc::getDemoPlayerNum(mHostActor);
        for (int i = 0; i < count; i++) rc::validatePlayerDamage(rc::getDemoPlayer(mHostActor, i)->getPlayerActor());
        al::setNerve(this, &NrvKoopaLastBlockPowStateDemoEndFadeIn);
    }
}
/** @brief Opens the wipe and waits for it to finish. */
void KoopaLastBlockPowStateDemoEnd::exeFadeIn() {
    if (al::isStep(this, 60)) mWipe->startOpen(60);
    if (mWipe->isAlive()) return;
    al::setNerve(this, &NrvKoopaLastBlockPowStateDemoEndEnd);
}
/** @brief Restores player recording and the camera-change layout. */
void KoopaLastBlockPowStateDemoEnd::exeEnd() {
    rc::requestEndDemoPlayer(mHostActor);
    rc::restartGhostPlayerAll(mHostActor);
    rc::setPlacementIdObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr(), false);
    rc::tryStartFromObjGhostPlayerRecorder(mHostActor, mEndRecorderId.cstr());
    rc::appearCameraChangeLayout(mHostActor);
    kill();
}

#include "MapObj/DoorLock.hpp"
#include "MapObj/DoorKey.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(DoorLock, ClosedWait);
NERVE_DECL(DoorLock, Open);
NERVE_DECL(DoorLock, WaitKeyDisappear);
NERVE_DECL(DoorLock, ClosedReaction);
NERVE_DECL(DoorLock, OpenWait);
NERVES_MAKE_STRUCT(DoorLock, ClosedWait, Open, WaitKeyDisappear, ClosedReaction, OpenWait)
}
DoorLock::DoorLock(const char* name) : al::LiveActor(name), mBreakModel(new al::LiveActor("DoorLockBreak")) {}
DoorLock::~DoorLock() {}
void DoorLock::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvDoorLock.ClosedWait, 0);
    bool complete = SingleModeDataFunction::isIslandScenarioIDComplete(this, info);
    al::initActorWithArchiveName(mBreakModel, info, "DoorLockBreak", nullptr);
    mBreakModel->makeActorDead();
    al::initSubActorKeeperNoFile(this, info, 1);
    mCollision = al::createCollisionObj(this, info, "DoorLock", al::getHitSensor(this, "DoorLock"), nullptr, nullptr);
    al::registerSubActorSyncClipping(this, mCollision, false);
    al::validateCollisionParts(mCollision);
    mCollision->makeActorAppeared();
    al::tryGetZoneID(&mZoneId, *info.mPlacementInfo);
    bool connect = false;
    if (al::tryGetArg(&connect, info, "IsConnectCollision") && connect) mConnector = al::createMtxConnector(this);
    al::tryGetArg(&mCutScene, info, "isCutScene");
    if (mCutScene) {
        al::tryGetArg(&mCutSceneDuration, info, "cutSceneDuration");
        mCamera = al::initObjectCamera_RS(this, info, nullptr);
        al::setFixActorCameraTarget(mCamera, this);
    }
    if (al::calcLinkChildNum(info, "DoorKey") == 1) {
        mLinkedKey = new DoorKey("DoorKey");
        al::initLinksActor(mLinkedKey, info, "DoorKey", 0);
    }
    if (complete) makeActorDead();
    else makeActorAppeared();
}
void DoorLock::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
}
void DoorLock::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
    if (mExplosionCooldown) --mExplosionCooldown;
}
void DoorLock::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorName(sender, "Push") && al::isNerve(this, &NrvDoorLock.Open) &&
        !al::isGreaterEqualStep(this, 120) && al::isSensorPlayer(receiver))
        al::sendMsgPushAndKillVelocityToTarget(this, sender, receiver);
}
bool DoorLock::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isNerve(this, &NrvDoorLock.ClosedWait)) return false;
    if (al::isSensorMapObj(receiver) && !al::isSensorName(receiver, "PushReaction") && al::isMsgKeyOpen(msg)) {
        mOpeningKey = static_cast<DoorKey*>(al::getSensorHost(sender));
        if (mLinkedKey) mLinkedKey->startUnlock();
        al::setNerve(this, &NrvDoorLock.WaitKeyDisappear);
        return true;
    }
    if (al::isMsgPlayerDisregard(msg)) return false;
    if (al::isMsgExplosion(msg) && al::isSensorName(sender, "AmiiboExplosion")) {
        if (mExplosionCooldown) return false;
        mExplosionCooldown = 115;
    }
    if (al::isMsgPlayerRollingAttack(msg) || al::isMsgPlayerHipDropAll(msg) ||
        al::isMsgPlayerClimbAttack(msg) || al::isMsgPlayerTailAttack(msg) ||
        al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerKouraAttack(msg) ||
        al::isMsgPlayerSpinAttack(msg) || al::isMsgBallAttack(msg) ||
        al::isMsgKickKouraReflect(msg) || al::isMsgNekoAttack(msg) || al::isMsgExplosion(msg) ||
        al::isMsgExplosionCollide(msg) || rc::isMsgBobsledBodyAttack(msg)) {
        al::setNerve(this, &NrvDoorLock.ClosedReaction);
        return !al::isMsgNekoAttack(msg);
    }
    return false;
}
void DoorLock::exeClosedWait() {
    if (al::isFirstStep(this)) al::startAction(this, "CloseWait");
}
void DoorLock::exeClosedReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "Slam");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvDoorLock.ClosedWait);
}
void DoorLock::exeWaitKeyDisappear() {
    mOpeningKey->updateOpenThrowPose(5);
    if (al::isGreaterEqualStep(this, 5)) {
        if (mLinkedKey) mLinkedKey->triggerKillForce(false);
        al::setNerve(this, &NrvDoorLock.Open);
    }
}
void DoorLock::exeOpen() {
    if (al::isFirstStep(this)) {
        if (mCutScene) {
            if (!rc::requestStartDemoInGameCutscene(this)) { al::setNerve(this, &NrvDoorLock.Open); return; }
            rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(3));
            rc::addDemoActor(this);
            rc::addDemoActor(mBreakModel);
            auto* controller = DisasterModeController::tryGetController(this);
            if (controller) { rc::addDemoActor(controller); controller->pause(true); }
            al::startCamera_RS(this, mCamera, -1);
        }
        mBreakModel->makeActorAppeared();
        al::startAction(mBreakModel, "Open");
    }
    if (al::isStep(this, 1)) al::hideModelIfShow(this);
    if (al::isStep(this, 42)) al::startHitReaction(this, "シーンストップ用");
    if (al::isStep(this, 120)) {
        al::invalidateCollisionParts(mCollision);
        al::invalidateCollisionParts(this);
    }
    if (al::isActionEnd(mBreakModel)) al::setNerve(this, &NrvDoorLock.OpenWait);
}
void DoorLock::exeOpenWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OpenWait");
        al::startHitReaction(this, "扉消失");
    }
    if (mCutScene && al::isStep(this, mCutSceneDuration)) {
        rc::requestEndDemoInGameCutscene(this);
        auto* controller = DisasterModeController::tryGetController(this);
        if (controller) controller->resume(true);
        al::endCamera_RS(this, mCamera, -1, false);
        al::tryOnStageSwitch(this, "SwitchGoalItemGetOn");
        mBreakModel->kill();
        kill();
    }
}
bool DoorLock::showActor() { al::LiveActor::showActor(); al::validateCollisionParts(mCollision); return true; }
bool DoorLock::hideActor() { al::LiveActor::hideActor(); al::invalidateCollisionParts(mCollision); return true; }

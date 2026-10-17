#include "MapObj/Coin.hpp"
#include "MapObj/CoinRotater.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/CoinUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(Coin, Wait)
NERVE_DECL(Coin, Spin)
NERVE_DECL(Coin, AppearJumpRise)
NERVE_DECL(Coin, AppearJumpFall)
NERVES_MAKE_NOSTRUCT(Coin, AppearJumpRise, AppearJumpFall)
NERVES_MAKE_STRUCT(Coin, Wait, Spin)
}
Coin::Coin(const char* name) : al::LiveActor(name) {}
void Coin::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "Coin", al::isSingleMode(info) ? "SM" : nullptr);
    al::initNerve(this, &NrvCoin.Wait, 1);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    mConnector = al::tryCreateMtxConnector(this, info);
    if (mConnector) al::tryGetArg(&mCollisionCheckDist, info, "CollisionCheckDist");
    resetBaseQuat();
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, mBaseQuat);
    if (!al::isNearZero(up.cross(sead::Vector3f(0.0f, 0.0f, 1.0f)), 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f(0.0f, 0.0f, 1.0f));
    al::listenStageSwitchOnKill(this, al::FunctorV0M(this, &Coin::killBySwitch));
    al::tryGetArg(&mInRouteDokan, info, "IsPlacementInRouteDokan");
    al::trySetShadowLength(this, info, "Coin");
    al::tryGetArg(&mDropShadowActorDown, info, "IsDropShadowActorDown");
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    if (mConnector) mAssistRotate->setConnector(mConnector, mBaseQuat);
    al::initNerveState(this, mAssistRotate, &NrvCoin.Spin, "回転");
    al::isClassName(info, "Coin");
    al::updateSeMaterialBeyondWall(this, mInRouteDokan);
    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &Coin::appearBySwitch)))
        makeActorDead();
    else
        makeActorAppeared();
    if (al::listenStageSwitchOnOff(this, "SwitchTimerAppear", al::FunctorV0M(this, &Coin::show), al::FunctorV0M(this, &Coin::hide)))
        hide();
    else
        show();
    mSingleMode = info.getActorSceneInfo().isSingleMode;
    if (mSingleMode) mFileID = info.getPlacementInfo()._28;
}
void Coin::killBySwitch() {
    if (al::isDead(this)) return;
    al::startHitReaction(this, "消滅[スイッチ]");
    kill();
}
void Coin::appearBySwitch() {
    if (mEnableAppear) {
        appear();
        if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, 50.0f, mCollisionCheckDist);
    }
}
void Coin::show() {
    if (mSingleMode) appearBySwitch();
    al::LiveActor::showActor();
}
void Coin::hide() {
    if (mSingleMode) killBySwitch();
    al::LiveActor::hideActor();
}
void Coin::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, 50.0f, mCollisionCheckDist);
    al::updateMaterialCodeWater(this);
    if (mSingleMode) {
        auto* watcher = static_cast<CloudBonusWatcher*>(al::getSceneObj(this, 52));
        if (watcher) watcher->tryRegisterActor(this, mFileID);
    }
}
void Coin::reappear() { appear(); }
void Coin::kill() { al::LiveActor::kill(); al::tryOnSwitchDeadOn(this); }
void Coin::control() {
    if (al::isNerve(this, &NrvCoin.Wait)) CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvCoin.Spin);
}
bool Coin::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (rc::isMsgStartGoalDemoHouse(msg)) { makeActorDead(); return true; }
    if (isEnableMsgItemGet(msg)) {
        rc::acquirerItemCoin(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}
bool Coin::isEnableMsgItemGet(const al::SensorMsg* msg) const {
    if (mInRouteDokan) return rc::isMsgRouteDokanItemGet(msg);
    return rc::isMsgCoinGet(msg);
}
bool Coin::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(msg, this, pointer)) { kill(); return true; }
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (al::isNerve(this, &NrvCoin.Wait)) al::setNerve(this, &NrvCoin.Spin);
        return true;
    }
    return false;
}
void Coin::appearJump() {
    appear();
    al::invalidateClipping(this);
    al::invalidateHitSensors(this);
    mJumpStart.set(al::getTrans(this));
    al::setNerve(this, &NrvCoinAppearJumpRise);
}
void Coin::resetBaseQuat() { mBaseQuat.set(al::getQuat(this)); }
void Coin::rotate() {
    if (mConnector) {
        sead::Quatf quat;
        al::rotateQuatYDirDegree(&quat, mBaseQuat, rc::getCoinRotateY(this));
        al::connectPoseQT(this, mConnector, quat, al::getConnectBaseTrans(mConnector));
    } else {
        al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
    }
}
void Coin::exeAppearJumpRise() {
    rotate();
    al::getTransPtr(this)->y = al::calcNerveEaseOutValue(this, 12, mJumpStart.y, mJumpStart.y + 200.0f);
    al::setNerveAtStep(this, &NrvCoinAppearJumpFall, 12);
}
void Coin::exeAppearJumpFall() {
    if (al::isFirstStep(this)) al::validateHitSensors(this);
    rotate();
    al::getTransPtr(this)->y = al::calcNerveEaseInValue(this, 12, mJumpStart.y + 200.0f, mJumpStart.y);
    if (al::isGreaterEqualStep(this, 12)) {
        al::setNerve(this, &NrvCoin.Wait);
        al::validateClipping(this);
    }
}
void Coin::exeWait() {
    rotate();
    if (mDropShadowActorDown) al::setShadowDropDirActorDown(this);
}
void Coin::exeSpin() {
    if (al::isFirstStep(this)) al::startSe(this, "PgTouched", nullptr);
    if (mDropShadowActorDown) al::setShadowDropDirActorDown(this);
    al::updateNerveStateAndNextNerve(this, &NrvCoin.Wait);
}

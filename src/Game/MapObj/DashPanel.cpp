#include "MapObj/DashPanel.hpp"
#include "MapObj/DashPanelSeTriggerChecker.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
DashPanel::DashPanel(const char* name) : al::LiveActor(name) {}
void DashPanel::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::tryGetArg(&mDashFrames, info, "DashFrame");
    al::tryGetArg(&mNoConnectCollision, info, "NoConnectCollision");
    al::tryGetArg(&mShowShadow, info, "IsShowShadowMask");
    al::tryGetArg(&mModifiedSpeed, info, "IsModifiedSpeed");
    if (!mNoConnectCollision) mConnector = al::createMtxConnector(this);
    if (!mShowShadow) al::invalidateShadow(this);
    makeActorAppeared();
    al::setEffectFollowMtxPtr(this, "Blur", &mEffectMtx);
    mSeTrigger = new DashPanelSeTriggerChecker();
    mSeTrigger->init(5, 40, info.getActorSceneInfo().isSingleMode);
    if (al::listenStageSwitchOnOffAppear(this, al::Functor(static_cast<al::LiveActor*>(this), &al::LiveActor::appear),
                                              al::Functor(static_cast<al::LiveActor*>(this), &al::LiveActor::kill)))
        mHasAppearSwitch = true;
}
void DashPanel::initNoPlacement(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "DashPanel", nullptr);
    mNoConnectCollision = true;
    al::setEffectFollowMtxPtr(this, "Blur", &mEffectMtx);
    mSeTrigger = new DashPanelSeTriggerChecker();
    mSeTrigger->init(5, 40, info.getActorSceneInfo().isSingleMode);
    makeActorDead();
}
void DashPanel::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    if (mHasAppearSwitch) kill();
}
void DashPanel::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!((al::isSensorPlayer(receiver) && rc::isPlayerOnGround(receiver)) ||
          al::isSensorRide(receiver) || al::isSensorKickKoura(receiver))) return;
    sead::Vector3f side, front, up;
    al::calcSideDir(&side, this);
    al::calcFrontDir(&front, this);
    al::calcUpDir(&up, this);
    sead::Vector3f offset = al::getTrans(al::getSensorHost(receiver)) - al::getTrans(this);
    if (sead::Mathf::abs(offset.dot(front)) > 200.0f ||
        sead::Mathf::abs(offset.dot(side)) > 200.0f ||
        sead::Mathf::abs(offset.dot(up)) > 30.0f) return;
    bool accepted = mModifiedSpeed ? rc::sendMsgModifiedDashPanel(receiver, sender, mDashFrames)
                                   : rc::sendMsgDashPanel(receiver, sender, mDashFrames);
    if (!accepted) return;
    int user = al::isSensorPlayer(receiver) ? alPlayerFunction::findPlayerHolderIndex(receiver)
                                         : rc::tryFindRelativeControlUserId(receiver);
    bool plessie = al::isSensorPlessie(receiver);
    if (user >= 0 && mSeTrigger->tryTrigger(user, plessie)) al::startSe(this, "DashPanelStart", nullptr);
    if (mActionCooldown <= 0) {
        mActionCooldown = 10;
        al::startAction(this, "Start");
        if (al::isSensorRide(receiver)) al::tryEmitEffect(this, "Blur", nullptr);
    }
}
void DashPanel::control() {
    if (mActionCooldown - 1 >= 0) --mActionCooldown;
    mSeTrigger->update();
    if (!mNoConnectCollision) {
        al::connectPoseQT(this, mConnector);
        if (mShowShadow) al::setShadowDropDirActorDown(this);
    }
}
void DashPanel::reappear() { al::LiveActor::appear(); }

#include "MapObj/JumpPanel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Player/PlayerBindEndParam.hpp"
namespace {
NERVE_DECL(JumpPanel, Wait)
NERVE_DECL(JumpPanel, TouchWait)
NERVE_DECL(JumpPanel, Jump)
NERVE_DECL(JumpPanel, BindWait)
NERVES_MAKE_NOSTRUCT(JumpPanel, Wait, TouchWait, Jump, BindWait)
const PlayerBindEndParam sBindEndParam("Jump", 1, 10, true, true, true, 1, 1.2f, 0);
}
JumpPanel::JumpPanel(const char* name) : al::LiveActor(name) {}
JumpPanel::~JumpPanel() = default;
void JumpPanel::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvJumpPanelWait, 0);
    const char* name = nullptr;
    al::tryGetArg(&mJumpPower, info, "JumpPower");
    if (al::tryGetObjectName(&name, info) && al::isEqualString(name, "SuperJumpPanel")) {
        mIsSuper = true;
        mJumpPower = 72.0f;
    }
    mConnector = al::tryCreateMtxConnector(this, info);
    if (mConnector) _142 = 1;
    mCollision = al::createCollisionObj(this, info, "PlayerOnly", al::getHitSensor(this, "CollisionParts"), nullptr, "PlayerOnly");
    mIsSingleMode = al::isSingleMode(info);
    if (mIsSingleMode) {
        al::validateClipping(this);
        al::addToHostActorClipping(mCollision, this);
        al::validateClipping(mCollision);
        al::startAction(this, "Wait");
    }
    mCollision->makeActorAppeared();
    if (al::listenStageSwitchOnAppear(this, al::Functor(this, &JumpPanel::appear))) {
        if (mIsSingleMode) mCollision->makeActorDead();
        makeActorDead();
    } else if (al::listenStageSwitchOnOff(this, "SwitchOnOffAppear", al::Functor(this, &JumpPanel::onPanelAppear), al::Functor(this, &JumpPanel::offPanelAppear))) {
        mAttachOnAppear = true;
        if (mIsSingleMode) mCollision->makeActorDead();
        makeActorDead();
    } else makeActorAppeared();
}
void JumpPanel::onPanelAppear() {
    if (mIsSingleMode) {
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvJumpPanelWait);
    }
    mCollision->appear();
    appear();
}
void JumpPanel::offPanelAppear() { kill(); mCollision->kill(); }
void JumpPanel::initAfterPlacement() {
    if (mConnector && !mAttachOnAppear) al::attachMtxConnectorToCollision(mConnector, this, false);
}
bool JumpPanel::hideActor() { mCollision->hideActor(); return al::LiveActor::hideActor(); }
bool JumpPanel::showActor() { mCollision->showActor(); return al::LiveActor::showActor(); }
void JumpPanel::appear() { al::LiveActor::appear(); if (mIsSingleMode) mCollision->appear(); }
void JumpPanel::makeActorDead() { al::LiveActor::makeActorDead(); if (mIsSingleMode) mCollision->kill(); }
void JumpPanel::control() {
    if (mJumpInputFrames - 1 >= 0) --mJumpInputFrames;
    if (mConnector) al::connectPoseQT(this, mConnector);
}
void JumpPanel::movementPaused(bool paused) {
    if (mConnector) al::connectPoseQT(this, mConnector);
    al::LiveActor::movementPaused(paused);
}
bool JumpPanel::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor*) {
    if (al::isMsgPlayerFloorTouch(msg)) {
        if (al::isNerve(this, &NrvJumpPanelWait) || al::isNerve(this, &NrvJumpPanelTouchWait)) {
            if (rc::isPlayerInputJumpTrigOn(al::getSensorHost(other), mIsSingleMode)) {
                mKoopaJrHipDrop = false;
                mJumpInputFrames = 2;
            }
        }
        return false;
    }
    if (al::isSensorKoopaJr(other) && al::isMsgPlayerHipDropAll(msg) &&
        (al::isNerve(this, &NrvJumpPanelWait) || al::isNerve(this, &NrvJumpPanelTouchWait))) {
        mKoopaJrHipDrop = true;
        return true;
    }
    if (al::isMsgBindStart(msg) || al::isMsgBindInit(msg) || al::isMsgBindCancel(msg)) return true;
    if (al::isMsgTouchAssist(msg)) {
        mTouchAssist = true;
        if (al::isNerve(this, &NrvJumpPanelWait)) al::setNerve(this, &NrvJumpPanelTouchWait);
        return true;
    }
    return false;
}
void JumpPanel::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        if (mAttachOnAppear) {
            mAttachOnAppear = false;
            al::attachMtxConnectorToCollision(mConnector, this, false);
        }
    }
    if (mPlayers.size() != 0) {
        bool jump = false;
        for (int i = 0; i < mPlayers.size(); ++i) jump |= rc::getPlayerKeyConfig(mPlayers.at(i))->isPadTriggerPlayerJump();
        if ((jump | (mJumpInputFrames > 0)) || mKoopaJrHipDrop) {
            for (int i = 0; i < mPlayers.size(); ++i) rc::requestPlayerBind(mPlayers.at(i), al::getHitSensor(this, "Bind"));
            al::setNerve(this, &NrvJumpPanelBindWait);
            return;
        }
        mPlayers.clear();
    } else if (mKoopaJrHipDrop) al::setNerve(this, &NrvJumpPanelJump);
}
void JumpPanel::exeTouchWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TouchWait");
        mTouchFrames = 0;
        mTouchReleaseFrames = 0;
    }
    bool jump = false;
    if (mTouchAssist) { ++mTouchFrames; mTouchReleaseFrames = 0; mTouchAssist = false; }
    else if (mTouchFrames >= 4) {
        if (mTouchReleaseFrames++ >= 4) { mTouchFrames = 0; mTouchReleaseFrames = 0; jump = true; }
    } else {
        al::setNerve(this, &NrvJumpPanelWait);
        mTouchFrames = 0;
        mTouchReleaseFrames = 0;
    }
    if (mPlayers.size() != 0) {
        for (int i = 0; i < mPlayers.size(); ++i) jump |= rc::getPlayerKeyConfig(mPlayers.at(i))->isPadTriggerPlayerJump();
        if (jump || mJumpInputFrames > 0 || mKoopaJrHipDrop) {
            for (int i = 0; i < mPlayers.size(); ++i) rc::requestPlayerBind(mPlayers.at(i), al::getHitSensor(this, "Bind"));
            al::setNerve(this, &NrvJumpPanelBindWait);
            return;
        }
        mPlayers.clear();
    } else if (jump || mKoopaJrHipDrop) al::setNerve(this, &NrvJumpPanelJump);
}
void JumpPanel::exeBindWait() {
    int count = 0;
    for (int i = 0; i < mPlayers.size(); ++i) {
        if (rc::isPlayerBinded(mPlayers.at(i), al::getHitSensor(this, "Bind"))) {
            ++count;
            auto* puppet = rc::startPuppet(al::getHitSensor(this, "Bind"), mPlayers.at(i));
            sead::Vector3f up;
            al::calcUpDir(&up, this);
            sead::Vector3f velocity;
            al::verticalizeVec(&velocity, up, rc::getPuppetVelocity(puppet));
            velocity = up * mJumpPower + velocity;
            rc::setPuppetVelocity(puppet, velocity);
            rc::endBindAndPuppetNull(&puppet, &sBindEndParam);
        }
    }
    mPlayers.clear();
    if (count > 0) al::setNerve(this, &NrvJumpPanelJump);
    else if (mIsSingleMode) al::setNerve(this, &NrvJumpPanelWait);
    else al::setNerve(this, &NrvJumpPanelJump);
}
void JumpPanel::exeJump() {
    if (al::isFirstStep(this)) { al::startAction(this, "Jump"); mKoopaJrHipDrop = false; }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvJumpPanelWait);
}
void JumpPanel::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    float width = mIsSuper ? 300.0f : 200.0f;
    if (mIsSingleMode) width += 15.0f;
    if ((al::isNerve(this, &NrvJumpPanelWait) || al::isNerve(this, &NrvJumpPanelTouchWait)) &&
        al::isSensorPlayer(other) && rc::isPlayerOnGround(other)) {
        for (int i = 0; i < mPlayers.size(); ++i)
            if (al::getSensorHost(mPlayers.at(i)) == al::getSensorHost(other)) return;
        sead::Vector3f side, front, up;
        al::calcSideDir(&side, this);
        al::calcFrontDir(&front, this);
        al::calcUpDir(&up, this);
        sead::Vector3f delta = al::getTrans(al::getSensorHost(other)) - al::getTrans(this);
        float height = 35.0f;
        if (mIsSingleMode) height = mConnector ? 45.0f : 35.0f;
        if (sead::Mathf::abs(delta.dot(front)) > width) return;
        if (sead::Mathf::abs(delta.dot(side)) > width) return;
        if (sead::Mathf::abs(delta.dot(up)) > height) return;
        if (!mPlayers.isFull()) mPlayers.pushBack(other);
        return;
    }
    if (al::isNerve(this, &NrvJumpPanelJump) && al::isLessEqualStep(this, 3)) {
        if (al::isSensorEnemyBody(other) || al::isSensorMapObj(other) || al::isSensorNpc(other) ||
            al::isSensorKoopaJr(other) || al::isSensorRide(other) || (al::isSensorKickKoura(other) && mIsSingleMode)) {
            sead::Vector3f side, front, up;
            al::calcSideDir(&side, this);
            al::calcFrontDir(&front, this);
            al::calcUpDir(&up, this);
            sead::Vector3f delta = al::getTrans(al::getSensorHost(other)) - al::getTrans(this);
            if (sead::Mathf::abs(delta.dot(front)) > width || sead::Mathf::abs(delta.dot(side)) > width) return;
            if (rc::sendMsgJumpPanelAction(other, self, mIsSuper) && al::isSensorKoopaJr(other)) al::setVelocityY(al::getSensorHost(other), mJumpPower);
        }
        return;
    }
    if (al::isNerve(this, &NrvJumpPanelJump) && al::isLessEqualStep(this, 36)) {
        sead::Vector3f side, front, up;
        al::calcSideDir(&side, this);
        al::calcFrontDir(&front, this);
        al::calcUpDir(&up, this);
        sead::Vector3f delta = al::getTrans(al::getSensorHost(other)) - al::getTrans(this);
        if (sead::Mathf::abs(delta.dot(front)) > width || sead::Mathf::abs(delta.dot(side)) > width || sead::Mathf::abs(delta.dot(up)) > 240.0f) return;
        al::sendMsgPush(other, self);
    }
}

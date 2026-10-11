#include "MapObj/BoxKuribo.hpp"
#include "MapObj/HeadgearPoseBuilder.hpp"
#include "MapObj/HeadgearStateBlow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Joint/JointRumbler.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
#include <prim/seadDelegate.h>
namespace {
NERVE_DECL(BoxKuribo, WaitItem);
NERVE_DECL(BoxKuribo, Blow);
NERVE_DECL(BoxKuribo, PopUp);
NERVE_DECL(BoxKuribo, Attach);
NERVE_DECL(BoxKuribo, Wait);
class BoxKuriboNrvRaidon : public al::Nerve {
    void execute(al::NerveKeeper* keeper) const override { keeper->getParent<BoxKuribo>()->exeWait(); }
};
NERVES_MAKE_STRUCT(BoxKuribo, WaitItem, Blow, PopUp, Attach, Raidon, Wait)
const sead::Vector3f popUpVelocity(0.0f, 20.0f, 0.0f);
bool tryEquip(al::HitSensor* player, al::HitSensor* headgear, sead::IDelegate* delegate) {
    return !rc::isPlayerManekinekoStatueOn(player) && rc::tryPlayerEquipHeadgear(player, headgear, delegate, 2);
}
}
BoxKuribo::BoxKuribo(const char* name) : al::LiveActor(name) {}
BoxKuribo::~BoxKuribo() {}
void BoxKuribo::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "BoxKuribo", nullptr);
    al::initNerve(this, &NrvBoxKuribo.WaitItem, 1);
    mColliderRadius = al::getColliderRadius(this);
    al::hideSilhouetteModel(this);
    al::offCollide(this);
    mPoseBuilder = new HeadgearPoseBuilder(this, nullptr);
    mPoseDelegate = new sead::Delegate<HeadgearPoseBuilder>(mPoseBuilder, &HeadgearPoseBuilder::updateAndCalcAnimDirect);
    mBlowState = new HeadgearStateBlow(this, nullptr);
    al::initNerveState(this, mBlowState, &NrvBoxKuribo.Blow, "吹き飛び");
    al::initJointControllerKeeper(this, 1);
    mRumbler = al::initJointRumbler(this, "BoxKuribo", 2.2f, 0.4f, 30, 0);
    mRumbler->initDetails(al::JointRumbler::EAxis_X, 0, 0.2f);
    mRumbler->initDetails(al::JointRumbler::EAxis_Y, 3, 0.75f);
    mRumbler->initDetails(al::JointRumbler::EAxis_Z, 6, 0.05f);
    al::tryGetArg(&mLargeCollider, info, "IsColliderSizeLarge");
    if (al::trySyncStageSwitchAppear(this)) al::setNerve(this, &NrvBoxKuribo.PopUp);
    else if (al::isPlaced(info)) makeActorAppeared();
    else makeActorDead();
}
void BoxKuribo::appearPopUp(const sead::Vector3f& pos) {
    appear();
    al::setNerve(this, &NrvBoxKuribo.PopUp);
    al::resetPosition(this, pos, false);
}
bool BoxKuribo::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorPlayerOrPlayerWeapon(sender)) return false;
    if (mPoseBuilder->getPlayerSensor()) {
        if (mBlowState->tryStart(msg, mPoseBuilder->getPlayerSensor())) {
            mPoseBuilder->clearPlayerSensor();
            al::setNerve(this, &NrvBoxKuribo.Blow);
            return true;
        }
        if (mPoseBuilder->getPlayerSensor() && rc::isPlayerBubble(al::getSensorHost(mPoseBuilder->getPlayerSensor()))) return false;
    }
    if (al::isMsgItemGetDirectAll(msg) && !(al::isNerve(this, &NrvBoxKuribo.PopUp) && al::isLessEqualStep(this, 15)) &&
        (al::isNerve(this, &NrvBoxKuribo.PopUp) || al::isNerve(this, &NrvBoxKuribo.WaitItem))) {
        if (!tryEquip(sender, receiver, mPoseDelegate)) return true;
        if (!al::isNoCollide(this)) al::offCollide(this);
        al::setVelocityZero(this);
        al::showSilhouetteModel(this);
        al::invalidateClipping(this);
        mPoseBuilder->start(sender);
        al::setNerve(this, &NrvBoxKuribo.Attach);
        return false;
    }
    if (al::isNerve(this, &NrvBoxKuribo.Attach) || al::isNerve(this, &NrvBoxKuribo.Wait) || al::isNerve(this, &NrvBoxKuribo.Raidon)) {
        if (al::isNerve(this, &NrvBoxKuribo.Raidon)) return false;
        if (rc::getPlayerCeilingCheckLevel(mPoseBuilder->getPlayerSensor()) < 3u) return false;
        if (al::isMsgPlayerTrampleForCrossoverSensor(msg, sender, receiver) || al::isMsgPlayerObjHipDropReflectAll(msg)) {
            if (al::getSensorHost(sender) == al::getSensorHost(mPoseBuilder->getPlayerSensor())) return false;
            mRumbler->start();
            return true;
        }
    }
    return false;
}
void BoxKuribo::exePopUp() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::onCollide(this);
        if (mLargeCollider) al::setColliderRadius(this, 130.0f);
        al::setVelocity(this, popUpVelocity);
    }
    al::addVelocityToGravity(this, 0.98f);
    al::scaleVelocity(this, 0.98f);
    if (al::isCollidedGround(this) && al::getVelocity(this).y < 0.0f) {
        al::offCollide(this);
        if (mLargeCollider) al::setColliderRadius(this, mColliderRadius);
        al::setNerve(this, &NrvBoxKuribo.WaitItem);
        al::setVelocityZero(this);
    }
}
void BoxKuribo::exeWaitItem() {
    if (al::isFirstStep(this)) al::startAction(this, "WaitItem");
}
void BoxKuribo::exeAttach() {
    if (al::isFirstStep(this)) al::startAction(this, "Attach");
    mRumbler->update();
    al::HitSensor* player = mPoseBuilder->getPlayerSensor();
    if (rc::isPlayerDamageTrigOn(player) || rc::isPlayerDead(al::getSensorHost(player))) {
        rc::removePlayerEquipHeadgear(player, false);
        return;
    }
    mPoseBuilder->update();
    al::setNerveAtActionEnd(this, &NrvBoxKuribo.Wait);
}
void BoxKuribo::exeWait() {
    if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "Wait");
    mRumbler->update();
    al::HitSensor* player = mPoseBuilder->getPlayerSensor();
    if (rc::isPlayerDamageTrigOn(player) || rc::isPlayerDead(al::getSensorHost(player))) {
        rc::removePlayerEquipHeadgear(player, false);
        return;
    }
    mPoseBuilder->update();
    bool raidon = mPoseBuilder->isPlayerRaidonActionPlaying();
    if (al::isNerve(this, &NrvBoxKuribo.Wait) && raidon) al::setNerve(this, &NrvBoxKuribo.Raidon);
    else if (al::isNerve(this, &NrvBoxKuribo.Raidon) && !raidon) al::setNerve(this, &NrvBoxKuribo.Wait);
}
void BoxKuribo::exeBlow() { if (al::updateNerveState(this)) kill(); }
bool BoxKuribo::hideActor() {
    if (al::isNerve(this, &NrvBoxKuribo.Attach)) return false;
    return al::LiveActor::hideActor();
}

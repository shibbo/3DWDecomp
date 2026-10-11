#include "MapObj/NeedleBar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(NeedleBar, Wait);
    NERVE_DECL(NeedleBar, SupportFreeze);
    NERVE_DECL(NeedleBar, SupportFreezeSync);
    NERVES_MAKE_NOSTRUCT(NeedleBar, SupportFreeze)
    NERVES_MAKE_STRUCT(NeedleBar, Wait, SupportFreezeSync)
}
NeedleBar::NeedleBar(const char* name) : al::LiveActor(name) {}
NeedleBar::~NeedleBar() {}
void NeedleBar::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, "Bar", 0);
    al::initNerve(this, &NrvNeedleBar.Wait, 0);
    mInitialQuat = al::getQuat(this);
    al::calcTransOffsetUp(&mCenter, this, 50.0f);
    mSensorRadius = al::getSensorRadius(this, "Damage");
    float centerSpace = 100.0f;
    al::tryGetArg(&centerSpace, info, "CenterSpace");
    mOrbitRadius = mSensorRadius + centerSpace;
    al::setClippingInfo(this, centerSpace + mSensorRadius * 2.0f, &mCenter);
    makeActorAppeared();
}
void NeedleBar::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isHitCylinderSensor(receiver, sender, mSide, 30.0f)) {
        rc::sendMsgNeedleRollerAttack(receiver, sender);
        al::sendMsgEnemyAttack(receiver, sender);
    }
}
bool NeedleBar::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isMsgPlayerInvincibleAttack(msg) || al::isMsgPlayerInvincibleTouch(msg) ||
         al::isMsgPlayerObjStatueDrop(msg) || al::isMsgPlayerStatueTouch(msg) ||
         al::isMsgPlayerGiantAttack(msg) || al::isMsgLaserAttack(msg) ||
         rc::isMsgBobsledBodyAttack(msg) || al::isMsgBlockUpperPunch(msg)) &&
        al::isHitCylinderSensor(sender, receiver, mSide, 30.0f)) {
        rc::requestHitReactionToAttacker(msg, receiver, sender);
        al::startHitReactionBreak(this);
        rc::addScoreCombo(this, sender, msg, 0.0f);
        al::setAppearItemAttackerSensor(this, sender);
        al::appearItem(this);
        kill();
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(msg) && al::isHitCylinderSensor(sender, receiver, mSide, 30.0f)) {
        rc::requestHitReactionToAttacker(msg, receiver, sender);
        return true;
    }
    if (al::isMsgKickKouraBreak(msg) || al::isMsgPlayerBoomerangBreak(msg)) {
        if (al::isSensorCollision(receiver) || al::isHitCylinderSensor(sender, receiver, mSide, 30.0f)) {
            rc::requestHitReactionToAttacker(msg, receiver, sender);
            return true;
        }
        return false;
    }
    return false;
}
bool NeedleBar::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssist(msg)) {
        mFreezeTime = 60;
        if (!al::isNerve(this, &NrvNeedleBarSupportFreeze)) al::setNerve(this, &NrvNeedleBarSupportFreeze);
        return true;
    }
    return false;
}
void NeedleBar::setRotateY(float angle, bool reset) {
    float previous = mRotateY;
    mRotateY = angle;
    if (reset) {
        mRotateX = 0.0f;
        return;
    }
    float rotation = (mOrbitRadius / -50.0f) * al::diffNearAngleDegree(previous, angle);
    mRotateX = al::wrapAngle(mRotateX + rotation);
}
void NeedleBar::control() {
    al::rotateQuatYDirDegree(this, mInitialQuat, mRotateY);
    al::rotateQuatXDirDegree(this, mRotateX);
    al::setTransOffsetLocalDir(this, al::getQuat(this), mCenter, mOrbitRadius, 0);
    al::calcSideDir(&mSide, this);
}
bool NeedleBar::isNerveSupportFreeze() const { return al::isNerve(this, &NrvNeedleBarSupportFreeze); }
bool NeedleBar::isStop() const { return !al::isNerve(this, &NrvNeedleBar.Wait); }
void NeedleBar::onSyncSupportFreeze() {
    if (al::isNerve(this, &NrvNeedleBar.Wait)) al::setNerve(this, &NrvNeedleBar.SupportFreezeSync);
}
void NeedleBar::offSyncSupportFreeze() {
    if (al::isNerve(this, &NrvNeedleBar.SupportFreezeSync)) al::setNerve(this, &NrvNeedleBar.Wait);
}
void NeedleBar::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
void NeedleBar::exeSupportFreeze() {
    if (al::isFirstStep(this)) al::startAction(this, "SupportFreeze");
    if (--mFreezeTime <= 0) al::setNerve(this, &NrvNeedleBar.Wait);
}
void NeedleBar::exeSupportFreezeSync() {
    if (al::isFirstStep(this)) al::startAction(this, "SupportFreeze");
}

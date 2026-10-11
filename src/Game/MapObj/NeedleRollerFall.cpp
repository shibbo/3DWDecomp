#include "MapObj/NeedleRollerFall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
namespace {
NERVE_DECL(NeedleRollerFall, Wait);
NERVE_DECL(NeedleRollerFall, Generate);
NERVE_DECL(NeedleRollerFall, SupportFreeze);
NERVES_MAKE_STRUCT(NeedleRollerFall, Wait, Generate, SupportFreeze)
}
NeedleRollerFall::NeedleRollerFall(const char* name) : al::LiveActor(name) {}
NeedleRollerFall::~NeedleRollerFall() {}
void NeedleRollerFall::kill() { al::LiveActor::kill(); }
void NeedleRollerFall::init(const al::ActorInitInfo& info) {
    if (mArchiveName) al::initActorWithArchiveName(this, info, mArchiveName, "Fall");
    else al::initMapPartsActor(this, info, "Fall", 0);
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalXRotator(this, &mRollAngle, "NeedleRoller");
    al::initNerve(this, &NrvNeedleRollerFall.Wait, 0);
    al::tryGetArg(&mMoveAccel, info, "MoveAccel");
    mBaseQuat.set(al::getQuat(this));
    mBaseTrans.set(al::getTrans(this));
    al::calcQuatSide(&mAxis, mBaseQuat);
    al::calcQuatFront(&mMoveDir, mBaseQuat);
    al::invalidateClipping(this);
    al::startAction(this, "Wait");
    makeActorDead();
}
void NeedleRollerFall::setArchiveName(const char* name) { mArchiveName = name; }
void NeedleRollerFall::setBasePose(const sead::Quatf& quat, const sead::Vector3f& trans) {
    mBaseQuat.set(quat);
    mBaseTrans.set(trans);
    al::calcQuatSide(&mAxis, mBaseQuat);
    al::calcQuatFront(&mMoveDir, mBaseQuat);
    al::setTrans(this, mBaseTrans);
    al::setQuat(this, mBaseQuat);
}
void NeedleRollerFall::setMoveAccel(float accel) { mMoveAccel = accel; }
bool NeedleRollerFall::isEnableAttack() const {
    if (al::isNerve(this, &NrvNeedleRollerFall.Generate)) return al::isGreaterEqualStep(this, 35);
    return true;
}
void NeedleRollerFall::control() {
    mPreviousTrans.set(al::getTrans(this));
    if (isEnableAttack() && rc::sendMsgNeedleRollerHitToCollition(this, al::getHitSensor(this, 0), mAxis, 30.0f)) {
        al::startHitReactionBreak(this);
        kill();
    }
}
void NeedleRollerFall::startGenerate() {
    mRollSpeed = 3.0f;
    al::setVelocityZero(this);
    mPreviousTrans.set(mBaseTrans);
    al::setTrans(this, mBaseTrans);
    al::setQuat(this, mBaseQuat);
    al::setNerve(this, &NrvNeedleRollerFall.Generate);
    al::startAction(this, "Generate");
    makeActorAppeared();
}
void NeedleRollerFall::startMove() {
    if (al::isNerve(this, &NrvNeedleRollerFall.Generate)) al::setNerve(this, &NrvNeedleRollerFall.Wait);
}
void NeedleRollerFall::start() {
    mRollSpeed = 3.0f;
    al::setVelocityZero(this);
    mPreviousTrans.set(mBaseTrans);
    al::setTrans(this, mBaseTrans);
    al::setQuat(this, mBaseQuat);
    al::setNerve(this, &NrvNeedleRollerFall.Wait);
    makeActorAppeared();
}
void NeedleRollerFall::exeGenerate() {}
void NeedleRollerFall::exeWait() {
    if (!al::isNearZero(mMoveAccel, 0.001f) && al::isCollidedGround(this)) {
        sead::Vector3f dir = mMoveDir;
        al::turnDirectionAlongGround(this, &dir);
        al::addVelocity(this, mMoveAccel * dir);
    }
    bool onGround = al::isOnGround(this, 6, 0.0f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    al::addVelocityToGravity(this, 0.5f);
    al::scaleVelocity(this, onGround ? 0.95f : 0.99f);
    sead::Vector3f delta = al::getTrans(this) - mPreviousTrans;
    if (onGround) {
        al::verticalizeVec(&delta, al::getOnGroundNormal(this, 6), delta);
        float roll = delta.length() / 78.53981781f;
        if (delta.z * mAxis.x - mAxis.z * delta.x <= 0.0f) roll = -roll;
        mRollSpeed = sead::Mathf::rad2deg(roll);
        mRollAngle += mRollSpeed;
        float speed = sead::Mathf::abs(roll);
        float volume = speed * 6.0f;
        if (volume > 0.3f) {
            float tempo = speed * 3.0f + 0.2f;
            al::holdSeSetVolumeTempoByName(this, "プログラム呼び出し", volume, tempo);
        }
        al::tryStartActionIfNotPlaying(this, "Ground");
        if (!mWasOnGround) al::startHitReaction(this, "接地");
        mWasOnGround = true;
        if (al::isCollidedGround(this)) al::sendMsgEnemyFloorTouchToColliderGround(this, al::getHitSensor(this, "Damage"));
    } else {
        al::tryStartActionIfNotPlaying(this, "Wait");
        mRollAngle += mRollSpeed;
        mRollSpeed *= 0.98f;
        mWasOnGround = false;
    }
    if (rc::isInAreaObj(this, rc::AreaObjType::NeedleRollerBreakArea)) {
        al::startHitReactionBreak(this);
        kill();
    } else if (rc::isInDeathArea(this)) disappear();
}
void NeedleRollerFall::disappear() {
    if (al::isAlive(this)) { al::startHitReactionDisappear(this); makeActorDead(); }
}
void NeedleRollerFall::exeSupportFreeze() {
    if (al::isFirstStep(this)) al::startAction(this, "SupportFreeze");
    if (--mFreezeTimer <= 0) al::setNerve(this, &NrvNeedleRollerFall.Wait);
}
void NeedleRollerFall::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!isEnableAttack()) return;
    if (rc::sendMsgNeedleRollerHit(receiver, sender, mAxis, 30.0f)) { al::startHitReactionBreak(this); kill(); return; }
    if (al::isHitCylinderSensor(receiver, sender, mAxis, 30.0f)) {
        rc::sendMsgNeedleRollerAttack(receiver, sender);
        al::sendMsgEnemyAttack(receiver, sender);
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && al::isSensorKickKoura(receiver)) al::sendMsgKouraDestroy(receiver, sender);
    }
}
bool NeedleRollerFall::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isMsgPlayerInvincibleAttack(msg) || al::isMsgPlayerInvincibleTouch(msg) || al::isMsgPlayerObjStatueDrop(msg) || al::isMsgPlayerStatueTouch(msg) || al::isMsgPlayerGiantAttack(msg) || al::isMsgLaserAttack(msg) || rc::isMsgBobsledBodyAttack(msg) || al::isMsgBlockUpperPunch(msg)) && al::isHitCylinderSensor(sender, receiver, mAxis, 30.0f)) {
        rc::requestHitReactionToAttacker(msg, receiver, sender);
        al::startHitReactionBreak(this);
        rc::addScoreCombo(this, sender, msg, 0.0f);
        al::setAppearItemAttackerSensor(this, sender);
        al::appearItem(this);
        kill();
        return true;
    }
    if ((al::isMsgPlayerFireBallAttack(msg) && al::isHitCylinderSensor(sender, receiver, mAxis, 30.0f)) ||
        ((al::isMsgKickKouraBreak(msg) || al::isMsgPlayerBoomerangBreak(msg)) && (al::isSensorCollision(receiver) || al::isHitCylinderSensor(sender, receiver, mAxis, 30.0f)))) {
        rc::requestHitReactionToAttacker(msg, receiver, sender);
        return true;
    }
    if (isEnableAttack()) {
        sead::Vector3f axis;
        float radius;
        if (rc::tryGetNeedleRollerHitParam(&axis, &radius, msg)) {
            if (al::isSensorCollision(receiver)) {
                al::startHitReactionBreak(this);
                kill();
                return true;
            }
            sead::Vector3f center = al::getSensorPos(sender);
            axis *= al::getSensorRadius(sender);
            sead::Vector3f senderA = center + axis;
            sead::Vector3f senderB = center - axis;
            sead::Vector3f extent = al::getSensorRadius(receiver) * mAxis;
            sead::Vector3f receiverA = extent + al::getTrans(this);
            sead::Vector3f receiverB = al::getTrans(this) - extent;
            sead::Vector3f pointA, pointB;
            float distance = radius + 30.0f;
            if (al::calcSquaredDistanceHitSegmentToSegment(senderA, senderB, receiverA, receiverB, &pointA, &pointB) < distance * distance) {
                al::startHitReactionBreak(this);
                kill();
                return true;
            }
        }
    }
    return false;
}
bool NeedleRollerFall::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (!al::isMsgTouchAssist(msg)) return false;
    mFreezeTimer = 60;
    if (al::isNerve(this, &NrvNeedleRollerFall.Wait)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvNeedleRollerFall.SupportFreeze);
    }
    return true;
}

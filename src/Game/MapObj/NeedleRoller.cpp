#include "MapObj/NeedleRoller.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/MapObj/SupportFreezeSyncGroupHolder.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(NeedleRoller, FreeMove);
NERVE_DECL(NeedleRoller, Wait);
NERVE_DECL(NeedleRoller, SupportFreeze);
NERVE_DECL(NeedleRoller, Move);
NERVES_MAKE_STRUCT(NeedleRoller, FreeMove, Wait)
NERVES_MAKE_NOSTRUCT(NeedleRoller, SupportFreeze, Move)
}
NeedleRoller::NeedleRoller(const char* name) : al::LiveActor(name) {}
NeedleRoller::~NeedleRoller() {}
void NeedleRoller::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    al::registSupportFreezeSyncGroup(this, info);
    mConnector = al::createMtxConnector(this);
    al::calcQuatSide(&mAxis, this);
    al::PlacementInfo endInfo;
    sead::Vector3f delta = sead::Vector3f::ez;
    if (al::tryGetLinksInfo(&endInfo, al::getPlacementInfo(info), "MoveEnd")) {
        sead::Vector3f end;
        al::tryGetTrans(&end, endInfo);
        delta.setSub(end, al::getTrans(this));
        if (al::normalizeOrZero(&mMoveDir, delta)) al::calcQuatFront(&mMoveDir, this);
    }
    float shadow = -1.0f;
    al::tryGetArg(&shadow, info, "ShadowLength");
    if (shadow > 0.0f) al::setShadowDropLength(this, shadow);
    mReverseRoll = mMoveDir.cross(sead::Vector3f::ey).dot(mAxis) >= 0.0f;
    float maxDistance = mMoveDir.dot(delta);
    mMinDistance = 0.0f;
    mMaxDistance = maxDistance;
    if (al::isObjectNameSubStr(info, "FreeMove")) {
        mFreeMove = true;
        al::initNerve(this, &NrvNeedleRoller.FreeMove, 0);
        al::startAction(this, "Wait");
    } else {
        mFreeMove = false;
        al::initNerve(this, &NrvNeedleRoller.Wait, 0);
        mSpeed = 8.0f;
        al::tryGetArg(&mSpeed, info, "MoveSpeed");
    }
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalXRotator(this, &mRollAngle, "NeedleRoller");
    makeActorAppeared();
}
void NeedleRoller::initAfterPlacement() { al::attachMtxConnectorToCollision(mConnector, this, false); }
void NeedleRoller::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isHitCylinderSensor(receiver, sender, mAxis, 30.0f)) al::sendMsgEnemyAttack(receiver, sender);
}
bool NeedleRoller::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
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
    if (al::isMsgIsNerveSupportFreeze(msg)) return false;
    al::isMsgOnSyncSupportFreeze(msg);
    al::isMsgOffSyncSupportFreeze(msg);
    return false;
}
bool NeedleRoller::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (!al::isMsgTouchAssist(msg)) return false;
    mFreezeTimer = 60;
    if (!al::isNerve(this, &NrvNeedleRollerSupportFreeze)) al::setNerve(this, &NrvNeedleRollerSupportFreeze);
    return true;
}
void NeedleRoller::control() {
    mRollAngle = al::modf(sead::Mathf::rad2deg(mDistance / 50.0f) + 360.0f, 360.0f) + 0.0f;
    if (mReverseRoll) mRollAngle = -mRollAngle;
    sead::Vector3f trans;
    trans.setScaleAdd(mDistance, mMoveDir, al::getConnectBaseTrans(mConnector));
    al::connectPoseQT(this, mConnector, al::getConnectBaseQuat(mConnector), trans);
}
void NeedleRoller::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (al::isGreaterEqualStep(this, 10)) al::setNerve(this, &NrvNeedleRollerMove);
}
void NeedleRoller::exeMove() {
    if (al::isFirstStep(this)) al::startAction(this, "Ground");
    mDistance += mSpeed;
    bool reached = false;
    if (mSpeed > 0.0f) {
        if (mDistance > mMaxDistance) { mDistance = mMaxDistance; reached = true; }
    } else if (mDistance < mMinDistance) { mDistance = mMinDistance; reached = true; }
    if (reached) {
        mSpeed = -mSpeed;
        al::setNerve(this, &NrvNeedleRoller.Wait);
        al::startHitReactionEnd(this);
        startSeHit(mSpeed);
    }
    holdSeMove(mSpeed);
}
void NeedleRoller::startSeHit(float speed) {
    float volume = sead::Mathf::abs(speed / 5.0f);
    if (volume > 0.3f) al::startSeSetVolumeByName(this, "StartHit", volume);
}
void NeedleRoller::holdSeMove(float speed) {
    float absSpeed = sead::Mathf::abs(speed);
    float volume = absSpeed / 6.0f;
    if (volume > 0.3f) {
        float tempo = absSpeed / 11.0f + 0.2f;
        al::holdSeSetVolumeTempoByName(this, "プログラム呼び出し", volume, tempo);
    }
}
void NeedleRoller::exeFreeMove() {
    sead::Vector3f dir;
    mConnector->multVec(&dir, mMoveDir);
    float previousSpeed = sead::Mathf::abs(mSpeed);
    if (dir.y > 0.05f) mSpeed -= 0.25f;
    if (dir.y < -0.05f) mSpeed += 0.25f;
    mSpeed *= 0.97f;
    mDistance += mSpeed;
    if (mDistance < mMinDistance) {
        mDistance = mMinDistance;
        if (mSpeed < 0.0f) { mSpeed *= -0.6f; startSeHit(mSpeed); }
    }
    if (mDistance > mMaxDistance) {
        mDistance = mMaxDistance;
        if (mSpeed > 0.0f) { mSpeed *= -0.6f; startSeHit(mSpeed); }
    }
    bool wasStopped = previousSpeed < 3.0f;
    float speed = sead::Mathf::abs(mSpeed);
    if (wasStopped != (speed < 3.0f)) al::startAction(this, speed < 3.0f ? "Wait" : "Ground");
    if (!(speed < 3.0f)) holdSeMove(mSpeed);
}
void NeedleRoller::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        if (mFreeMove) mSpeed = 0.0f;
        al::startAction(this, "SupportFreeze");
    }
    if (--mFreezeTimer <= 0) {
        if (mFreeMove) al::setNerve(this, &NrvNeedleRoller.FreeMove);
        else al::setNerve(this, &NrvNeedleRoller.Wait);
    }
}

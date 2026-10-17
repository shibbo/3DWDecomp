#include "MapObj/ClimbHandle.hpp"
#include "MapObj/ClimbHandleMapParts.hpp"
#include "MapObj/TimerManager.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Util/PlayerUtil.hpp"
#include <math.h>
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
NERVE_DECL(ClimbHandle, Stop)
NERVE_DECL(ClimbHandle, Top)
NERVE_DECL(ClimbHandle, TopFirst)
NERVE_DECL(ClimbHandle, TopRotate)
NERVE_DECL(ClimbHandle, Up)
NERVE_DECL(ClimbHandle, Skid)
NERVE_DECL(ClimbHandle, Down)
NERVE_DECL(ClimbHandle, Keep)
NERVE_DECL(ClimbHandle, StopAtTop)
NERVE_DECL(ClimbHandle, SkidBeforeDown)
NERVE_DECL(ClimbHandle, Bound)
NERVES_MAKE_NOSTRUCT(ClimbHandle, Stop, Top, TopFirst, TopRotate, Up, Skid, Down, Keep, StopAtTop, SkidBeforeDown, Bound)
}
ClimbHandle::ClimbHandle(const char* name) : al::LiveActor(name) {}
ClimbHandle::~ClimbHandle() = default;
void ClimbHandle::init(const al::ActorInitInfo& info) {
    mIsSingleMode = info.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) al::initActorSuffix(this, info, "SM");
    else al::initActor(this, info);
    al::initNerve(this, &NrvClimbHandleStop, 0);
    al::tryGetArg(&mCancelDistance, info, "CancelDistance");
    if (!al::tryGetArg(&mWaitTime, info, "WaitTime")) makeActorDead();
    if (!al::tryGetArg(&mIsStop, info, "IsStop")) mIsStop = false;
    if (!al::calcLinkChildNum(info, "MapPartsStart")) { makeActorDead(); return; }
    al::PlacementInfo startInfo, endInfo;
    al::tryGetLinksInfo(&startInfo, info, "MapPartsStart");
    if (al::tryGetLinksInfo(&endInfo, startInfo, "MoveNext")) {
        mMapParts = new ClimbHandleMapParts("ClimbHandleMapParts");
        al::initLinksActor(mMapParts, info, "MapPartsStart", 0);
        mBottom.set(al::getTrans(mMapParts));
        if (al::tryGetTrans(&mTop, endInfo)) {
            mClippingCenter.set(al::getTrans(this));
            float height = mTop.y - mBottom.y;
            if (height <= 0.0f) makeActorDead();
            al::setClippingInfo(this, height, &mClippingCenter);
            al::initJointControllerKeeper(this, 1);
            al::initJointLocalZRotator(this, &mRotation, "Handle");
            mMoveStart.set(mBottom);
            mMoveEnd.set(mBottom);
            mMoveEnd.y += 300.0f;
            makeActorAppeared();
            return;
        }
    }
    makeActorDead();
}
bool ClimbHandle::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (!(al::isMsgPlayerClimbAttack(msg) || al::isMsgPlayerBodyAttack(msg) || al::isMsgPlayerClimbRollingAttack(msg) || al::isMsgPlayerClimbSlidingAttack(msg))) return false;
    if (al::isNerve(this, &NrvClimbHandleTop) || al::isNerve(this, &NrvClimbHandleTopFirst)) {
        al::setNerve(this, &NrvClimbHandleTopRotate); return true;
    }
    if (al::isNerve(this, &NrvClimbHandleUp)) return false;
    if (al::isNerve(this, &NrvClimbHandleTopRotate)) return false;
    if (al::isNerve(this, &NrvClimbHandleSkid)) return false;
    if (al::isNerve(this, &NrvClimbHandleStop)) al::setNerve(this, &NrvClimbHandleSkid);
    else al::setNerve(this, &NrvClimbHandleUp);
    return true;
}
void ClimbHandle::exeStop() { if (al::isFirstStep(this)) mMapParts->setNerveToStop(); }
void ClimbHandle::exeSkid() {
    mRotation += 20.0f;
    if (al::isGreaterStep(this, 4)) { al::startHitReactionStart(this); al::setNerve(this, &NrvClimbHandleUp); }
}
void ClimbHandle::exeSkidBeforeDown() {
    if (al::isFirstStep(this)) mFallSpeed = 0.0f;
    mFallSpeed = (mFallSpeed - 0.6f) * 0.99f;
    mRotationSpeed = mFallSpeed * 1.1f;
    mRotation += mRotationSpeed;
    if (al::isGreaterStep(this, 4)) al::setNerve(this, &NrvClimbHandleDown);
}
void ClimbHandle::exeUp() {
    if (al::isFirstStep(this)) {
        mMoveStart.set(al::getTrans(mMapParts)); mMoveEnd.set(mMoveStart); mMoveEnd.y += 300.0f;
        al::startHitReaction(this, "上昇");
        al::tryStartSe(mMapParts, "MoveStart", nullptr);
        mMapParts->setNerveToUp();
    }
    float t = al::getNerveStep(this) / -35.0f + 1.0f;
    float rate = 1.0f - t * (t * t);
    float oldY = al::getTrans(mMapParts).y;
    sead::Vector3f trans;
    al::lerpVec(&trans, mMoveStart, mMoveEnd, rate);
    bool top = false, end = false;
    if (trans.y >= mTop.y) { trans.y = mTop.y; top = true; }
    else if (al::isGreaterEqualStep(this, 35)) { trans.y = mMoveEnd.y; end = true; }
    al::setTrans(mMapParts, trans);
    float rotationSpeed = (trans.y - oldY) * 1.1f;
    mRotation += rotationSpeed;
    al::holdSeWithParam(this, "RotationUp", rotationSpeed, nullptr);
    al::tryHoldSeWithParam(mMapParts, "Move", rotationSpeed, nullptr);
    if (top) {
        al::startHitReaction(this, "頂上到着");
        al::tryStartSe(mMapParts, "Top", nullptr);
        al::tryOnStageSwitch(this, "SwitchTopOn");
        al::setNerve(this, &NrvClimbHandleTop);
    } else if (end) al::setNerve(this, &NrvClimbHandleKeep);
}
void ClimbHandle::exeKeep() {
    if (al::isFirstStep(this)) mMapParts->setNerveToStop();
    if (al::isGreaterEqualStep(this, 180)) {
        if (mIsStop) al::setNerve(this, &NrvClimbHandleStopAtTop);
        else al::setNerve(this, &NrvClimbHandleSkidBeforeDown);
    }
}
void ClimbHandle::exeDown() {
    if (al::isFirstStep(this)) {
        mFallSpeed = 0.0f;
        mMapParts->setNerveToDown();
        float y = al::getTrans(mMapParts).y;
        float speed = 0.0f;
        int steps = 0;
        if (mBottom.y < y) { do { speed = (speed - 0.6f) * 0.99f; y += speed; ++steps; } while (y > mBottom.y); }
        float time = steps;
        mRotationAccel = -2.0f * (mRotation - sead::Mathf::abs(mRotationSpeed) * time) / time / time;
    }
    mFallSpeed = (mFallSpeed - 0.6f) * 0.99f;
    sead::Vector3f trans = al::getTrans(mMapParts);
    trans.y = mFallSpeed + trans.y;
    bool bottom = false;
    if (trans.y <= mBottom.y) { trans.y = mBottom.y; bottom = true; }
    mRotationSpeed = mRotationAccel + mRotationSpeed;
    mRotation = mRotationSpeed + mRotation;
    if (mRotation < 0.0f) mRotation = 0.0f;
    al::setTrans(mMapParts, trans);
    al::holdSeWithParam(this, "RotationDown", mFallSpeed, nullptr);
    al::tryHoldSeWithParam(mMapParts, "Move", mRotationSpeed, nullptr);
    if (bottom) {
        al::startHitReaction(this, "落下");
        al::startHitReactionOnGround(mMapParts);
        al::tryStartSe(mMapParts, "Bound", nullptr);
        mMapParts->setNerveToStop();
        al::setNerve(this, &NrvClimbHandleBound);
    }
}
void ClimbHandle::exeBound() {
    if (al::isFirstStep(this)) {
        float speed = mFallSpeed * 0.4f;
        mFallSpeed = speed < 0.0f ? -speed : speed;
        mMapParts->setNerveToStop();
    }
    al::holdSeWithParam(this, "RotationDown", mFallSpeed, nullptr);
    mFallSpeed = (mFallSpeed - 0.6f) * 0.99f;
    sead::Vector3f trans = al::getTrans(mMapParts);
    trans.y = mFallSpeed + trans.y;
    bool bottom = false;
    if (trans.y <= mBottom.y) { trans.y = mBottom.y; bottom = true; }
    mRotation += (trans.y - al::getTrans(mMapParts).y) * 1.1f;
    al::setTrans(mMapParts, trans);
    if (bottom) {
        if (al::isNearZero(mFallSpeed, 1.0f)) al::setNerve(this, &NrvClimbHandleStop);
        else mFallSpeed *= -0.4f;
    }
}
void ClimbHandle::exeTop() {
    if (al::isFirstStep(this)) mMapParts->setNerveToStop();
    if (!mIsStop && al::isLessStep(this, mWaitTime)) {
        int interval = al::isGreaterEqualStep(this, mWaitTime - 120) ? 15 : 30;
        if ((120 - al::getNerveStep(this)) % interval == 0) {
            if (!TimerManager::isTimerManagerActive(this)) {
                if (mTimerHigh) al::startSe(this, "TimerCountH", nullptr);
                else al::startSe(this, "TimerCountL", nullptr);
            }
            mTimerHigh = !mTimerHigh;
        }
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) {
        if (mIsStop) al::setNerve(this, &NrvClimbHandleStopAtTop);
        else {
            if (!TimerManager::isTimerManagerActive(this)) al::startSe(this, "PgTimeUp", nullptr);
            al::setNerve(this, &NrvClimbHandleSkidBeforeDown);
        }
    } else if (mCancelDistance > 0.0f && !rc::tryFindNearestActivePlayerActorInSphere(this, mCancelDistance)) {
        al::tryStopSe(this, "TimerCountH");
        al::tryStopSe(this, "TimerCountL");
        al::setNerve(this, &NrvClimbHandleSkidBeforeDown);
    }
}
void ClimbHandle::exeTopFirst() {
    if (al::isFirstStep(this)) mMapParts->setNerveToStop();
    if (al::isGreaterEqualStep(this, 30)) { al::setTrans(mMapParts, mTop); al::setNerve(this, &NrvClimbHandleTop); return; }
    if (al::isLessEqualStep(this, 10)) {
        float t = al::getNerveStep(this) / 10.0f;
        float sine = sinf(((t + t) * 3.1415927410125732f) * 0.5f);
        float rate = 1.0f - t;
        float offset = rate * (sine * rate);
        al::setTrans(mMapParts, mTop.x, mTop.y + offset, mTop.z);
        mRotation = offset * 1.1f + mRotation;
    }
}
void ClimbHandle::exeTopRotate() {
    if (al::isFirstStep(this)) { mTopRotation = mRotation; mMapParts->setNerveToStop(); }
    if (al::isGreaterEqualStep(this, 28)) { al::setNerve(this, &NrvClimbHandleTop); return; }
    if (al::isLessEqualStep(this, 8)) {
        float t = al::getNerveStep(this) / 8.0f;
        float sine = sinf(((t + t) * 3.1415927410125732f) * 0.5f);
        float rate = 1.0f - t;
        float offset = rate * (sine * rate);
        mRotation = mTopRotation + 25.0f * offset;
    }
}
void ClimbHandle::exeStopAtTop() { if (al::isFirstStep(this)) mMapParts->setNerveToStop(); }

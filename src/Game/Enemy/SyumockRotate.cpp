#include "Enemy/SyumockRotate.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/JointAimUtil.hpp"
#include "Enemy/SyumockUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Joint/JointSpringControllerHolder.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(SyumockRotate, Reaction)
NERVES_MAKE_NOSTRUCT(SyumockRotate, Reaction)
NERVE_DECL(SyumockRotate, SupportFreeze)
NERVE_DECL(SyumockRotate, Rotate)
NERVES_MAKE_STRUCT(SyumockRotate, SupportFreeze, Rotate)
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                sead::Vector3f(0.0f, 100.0f, 0.0f));
}

/** @brief Creates a rotating Syumock. @param pName Actor name. */
SyumockRotate::SyumockRotate(const char* pName) : al::LiveActor(pName) {}

/** @brief Attacks contacts inside the rotating cylinder.
 * @param pSelf Attack sensor. @param pOther Contacted sensor. */
void SyumockRotate::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf) ||
        !al::isHitCylinderSensor(pOther, pSelf, mSide, 30.0f))
        return;
    if (mActorSceneInfo->isSingleMode && al::isSensorRide(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }
    if (al::sendMsgEnemyAttack(pOther, pSelf) && !al::isNerve(this, &NrvSyumockRotateReaction)) {
        mReactionStep = 0;
        al::setNerve(this, &NrvSyumockRotateReaction);
    }
}

/** @brief Handles lethal attacks and reactions within the cylinder.
 * @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Receiver sensor.
 * @return Whether the message was handled. */
bool SyumockRotate::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (SyumockUtil::isMsgSyumockDeath(pMsg) &&
        al::isHitCylinderSensor(pOther, pSelf, mSide, 30.0f)) {
        SyumockUtil::doKill(this, pMsg, pOther, pSelf);
        return true;
    }
    if (!SyumockUtil::isMsgSyumockReaction(pMsg, mReactionStep) ||
        !al::isHitCylinderSensor(pOther, pSelf, mSide, 30.0f))
        return false;
    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    al::setNerve(this, &NrvSyumockRotateReaction);
    mReactionStep = 0;
    return true;
}

/** @brief Starts support freezing when a screen interaction is accepted.
 * @param pMsg Incoming message. @param pPointer Screen pointer. @param pTarget Screen target.
 * @return Whether the interaction was accepted. */
bool SyumockRotate::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (!mSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget))
        return false;
    if (!al::isNerve(this, &NrvSyumockRotate.SupportFreeze))
        al::setNerve(this, &NrvSyumockRotate.SupportFreeze);
    return true;
}

/** @brief Updates the collision axis and reaction cooldown. */
void SyumockRotate::control() {
    al::calcQuatSide(&mSide, this);
    if (mReactionStep < 600)
        ++mReactionStep;
}

/** @brief Initializes rotation, eye joints and support freezing. @param rInfo Placement information. */
void SyumockRotate::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvSyumockRotate.Rotate, 1);
    mSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mSupportFreeze, &NrvSyumockRotate.SupportFreeze, "[state]DRC\u62d8\u675f");
    mBaseQuat.set(al::getQuat(this));
    mCenter.set(al::getTrans(this));
    al::tryGetLinksTrans(&mCenter, rInfo, "RotateCenter");
    mOffset = al::getTrans(this) - mCenter;
    s32 axis = 1;
    al::tryGetArg(&axis, rInfo, "RotateAxis");
    al::calcQuatLocalAxis(&mRotateAxis, this, axis);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    sead::Vector3f cross;
    cross.setCross(mOffset, front);
    if (mRotateAxis.dot(cross) < 0.0f)
        mRotateAxis = -mRotateAxis;
    al::tryGetArg(&mAngle, rInfo, "RotateStartAngle");
    al::tryGetArg(&mSpeed, rInfo, "RotateSpeed");
    bool drop = false;
    al::tryGetArg(&drop, rInfo, "IsDropShadowActorDown");
    if (drop)
        al::setShadowDropDirActorDown(this);
    bool light = true;
    al::tryGetArg(&light, rInfo, "IsUseLight");
    if (!light)
        al::killPrePassLightAll(this, -1);
    mEyeInfo = SyumockUtil::createSyumockEyeJointInfo(this);
    al::initJointControllerKeeper(this, 9);
    al::initJointAimController(this, mEyeInfo, "EyeLeft");
    al::initJointAimController(this, mEyeInfo, "EyeRight");
    mJointSpring = SyumockUtil::initJointSyumockJointSpringController(this);
    makeActorAppeared();
}

/** @brief Rotates with the idle animation and restores joint motion. */
void SyumockRotate::exeRotate() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    mJointSpring->addControlRateAll(0.1f);
    updateMove();
}

/** @brief Applies rotation around the placement center and updates eye tracking. */
void SyumockRotate::updateMove() {
    mAngle = al::wrapAngle(mAngle + mSpeed);
    sead::Quatf rotation;
    rotation.setAxisAngle(mRotateAxis, mAngle);
    al::getQuatPtr(this)->setMul(rotation, mBaseQuat);
    al::getTransPtr(this)->setRotated(rotation, mOffset);
    *al::getTransPtr(this) += mCenter;
    JointAimUtil::updateEyeJointInfo(this, mEyeInfo, 1500.0f, 0.2f);
}

/** @brief Plays the hit reaction while continuing rotation. */
void SyumockRotate::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "Reaction");
    mJointSpring->subControlRateAll(0.1f);
    updateMove();
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvSyumockRotate.Rotate);
}

/** @brief Updates support freezing while suppressing joint motion. */
void SyumockRotate::exeSupportFreeze() {
    mJointSpring->subControlRateAll(0.1f);
    updateMove();
    if (al::updateNerveState(this))
        al::setNerve(this, &NrvSyumockRotate.Rotate);
}

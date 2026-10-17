#include "Enemy/SyumockConveyer.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/JointAimUtil.hpp"
#include "Enemy/SyumockUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Project/LiveActor/ConveyerKeyKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Joint/JointSpringControllerHolder.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(SyumockConveyer, Move)
NERVE_DECL(SyumockConveyer, SupportFreeze)
NERVE_DECL(SyumockConveyer, Reaction)
NERVE_DECL(SyumockConveyer, End)
NERVES_MAKE_STRUCT(SyumockConveyer, Move, SupportFreeze, Reaction, End)
ActorStateSupportFreezeParam sConveyerFreezeParam(true, 15, false, true, 120,
                                                sead::Vector3f(0.0f, 100.0f, 0.0f));
}

/** @brief Creates a conveyor Syumock. @param pName Actor name. */
SyumockConveyer::SyumockConveyer(const char* pName) : al::LiveActor(pName) {}

/** @brief Reflects items and attacks contacts inside the cylinder.
 * @param pSelf Attack sensor. @param pOther Contacted sensor. */
void SyumockConveyer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorEnemyAttack(pSelf))
        return;
    sead::Vector3f side = sead::Vector3f::ex;
    al::calcQuatSide(&side, this);
    sead::Vector3f reflect;
    if (!al::isHitCylinderSensor(nullptr, &reflect, pOther, pSelf, side, 30.0f))
        return;
    rc::sendMsgItemReflect(pOther, pSelf, reflect);
    if (mActorSceneInfo->isSingleMode && al::isSensorRide(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }
    if (al::sendMsgEnemyAttack(pOther, pSelf) && !al::isNerve(this, &NrvSyumockConveyer.Reaction)) {
        mReactionStep = 0;
        al::setNerve(this, &NrvSyumockConveyer.Reaction);
    }
}

/** @brief Handles lethal attacks and reactions within the cylinder.
 * @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Receiver sensor.
 * @return Whether the message was handled. */
bool SyumockConveyer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (SyumockUtil::isMsgSyumockDeath(pMsg) &&
        al::isHitCylinderSensor(pOther, pSelf, mSide, 30.0f)) {
        SyumockUtil::doKill(this, pMsg, pOther, pSelf);
        al::setNerve(this, &NrvSyumockConveyer.End);
        return true;
    }
    if (!SyumockUtil::isMsgSyumockReaction(pMsg, mReactionStep) ||
        !al::isHitCylinderSensor(pOther, pSelf, mSide, 30.0f))
        return false;
    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    al::setNerve(this, &NrvSyumockConveyer.Reaction);
    mReactionStep = 0;
    return true;
}

/** @brief Starts support freezing when a screen interaction is accepted.
 * @param pMsg Incoming message. @param pPointer Screen pointer. @param pTarget Screen target.
 * @return Whether the interaction was accepted. */
bool SyumockConveyer::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (!mSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget))
        return false;
    if (!al::isNerve(this, &NrvSyumockConveyer.End) &&
        !al::isNerve(this, &NrvSyumockConveyer.SupportFreeze))
        al::setNerve(this, &NrvSyumockConveyer.SupportFreeze);
    return true;
}

/** @brief Updates the collision axis and reaction cooldown. */
void SyumockConveyer::control() {
    al::calcQuatSide(&mSide, this);
    if (mReactionStep < 600)
        ++mReactionStep;
}

/** @brief Initializes eye joints, spring motion and support freezing. @param rInfo Placement information. */
void SyumockConveyer::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvSyumockConveyer.Move, 1);
    mSupportFreeze = new ActorStateSupportFreeze(this, &sConveyerFreezeParam);
    al::initNerveState(this, mSupportFreeze, &NrvSyumockConveyer.SupportFreeze, "[state]DRC\u62d8\u675f");
    mEyeInfo = SyumockUtil::createSyumockEyeJointInfo(this);
    al::initJointControllerKeeper(this, 9);
    al::initJointAimController(this, mEyeInfo, "EyeLeft");
    al::initJointAimController(this, mEyeInfo, "EyeRight");
    mJointSpring = SyumockUtil::initJointSyumockJointSpringController(this);
    bool drop = false;
    al::tryGetArg(&drop, rInfo, "IsDropShadowActorDown");
    if (drop)
        al::setShadowDropDirActorDown(this);
    bool light = true;
    al::tryGetArg(&light, rInfo, "IsUseLight");
    if (!light)
        al::killPrePassLightAll(this, -1);
    makeActorAppeared();
}

/** @brief Restores joint motion and tracks nearby players. */
void SyumockConveyer::exeMove() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    mJointSpring->addControlRateAll(0.1f);
    JointAimUtil::updateEyeJointInfo(this, mEyeInfo, 1500.0f, 0.2f);
}

/** @brief Plays the hit reaction and then resumes normal motion. */
void SyumockConveyer::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "Reaction");
    mJointSpring->subControlRateAll(0.2f);
    JointAimUtil::updateEyeJointInfo(this, mEyeInfo, 1500.0f, 0.2f);
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvSyumockConveyer.Move);
}

/** @brief Updates support freezing and suppresses joint motion. */
void SyumockConveyer::exeSupportFreeze() {
    mJointSpring->subControlRateAll(0.2f);
    JointAimUtil::updateEyeJointInfo(this, mEyeInfo, 1500.0f, 0.2f);
    if (al::updateNerveState(this))
        al::setNerve(this, &NrvSyumockConveyer.Move);
}

/** @brief Fades eye tracking after death. */
void SyumockConveyer::exeEnd() {
    mEyeInfo->subPowerRate(0.2f);
}

/** @brief Assigns the conveyor path. @param pKeeper Path keys. @param maxCoord Loop length. */
void SyumockConveyer::setConveyerKeyKeeper(const al::ConveyerKeyKeeper* pKeeper, f32 maxCoord) {
    mKeys = pKeeper;
    mMaxCoord = maxCoord;
}

/** @brief Updates the path pose and handles wrapping and visibility.
 * @param coord Path coordinate. @param forward Whether motion is forward.
 * @param reset Whether to reset the actor position immediately. */
void SyumockConveyer::setTransByCoord(f32 coord, bool forward, bool reset) {
    coord = al::wrapValue(coord, mMaxCoord);
    if (!al::isNerve(this, &NrvSyumockConveyer.End)) {
        mKeys->calcPosAndQuat(al::getTransPtr(this), al::getQuatPtr(this), nullptr, coord);
        sead::Vector3f previous;
        mKeys->calcPosAndQuat(&previous, nullptr, nullptr, coord - 200.0f);
        sead::Vector3f side;
        al::calcSideDir(&side, this);
        sead::Vector3f direction = al::getTrans(this) - previous;
        al::verticalizeVec(&direction, side, direction);
        if (!al::normalizeOrZero(&direction)) {
            auto* pQuat = al::getQuatPtr(this);
            al::turnQuatZDirRate(pQuat, *pQuat, direction, 1.0f);
        }
        if ((forward ? coord < mCoord : coord > mCoord) || reset)
            al::resetPosition(this, false);
    }
    if (coord > mKeys->getTotalMoveDistance()) {
        if (!al::isDead(this))
            makeActorDead();
        al::setNerve(this, &NrvSyumockConveyer.Move);
    } else if (!al::isNerve(this, &NrvSyumockConveyer.End) && al::isDead(this)) {
        makeActorAppeared();
    }
    mCoord = coord;
}

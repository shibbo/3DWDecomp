#include "Enemy/SyumockRailMove.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/JointAimUtil.hpp"
#include "Enemy/SyumockUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Factory/Factory.hpp"
#include "Enemy/EnemyAttachItem.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Joint/JointSpringControllerHolder.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(SyumockRailMove, Reaction)
NERVES_MAKE_NOSTRUCT(SyumockRailMove, Reaction)
NERVE_DECL(SyumockRailMove, SupportFreeze)
NERVE_DECL(SyumockRailMove, Move)
NERVES_MAKE_STRUCT(SyumockRailMove, Move, SupportFreeze)
ActorStateSupportFreezeParam sRailFreezeParam(true, 15, false, true, 120,
                                                sead::Vector3f(0.0f, 100.0f, 0.0f));
}

/** @brief Creates a rail-following Syumock. @param pName Actor name. */
SyumockRailMove::SyumockRailMove(const char* pName) : al::LiveActor(pName) {}

/** @brief Checks the head collision cylinder. @param pOther Contact sensor. @param pSelf Head sensor.
 * @return Whether the contact intersects the head. */
bool SyumockRailMove::isHitHeadCylinder(al::HitSensor* pOther, al::HitSensor* pSelf) const {
    return al::isHitCylinderSensor(pOther, pSelf, mSide, mIsPlacedKinopioBrigade ? 80.0f : 30.0f);
}

/** @brief Checks head contact and computes reflection.
 * @param pReflect Reflection output. @param pOther Contact sensor. @param pSelf Head sensor.
 * @return Whether the contact intersects the head. */
bool SyumockRailMove::isHitHeadCylinder(sead::Vector3f* pReflect, al::HitSensor* pOther,
                                      al::HitSensor* pSelf) const {
    return al::isHitCylinderSensor(nullptr, pReflect, pOther, pSelf, mSide,
                                  mIsPlacedKinopioBrigade ? 80.0f : 30.0f);
}

/** @brief Checks single-player contacts and reflects items.
 * @param pSelf Actor sensor. @param pOther Contact sensor. @return Whether the contact is valid. */
bool SyumockRailMove::isHitSensorsSingleMode(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorName(pSelf, "BodyReact"))
        return true;
    if (al::isSensorEnemyAttack(pSelf)) {
        sead::Vector3f reflect;
        if (isHitHeadCylinder(&reflect, pOther, pSelf)) {
            rc::sendMsgItemReflect(pOther, pSelf, reflect);
            return true;
        }
    }
    return false;
}

/** @brief Attacks or pushes contacts according to the game mode.
 * @param pSelf Actor sensor. @param pOther Contact sensor. */
void SyumockRailMove::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mIsSingleMode) {
        if (!isHitSensorsSingleMode(pSelf, pOther))
            return;
        if (al::isSensorPlessie(pOther)) {
            al::sendMsgPush(pOther, pSelf);
            return;
        }
        if (al::isSensorPlayer(pOther) || al::isSensorRide(pOther)) {
            if (!al::sendMsgEnemyAttack(pOther, pSelf))
                return;
        } else if (al::isSensorKoopaJr(pOther) && al::isSensorName(pOther, "Body")) {
            al::sendMsgPush(pOther, pSelf);
            al::sendMsgHitStrong(pOther, pSelf);
            return;
        } else if (al::isSensorHostName(pOther, "\u30ce\u30b3\u30ce\u30b3 \u901a\u5e38") &&
                   al::isSensorName(pOther, "Body")) {
            al::sendMsgEnemyAttackFire(pOther, pSelf);
            return;
        } else if (al::isSensorHostName(pOther, "BallNeko")) {
            al::sendMsgPush(pOther, pSelf);
            return;
        } else if (!al::isSensorNpc(pOther)) {
            return;
        }
    } else {
        if (!al::isSensorEnemyAttack(pSelf))
            return;
        sead::Vector3f reflect;
        if (!isHitHeadCylinder(&reflect, pOther, pSelf))
            return;
        rc::sendMsgItemReflect(pOther, pSelf, reflect);
        if (!al::sendMsgEnemyAttack(pOther, pSelf))
            return;
    }
    if (!al::isNerve(this, &NrvSyumockRailMoveReaction)) {
        mReactionStep = 0;
        al::setNerve(this, &NrvSyumockRailMoveReaction);
    }
}

/** @brief Handles mode-specific attacks, reactions and pushes.
 * @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Receiver sensor.
 * @return Whether the message was handled. */
bool SyumockRailMove::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (mIsSingleMode) {
        if (!al::isAlive(this))
            return false;
        if (al::isSensorHostName(pOther, "\u96ea\u7389") && al::isMsgBallAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            return true;
        }
        if (SyumockUtil::isMsgSyumockReactionSingleMode(pMsg, mReactionStep)) {
            if (!al::isNerve(this, &NrvSyumockRailMoveReaction)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                al::setNerve(this, &NrvSyumockRailMoveReaction);
            }
            mReactionStep = 0;
            if (al::isMsgKouraThrow(pMsg))
                al::sendMsgPush(pOther, pSelf);
            return true;
        }
        if (SyumockUtil::isMsgSyumockPush(pMsg)) {
            al::sendMsgPush(pOther, pSelf);
            return true;
        }
        if (al::isSensorName(pOther, "Land"))
            return false;
        if (SyumockUtil::isMsgSyumockDeathSingleMode(pMsg)) {
            SyumockUtil::doKill(this, pMsg, pOther, pSelf);
            return true;
        }
        return false;
    }
    if (SyumockUtil::isMsgSyumockDeath(pMsg) && isHitHeadCylinder(pOther, pSelf)) {
        SyumockUtil::doKill(this, pMsg, pOther, pSelf);
        return true;
    }
    if (al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDeath(this);
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        kill();
        return true;
    }
    if (!SyumockUtil::isMsgSyumockReaction(pMsg, mReactionStep) || !isHitHeadCylinder(pOther, pSelf))
        return false;
    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    al::setNerve(this, &NrvSyumockRailMoveReaction);
    mReactionStep = 0;
    return true;
}

/** @brief Starts support freezing when a screen interaction is accepted.
 * @param pMsg Incoming message. @param pPointer Screen pointer. @param pTarget Screen target.
 * @return Whether the interaction was accepted. */
bool SyumockRailMove::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (!mSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget))
        return false;
    if (!al::isNerve(this, &NrvSyumockRailMove.SupportFreeze))
        al::setNerve(this, &NrvSyumockRailMove.SupportFreeze);
    return true;
}

/** @brief Updates the collision axis and reaction cooldown. */
void SyumockRailMove::control() {
    al::calcQuatSide(&mSide, this);
    if (mReactionStep < 600)
        ++mReactionStep;
}

/** @brief Initializes the rail, optional item and joint controllers. @param rInfo Placement information. */
void SyumockRailMove::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.mActorSceneInfo.isSingleMode) {
        const char* pModelName = nullptr;
        alPlacementFunction::tryGetModelName(&pModelName, rInfo);
        if (al::isEqualString(pModelName, "Syumock4M"))
            al::initActorWithArchiveName(this, rInfo, "Syumock4MFur", nullptr);
        else
            al::initMapPartsActor(this, rInfo, nullptr, 0);
    } else {
        al::initMapPartsActor(this, rInfo, nullptr, 0);
    }
    al::tryGetArg(&mSpeed, rInfo, "Speed");
    if (!al::isExistRail(this)) {
        makeActorDead();
        return;
    }
    const char* pItemName = "NoItem";
    al::tryGetStringArg(&pItemName, rInfo, "ItemType");
    if (!al::isEqualString(pItemName, "NoItem")) {
        mItem = new EnemyAttachItem(this,
            "\u30b7\u30e5\u30e2\u30c3\u30af\u30ec\u30fc\u30eb\u79fb\u52d5\u30a2\u30a4\u30c6\u30e0",
            pItemName, rc::getItemType(rInfo), nullptr, &mItemMtx, sead::Vector3f::zero, true);
        al::initCreateActorWithPlacementInfo(mItem, rInfo);
        al::initSubActorKeeperNoFile(this, rInfo, 1);
        al::registerSubActorSyncClipping(this, mItem, false);
        mItem->appear();
    }
    bool drop = false;
    al::tryGetArg(&drop, rInfo, "IsDropShadowActorDown");
    if (drop)
        al::setShadowDropDirActorDown(this);
    bool light = true;
    al::tryGetArg(&light, rInfo, "IsUseLight");
    if (!light)
        al::killPrePassLightAll(this, -1);
    al::tryGetArg(&mIsPlacedKinopioBrigade, rInfo, "IsPlacedKinopioBrigade");
    al::getClippingRadius(this);
    al::setSyncRailToNearestPos(this);
    al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 100.0f);
    if (!al::isRailPlusPoseFront(this))
        al::reverseRail(this);
    mEyeInfo = SyumockUtil::createSyumockEyeJointInfo(this);
    al::initJointControllerKeeper(this, 9);
    al::initJointAimController(this, mEyeInfo, "EyeLeft");
    al::initJointAimController(this, mEyeInfo, "EyeRight");
    mJointSpring = SyumockUtil::initJointSyumockJointSpringController(this);
    al::initNerve(this, &NrvSyumockRailMove.Move, 1);
    mSupportFreeze = new ActorStateSupportFreeze(this, &sRailFreezeParam);
    al::initNerveState(this, mSupportFreeze, &NrvSyumockRailMove.SupportFreeze, "[state]DRC\u62d8\u675f");
    makeActorAppeared();
    mIsSingleMode = rInfo.mActorSceneInfo.isSingleMode;
}

/** @brief Selects the air or water animation and advances along the rail. */
void SyumockRailMove::exeMove() {
    if (rc::isInWaterArea(this))
        al::tryStartActionIfNotPlaying(this, "Wait");
    else
        al::tryStartActionIfNotPlaying(this, "Air");
    mJointSpring->addControlRateAll(0.1f);
    updateMove();
}

/** @brief Updates water effects, rail motion, item position and facing. */
void SyumockRailMove::updateMove() {
    if (rc::isInWaterArea(this)) {
        if (!mInWater)
            al::startHitReaction(this, "\u6c34\u306b\u5165\u3063\u305f");
        al::setMaterialCode(this, "");
        mInWater = true;
    } else {
        if (mInWater)
            al::startHitReaction(this, "\u6c34\u304b\u3089\u51fa\u305f");
        al::setMaterialCode(this, "Air");
        mInWater = false;
    }
    JointAimUtil::updateEyeJointInfo(this, mEyeInfo, 1500.0f, 0.2f);
    al::moveSyncRailLoop(this, mSpeed);
    sead::Vector3f previous;
    al::calcRailPosFront(&previous, this, -100.0f);
    sead::Vector3f direction = -(previous - al::getTrans(this));
    if (al::normalizeOrZero(&direction))
        direction = al::getRailDir(this);
    if (mItem) {
        sead::Vector3f itemPos;
        al::calcRailPosFront(&itemPos, this, -350.0f);
        mItemMtx.setTranslation(itemPos);
    }
    if (!al::isParallelDirection(direction, sead::Vector3f::ey, 0.01f)) {
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        auto* pQuat = al::getQuatPtr(this);
        al::turnQuatYDirRadian(pQuat, *pQuat, up.y >= 0.0f ? sead::Vector3f::ey : -sead::Vector3f::ey,
                              0.1745329201221466f);
    }
    auto* pQuat = al::getQuatPtr(this);
    al::turnQuatZDirRate(pQuat, *pQuat, direction, 1.0f);
}

/** @brief Plays the hit reaction while continuing rail movement. */
void SyumockRailMove::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "Reaction");
    mJointSpring->subControlRateAll(0.1f);
    updateMove();
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvSyumockRailMove.Move);
}

/** @brief Updates support freezing while suppressing joint motion. */
void SyumockRailMove::exeSupportFreeze() {
    mJointSpring->subControlRateAll(0.1f);
    updateMove();
    if (al::updateNerveState(this))
        al::setNerve(this, &NrvSyumockRailMove.Move);
}

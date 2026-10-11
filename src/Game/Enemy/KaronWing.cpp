#include "Enemy/KaronWing.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/FlyerStateChase.hpp"
#include "Enemy/FlyerStateChaseParam.hpp"
#include "Enemy/FlyerStateFindPlayer.hpp"
#include "Enemy/FlyerStateFindPlayerParam.hpp"
#include "Enemy/FlyerStateFunction.hpp"
#include "Enemy/FlyerStateParam.hpp"
#include "Enemy/FlyerStateReturnArea.hpp"
#include "Enemy/FlyerStateReturnAreaParam.hpp"
#include "Enemy/FlyerStateWander.hpp"
#include "Enemy/FlyerStateWanderParam.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/ActorParamHolder.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
NERVE_DECL(KaronWing, Wander)
NERVE_DECL(KaronWing, Chase)
NERVE_DECL(KaronWing, FindPlayer)
NERVE_DECL(KaronWing, ReturnArea)
NERVE_DECL(KaronWing, SupportFreeze)
NERVE_DECL(KaronWing, AttackHit)
NERVE_DECL(KaronWing, BreakDown)
NERVE_DECL(KaronWing, BreakWait)
NERVE_DECL(KaronWing, BreakReaction)
NERVE_DECL(KaronWing, Trampled)
NERVE_DECL(KaronWing, Reaction)
NERVE_DECL(KaronWing, Break)
NERVE_DECL(KaronWing, BreakGroundHit)
NERVE_DECL(KaronWing, RecoverSign)
NERVE_DECL(KaronWing, Recover)
NERVE_DECL(KaronWing, AttackHitWait)
// Most nerves live in .data; the attack hit nerves are constant objects next to their vtables.
KaronWingNrvWander NrvKaronWingWander;
KaronWingNrvChase NrvKaronWingChase;
KaronWingNrvFindPlayer NrvKaronWingFindPlayer;
KaronWingNrvReturnArea NrvKaronWingReturnArea;
KaronWingNrvSupportFreeze NrvKaronWingSupportFreeze;
const KaronWingNrvAttackHit NrvKaronWingAttackHit{};
KaronWingNrvBreakDown NrvKaronWingBreakDown;
KaronWingNrvBreakWait NrvKaronWingBreakWait;
KaronWingNrvBreakReaction NrvKaronWingBreakReaction;
KaronWingNrvTrampled NrvKaronWingTrampled;
KaronWingNrvReaction NrvKaronWingReaction;
KaronWingNrvBreak NrvKaronWingBreak;
KaronWingNrvBreakGroundHit NrvKaronWingBreakGroundHit;
KaronWingNrvRecoverSign NrvKaronWingRecoverSign;
KaronWingNrvRecover NrvKaronWingRecover;
const KaronWingNrvAttackHitWait NrvKaronWingAttackHitWait{};

typedef al::FunctorV0M<KaronWing*, void (KaronWing::*)()> KaronWingFunctor;

const al::ActorParamMove sWanderMoveParam = {0.1f, 0.0f, 0.95f, 0.7f};
FlyerStateParam sFlyerStateParam(0.9f, 0.97f);
FlyerStateWanderParam sWanderParam(30, 540, 180, "Fly", &sWanderMoveParam);
FlyerStateFindPlayerParam sFindPlayerParam(8.0f, const_cast<char*>("Find"));
FlyerStateChaseParam sChaseParam(0.6f, 600, 2.0f, 0.98f, 70, 100, "FlyChase", "Wait");
FlyerStateReturnAreaParam sReturnAreaParam(0.97f, 90, 3.0f, 0.35f, 30.0f, 300);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 110.0f, 0.0f));

/**
 * @brief Checks whether the actor is broken apart (trampled, broken or recovering).
 * @param pActor KaronWing actor.
 * @return Whether the actor is in one of the break nerves.
 */
bool isBreak(const KaronWing* pActor) {
    return al::isNerve(pActor, &NrvKaronWingTrampled) || al::isNerve(pActor, &NrvKaronWingBreak) ||
           al::isNerve(pActor, &NrvKaronWingBreakGroundHit) ||
           al::isNerve(pActor, &NrvKaronWingBreakWait) ||
           al::isNerve(pActor, &NrvKaronWingBreakReaction) ||
           al::isNerve(pActor, &NrvKaronWingBreakDown) ||
           al::isNerve(pActor, &NrvKaronWingRecoverSign) ||
           al::isNerve(pActor, &NrvKaronWingRecover);
}

/**
 * @brief Picks the next nerve (find the player, return to the area or wander) from the target.
 * @param pActor KaronWing actor.
 * @param pTargetFinder Target finder.
 * @param pArea Area the actor is limited to, or nullptr.
 */
void changeNerveByTarget(KaronWing* pActor, TargetFinder* pTargetFinder, al::AreaObjGroup* pArea) {
    pTargetFinder->refindTarget();
    if (pTargetFinder->isFoundTargetNow() &&
        (pArea == nullptr || al::tryIsInAreaObj(pArea, al::getTrans(pTargetFinder->getTarget())))) {
        al::setNerve(pActor, &NrvKaronWingFindPlayer);
        return;
    }

    if (!al::tryIsInAreaObj(pArea, al::getTrans(pActor)) && pArea != nullptr) {
        al::setNerve(pActor, &NrvKaronWingReturnArea);
    } else {
        al::setNerve(pActor, &NrvKaronWingWander);
    }
}

/**
 * @brief Moves an eye angle one degree back toward zero.
 * @param angle Current eye angle.
 * @return New eye angle.
 */
inline f32 decayEyeAngle(f32 angle) {
    if (angle > 0.0f) {
        angle -= 1.0f;
    }

    if (angle < 0.0f) {
        angle += 1.0f;
    }

    return angle;
}

/**
 * @brief Raises an eye angle toward a target by at most one degree, up to a maximum.
 * @param angle Current eye angle.
 * @param target Target eye angle.
 * @param max Maximum eye angle.
 * @return New eye angle.
 */
inline f32 raiseEyeAngle(f32 angle, f32 target, f32 max) {
    return sead::Mathf::clampMax(sead::Mathf::min(target, angle + 1.0f), max);
}

/**
 * @brief Lowers an eye angle toward a target by at most one degree, down to a minimum.
 * @param angle Current eye angle.
 * @param target Target eye angle.
 * @param min Minimum eye angle.
 * @return New eye angle.
 */
inline f32 lowerEyeAngle(f32 angle, f32 target, f32 min) {
    return sead::Mathf::clampMin(sead::Mathf::max(target, angle - 1.0f), min);
}
}  // namespace

/**
 * @brief Constructs a KaronWing.
 * @param pName Actor name.
 */
KaronWing::KaronWing(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the actor, its target finder, the flyer states and the eye joint.
 * @param rInfo Placement info of the actor.
 */
void KaronWing::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = al::isSingleMode(rInfo);
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvKaronWingWander, 5);
    mTargetFinderParam = new TargetFinderParam(1800.0f, 180.0f, 90.0f, 0x7fffffff, -1.0f, -1.0f,
                                               -1.0f, 2400.0f, false);
    al::tryGetArg(&mTargetFinderParam->_0, rInfo, "SearchDistance");
    mTargetFinderParam->_1C = mTargetFinderParam->_0 + 600.0f;
    mInitHeight = al::getTrans(this).y;
    mLinkAreaGroup = al::createLinkAreaGroup(this, rInfo, "ChaseArea",
                                             "追いかけ有効エリアグループ", "子供エリア");
    mTargetFinder = new TargetFinder(this, mTargetFinderParam);
    mStateChase = new FlyerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                      &sFlyerStateParam, &sChaseParam);
    mStateFindPlayer = new FlyerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                                &sFlyerStateParam, &sFindPlayerParam);
    mStateWander = new FlyerStateWander(this, al::getFrontPtr(this), mTargetFinder,
                                        &sFlyerStateParam, &sWanderParam);
    mStateReturnArea = new FlyerStateReturnArea(this, al::getFrontPtr(this), al::getTrans(this),
                                                &sFlyerStateParam, &sReturnAreaParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateChase, &NrvKaronWingChase, "[state]飛行敵追いかけ");
    al::initNerveState(this, mStateFindPlayer, &NrvKaronWingFindPlayer,
                       "[state]飛行敵プレイヤー発見");
    al::initNerveState(this, mStateWander, &NrvKaronWingWander, "[state]飛行敵うろつき");
    al::initNerveState(this, mStateReturnArea, &NrvKaronWingReturnArea, "[state]エリアに戻る");
    al::initNerveState(this, mStateSupportFreeze, &NrvKaronWingSupportFreeze, "[state]フリーズ");
    if (al::isExistJoint(this, "Eye")) {
        al::initJointControllerKeeper(this, 3);
        al::initJointLocalRotator(this, &mEyeRotate, "Eye");
    }

    al::listenStageSwitchOnKill(this, KaronWingFunctor(this, &KaronWing::killBySwitch));
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    makeActorAppeared();
    const sead::Vector3f& trans = al::getTrans(this);
    mInitTrans.x = trans.x;
    mInitTrans.y = trans.y;
    mInitTrans.z = trans.z;
    const sead::Vector3f& front = al::getFront(this);
    mInitFront.x = front.x;
    mInitFront.y = front.y;
    mInitFront.z = front.z;
}

/** @brief Dies with the death hit reaction when the kill stage switch turns on. */
void KaronWing::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }

    al::startHitReactionDeath(this);
    kill();
}

/** @brief Dies when touching a kill area or a deadly material. */
void KaronWing::control() {
    EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this);
}

/**
 * @brief Pushes other enemies and attacks the player.
 * @param pSelf Own sensor.
 * @param pOther Other sensor.
 */
void KaronWing::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isBreak(this)) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) &&
        al::sendMsgEnemyAttack(pOther, pSelf)) {
        al::faceToTarget(this, al::getSensorPos(pOther));
        al::setNerve(this, &NrvKaronWingAttackHit);
    }
}

/**
 * @brief Handles restore, player attacks, tramples, explosions, blow downs and pushes.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool KaronWing::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (al::isMsgRestore(pMsg)) {
        makeActorAppeared();
        mIsRestored = true;
        al::resetPosition(this, mInitTrans, false);
        al::setFront(this, mInitFront);
        al::setNerve(this, &NrvKaronWingWander);
        return true;
    }

    if (al::isNerve(this, &NrvKaronWingBreakDown)) {
        return false;
    }

    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
        al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
        al::isMsgPlayerGiantHipDrop(pMsg) || rc::isMsgJumpPanelAction(pMsg)) {
        al::setNerve(this, &NrvKaronWingBreakDown);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        return true;
    }

    if (al::isMsgPressureDeath(pMsg) && al::isSensorName(pSelf, "Body")) {
        mIsAppearItem = false;
        al::setNerve(this, &NrvKaronWingBreakDown);
        return true;
    }

    if (rc::isMsgBoxKillerBulletNoTouch(pMsg) && isBreak(this)) {
        return true;
    }

    if (al::isNerve(this, &NrvKaronWingBreakWait) && al::isMsgExplosion(pMsg)) {
        al::setNerve(this, &NrvKaronWingBreakReaction);
        return true;
    }

    if (isBreak(this) && !al::isNerve(this, &NrvKaronWingRecoverSign) &&
        !al::isNerve(this, &NrvKaronWingRecover)) {
        return false;
    }

    if (rc::isMsgBobsledBodyAttack(pMsg) || rc::isMsgBobsledTrample(pMsg)) {
        if (mIsSingleMode) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }

        al::setNerve(this, &NrvKaronWingTrampled);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    if (!isBreak(this) && al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
        return true;
    }

    if (EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setNerve(this, &NrvKaronWingTrampled);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    if (EnemyStateUtil::isMsgBlowDown(pMsg) || rc::isMsgNeedleRollerAttack(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        if (al::isMsgPlayerFireBallAttack(pMsg)) {
            if (!isBreak(this)) {
                al::setNerve(this, &NrvKaronWingReaction);
            }

            return true;
        }

        al::setNerve(this, &NrvKaronWingTrampled);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    return false;
}

/**
 * @brief Freezes the actor when it is touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool KaronWing::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (isBreak(this)) {
        return false;
    }

    if (!al::isNerve(this, &NrvKaronWingSupportFreeze)) {
        al::setNerve(this, &NrvKaronWingSupportFreeze);
    }

    return true;
}

/** @brief Wanders around, looking for the player in the area. */
void KaronWing::exeWander() {
    FlyerStateFunction::recoverHeight(this, mInitHeight, &sFlyerStateParam);
    resetEyes();
    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget()) {
        al::AreaObjGroup* area = mLinkAreaGroup;
        if (area == nullptr ||
            al::tryIsInAreaObj(area, al::getTrans(mTargetFinder->getTarget()))) {
            al::setNerve(this, &NrvKaronWingFindPlayer);
            mStateFindPlayer->_20 = true;
            return;
        }
    }

    al::updateNerveState(this);
}

/** @brief Moves the eyes one step back toward the center. */
void KaronWing::resetEyes() {
    f32 eyeY = mEyeRotate.y;
    f32 eyeZ = mEyeRotate.z;
    eyeY = decayEyeAngle(eyeY);
    eyeZ = decayEyeAngle(eyeZ);
    mEyeRotate.y = eyeY;
    mEyeRotate.z = eyeZ;
}

/** @brief Updates the find player state, then chases or wanders. */
void KaronWing::exeFindPlayer() {
    FlyerStateFunction::recoverHeight(this, mInitHeight, &sFlyerStateParam);
    moveEyesToPlayer();
    if (al::updateNerveState(this)) {
        if (mTargetFinder->isFoundTargetNow()) {
            al::setNerve(this, &NrvKaronWingChase);
        } else {
            al::setNerve(this, &NrvKaronWingWander);
        }
    }
}

/** @brief Turns the eyes toward the found target. */
void KaronWing::moveEyesToPlayer() {
    if (!mTargetFinder->isFoundTarget()) {
        return;
    }

    sead::Vector3f dir = al::getTrans(mTargetFinder->getTarget());
    dir -= al::getTrans(this);
    al::normalizeOrZero(&dir);
    if (al::isNearZero(dir, 0.001f)) {
        return;
    }

    sead::Vector3f horizontal = dir;
    sead::Vector3f vertical = dir;
    horizontal.y = 0.0f;
    vertical.x = 0.0f;
    vertical.z = 0.0f;
    al::normalizeOrZero(&horizontal);
    sead::Vector3f front = sead::Vector3f::ez;
    sead::Vector3f side = sead::Vector3f::ex;
    al::calcSideDir(&side, this);
    al::calcFrontDir(&front, this);
    vertical = vertical + front;
    al::normalizeOrZero(&vertical);
    f32 eyeY = mEyeRotate.y;
    f32 eyeZ = mEyeRotate.z;
    if (!al::isNearZero(horizontal, 0.001f)) {
        f32 angle = al::calcAngleOnPlaneDegree(front, horizontal, sead::Vector3f::ey);
        if (angle > 1.0f) {
            eyeY = raiseEyeAngle(eyeY, angle * 9.0f / 90.0f, 9.0f);
        } else if (angle < -1.0f) {
            eyeY = lowerEyeAngle(eyeY, angle * 9.0f / 90.0f, -9.0f);
        }
    }

    if (!al::isNearZero(vertical, 0.001f)) {
        f32 angle = al::calcAngleOnPlaneDegree(front, vertical, side);
        if (angle > 1.0f) {
            eyeZ = lowerEyeAngle(eyeZ, angle * -15.0f / 90.0f, -15.0f);
        } else if (angle < -1.0f) {
            eyeZ = raiseEyeAngle(eyeZ, angle * -15.0f / 90.0f, 15.0f);
        }
    }

    mEyeRotate.y = eyeY;
    mEyeRotate.z = eyeZ;
}

/** @brief Chases the target and returns to the area when leaving it. */
void KaronWing::exeChase() {
    FlyerStateFunction::recoverHeight(this, mInitHeight, &sFlyerStateParam);
    if (al::updateNerveState(this)) {
        changeNerveByTarget(this, mTargetFinder, mLinkAreaGroup);
        return;
    }

    moveEyesToPlayer();
    al::AreaObjGroup* area = mLinkAreaGroup;
    if (area == nullptr || al::tryIsInAreaObj(area, al::getTrans(this))) {
        return;
    }

    if (mTargetFinder->isFoundTarget() &&
        al::tryIsInAreaObj(mLinkAreaGroup, al::getTrans(mTargetFinder->getTarget()))) {
        return;
    }

    al::setNerve(this, &NrvKaronWingReturnArea);
}

/** @brief Flies back into the area, or looks for the player again when they enter it. */
void KaronWing::exeReturnArea() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Fly");
    }

    resetEyes();
    mTargetFinder->update();
    if (mTargetFinder->isFoundTarget() &&
        al::tryIsInAreaObj(mLinkAreaGroup, al::getTrans(mTargetFinder->getTarget()))) {
        al::setNerve(this, &NrvKaronWingFindPlayer);
    } else {
        if (!al::updateNerveState(this)) {
            return;
        }

        changeNerveByTarget(this, mTargetFinder, mLinkAreaGroup);
        if (!al::isNerve(this, &NrvKaronWingFindPlayer)) {
            return;
        }
    }

    mStateFindPlayer->_20 = true;
}

/** @brief Plays the trampled action, then breaks apart. */
void KaronWing::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Trampled");
    }

    resetEyes();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKaronWingBreak);
    }
}

/** @brief Breaks apart and falls to the ground. */
void KaronWing::exeBreak() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "Break");
        al::hideShadow(this, "WingL2");
        al::hideShadow(this, "WingR2");
        al::setColliderOffsetY(this, 60.0f);
        al::setVelocityZero(this);
    }

    al::addVelocityToGravity(this, 2.0f);
    al::scaleVelocity(this, 0.95f);
    if (al::isActionEnd(this) && al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvKaronWingBreakGroundHit);
    }
}

/** @brief Plays the ground hit action of the broken bones. */
void KaronWing::exeBreakGroundHit() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BreakGroundHit");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKaronWingBreakWait);
    }
}

/** @brief Lies broken on the ground for a while. */
void KaronWing::exeBreakWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BreakWait");
    }

    if (al::isGreaterEqualStep(this, 240)) {
        al::setNerve(this, &NrvKaronWingRecoverSign);
    }
}

/** @brief Reacts to an explosion while broken. */
void KaronWing::exeBreakReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BreakReaction");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKaronWingBreakWait);
    }
}

/** @brief Shakes before reassembling. */
void KaronWing::exeRecoverSign() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverSign");
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvKaronWingRecover);
    }
}

/** @brief Reassembles, then flies again. */
void KaronWing::exeRecover() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Recover");
        al::showShadow(this, "WingL2");
        al::showShadow(this, "WingR2");
    }

    if (al::isActionEnd(this)) {
        al::validateClipping(this);
        al::setColliderOffsetY(this, 90.0f);
        al::setVelocityZero(this);
        changeNerveByTarget(this, mTargetFinder, mLinkAreaGroup);
        if (al::isNerve(this, &NrvKaronWingFindPlayer)) {
            mStateFindPlayer->_20 = true;
        }
    }
}

/** @brief Plays the attack hit action. */
void KaronWing::exeAttackHit() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackHit");
    }

    al::setVelocityZero(this);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKaronWingAttackHitWait);
    }
}

/** @brief Waits in place after an attack hit. */
void KaronWing::exeAttackHitWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    al::setVelocityZero(this);
    if (al::isGreaterEqualStep(this, 60)) {
        changeNerveByTarget(this, mTargetFinder, mLinkAreaGroup);
    }
}

/** @brief Reacts to a fire ball or blow down. */
void KaronWing::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }

    al::setVelocityZero(this);
    if (al::isActionEnd(this)) {
        changeNerveByTarget(this, mTargetFinder, mLinkAreaGroup);
    }
}

/** @brief Breaks down for good, drops its item and dies. */
void KaronWing::exeBreakDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BreakDown");
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        if (mIsAppearItem) {
            al::appearItem(this);
        }

        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen by the touch screen until the freeze state ends. */
void KaronWing::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        changeNerveByTarget(this, mTargetFinder, mLinkAreaGroup);
    }
}

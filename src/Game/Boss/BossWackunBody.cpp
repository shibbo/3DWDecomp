#include "Boss/BossWackunBody.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>

#include "Boss/BossWackun.hpp"
#include "Boss/BossWackunHand.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(BossWackunBody, Wait);
NERVE_DECL(BossWackunBody, Damage);
NERVE_DECL(BossWackunBody, Recover);
NERVE_DECL(BossWackunBody, MoveEnd);
NERVE_DECL(BossWackunBody, StartDemo);
NERVE_DECL(BossWackunBody, Down);
NERVE_DECL(BossWackunBody, Turn);
NERVE_DECL(BossWackunBody, MoveStart);
NERVE_DECL(BossWackunBody, Move);
NERVES_MAKE_NOSTRUCT(BossWackunBody, Wait, Damage, Recover, MoveEnd, StartDemo, Down, Turn,
                     MoveStart, Move)

/** @brief The four corners (boss-local) the body slides between. */
const sead::Vector3f cMoveTargets[4] = {
    {175.0f, 175.0f, 0.0f},
    {-175.0f, 175.0f, 0.0f},
    {-175.0f, -175.0f, 0.0f},
    {175.0f, -175.0f, 0.0f},
};

constexpr s32 cMoveTargetNum = 4;

/**
 * @brief Copies a vector as plain data.
 * @param pDst Destination vector.
 * @param rSrc Source vector.
 */
void copyVec(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    static_cast<sead::BaseVec3<f32>&>(*pDst) = rSrc;
}
constexpr s32 cTurnStep = 20;
constexpr s32 cDownTurnStartStep = 104;
}  // namespace

/**
 * @brief Creates the body attached to BossWackun.
 * @param pBoss Boss the body is placed relative to.
 * @param pHand Hand that pushes the body around.
 */
BossWackunBody::BossWackunBody(BossWackun* pBoss, BossWackunHand* pHand)
    : al::LiveActor("ボスワックン体"), mBoss(pBoss), mHand(pHand) {}

/** @brief Kills the hand together with the body without any reaction. */
void BossWackunBody::makeActorDead() {
    mHand->makeActorDead();
    al::LiveActor::makeActorDead();
}

/** @brief Plays the death reaction and kills the hand together with the body. */
void BossWackunBody::kill() {
    al::startHitReactionDeath(this);
    mHand->kill();
    al::LiveActor::kill();
}

/**
 * @brief Initializes the body model and its waiting nerve, then appears.
 * @param rInfo Actor placement and scene initialization information.
 */
void BossWackunBody::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BossWackunBody", nullptr);
    al::initNerve(this, &NrvBossWackunBodyWait, 0);
    makeActorAppeared();
}

/**
 * @brief Attacks the player when the body lands on them face down.
 * @param pSelf Sensor of the body.
 * @param pOther Sensor that touched the body.
 */
void BossWackunBody::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!isEnableAttack()) {
        return;
    }

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    if (frontDir.y >= 0.49999997f) {
        sead::Vector3f localPos;
        al::multVecInvQuat(&localPos, this, al::getActorTrans(pOther));
        if (mAttackBox.isInside(localPos)) {
            al::sendMsgEnemyAttack(pOther, pSelf);
        }
    }
}

/** @return Whether the body may currently hurt the player. */
bool BossWackunBody::isEnableAttack() const {
    return !(al::isNerve(this, &NrvBossWackunBodyStartDemo) ||
             al::isNerve(this, &NrvBossWackunBodyDamage) ||
             al::isNerve(this, &NrvBossWackunBodyRecover) ||
             al::isNerve(this, &NrvBossWackunBodyDown));
}

/**
 * @brief Takes damage when the player stomps the weak face of the body.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the body.
 * @return Whether the message was handled.
 */
bool BossWackunBody::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                al::HitSensor* pSelf) {
    if (!al::isMsgPlayerTrample(pMsg) && !al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
        return false;
    }

    if (!al::isNerve(this, &NrvBossWackunBodyWait) &&
        !al::isNerve(this, &NrvBossWackunBodyDamage) &&
        !al::isNerve(this, &NrvBossWackunBodyRecover)) {
        return false;
    }

    if (!(al::getVelocity(al::getSensorHost(pOther)).y < 0.0f)) {
        return false;
    }

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    if (!(frontDir.y <= -0.49999997f)) {
        return false;
    }

    sead::Vector3f localPos;
    al::multVecInvQuat(&localPos, this, al::getActorTrans(pOther));
    if (!mTrampleBox.isInside(localPos)) {
        return false;
    }

    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    if (al::isNerve(this, &NrvBossWackunBodyWait)) {
        rc::addScore(this, pOther, 0.0f, mDamageCount);
        mDamageCount++;
        al::startHitReaction(this, "踏み");
        al::setNerve(this, &NrvBossWackunBodyDamage);
    }

    return true;
}

/**
 * @brief Calculates the index of the corner the body slides to next.
 * @param isForward Whether to advance to the next corner or go back to the previous one.
 * @return Next corner index, wrapped to the corner count.
 */
s32 BossWackunBody::calcNextMoveTargetIndex(bool isForward) const {
    f32 index = mMoveTargetIndex;
    if (isForward) {
        index += 1.0f;
        if (index >= cMoveTargetNum) {
            index = 0.0f;
        }
    } else {
        index -= 1.0f;
        if (index < 0.0f) {
            index = cMoveTargetNum - 1;
        }
    }

    return index;
}

/**
 * @brief Advances to the next corner and stores its position as the move target.
 * @param isForward Whether to advance to the next corner or go back to the previous one.
 */
void BossWackunBody::nextMoveTarget(bool isForward) {
    mMoveTargetIndex = calcNextMoveTargetIndex(isForward);
    mMoveTargetTrans = cMoveTargets[mMoveTargetIndex];
}

/** @return Whether the body finished its turn and slide. */
bool BossWackunBody::isTurnEnd() const {
    return al::isNerve(this, &NrvBossWackunBodyWait) ||
           al::isNerve(this, &NrvBossWackunBodyMoveEnd);
}

/** @return Whether the body is reacting to a stomp. */
bool BossWackunBody::isDamage() const {
    return al::isNerve(this, &NrvBossWackunBodyDamage);
}

/** @brief Appears for the battle start demo, with the hand hidden. */
void BossWackunBody::startDemo() {
    makeActorAppeared();
    al::startAction(this, "DemoBattleStart");
    mHand->makeActorDead();
    copyVec(&mLocalTrans, sead::Vector3f::zero);
    al::setNerve(this, &NrvBossWackunBodyStartDemo);
}

/** @brief Places the body on its first corner and starts the battle. */
void BossWackunBody::startBattle() {
    mHand->makeActorAppeared();
    copyVec(&mLocalTrans, cMoveTargets[0]);
    al::startAction(this, "Wait");
    al::setNerve(this, &NrvBossWackunBodyWait);
    mHand->startWait();
    al::clearSklAnimInterpole(this);
}

/** @brief Returns the body and its hand to waiting. */
void BossWackunBody::startWait() {
    al::startAction(this, "Wait");
    al::setNerve(this, &NrvBossWackunBodyWait);
    mHand->startWait();
}

/**
 * @brief Starts sliding towards the next corner after the boss tumbles.
 * @param rAxis Axis the boss tumbles around.
 * @param rotateType How the boss tumbles (see BossWackun::RotateType).
 * @param moveNum How many corners to slide through.
 * @param moveStep Steps before the slide starts.
 * @param moveTime Steps a single slide lasts.
 */
void BossWackunBody::startMove(const sead::Vector3f& rAxis, s32 rotateType, s32 moveNum,
                               s32 moveStep, s32 moveTime) {
    mMoveNum = moveNum;
    mMoveStep = moveStep;
    mMoveTime = moveTime;

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    if (rotateType == static_cast<s32>(BossWackun::RotateType::Rise)) {
        return;
    }

    sead::Vector3f bossFrontDir;
    al::calcFrontDir(&bossFrontDir, mBoss);
    mIsMoveForward = bossFrontDir.dot(rAxis) > 0.0f;
    nextMoveTarget(mIsMoveForward);
    if (mIsTurn) {
        al::setNerve(this, &NrvBossWackunBodyTurn);
    } else {
        al::setNerve(this, &NrvBossWackunBodyMoveStart);
    }
}

/**
 * @brief Plays the tumble animation matching the boss rotation.
 * @param rotateType How the boss tumbles (see BossWackun::RotateType).
 */
void BossWackunBody::startRotate(s32 rotateType) {
    const char* actionName;
    switch (rotateType) {
    case static_cast<s32>(BossWackun::RotateType::FallBack):
        actionName = "FallBack";
        break;
    case static_cast<s32>(BossWackun::RotateType::FallFront):
        actionName = "FallFront";
        break;
    case static_cast<s32>(BossWackun::RotateType::Rise):
        if (al::calcQuatFrontY(al::getQuat(this)) > 0.0f) {
            sead::Vector3f sideDir;
            al::calcSideDir(&sideDir, this);
            actionName = sideDir.dot(mBoss->getRotateAxis()) > 0.70700002f ? "RiseFront" :
                                                                             "RiseFrontReverse";
        } else {
            al::startAction(this, "RiseBack");
            return;
        }
        break;
    default:
        al::tryStartActionIfNotPlaying(this, "Wait");
        return;
    }

    al::startAction(this, actionName);
}

/**
 * @brief Plays the landing animation matching the boss rotation.
 * @param rotateType How the boss tumbled (see BossWackun::RotateType).
 */
void BossWackunBody::startLand(s32 rotateType) {
    switch (rotateType) {
    case static_cast<s32>(BossWackun::RotateType::FallFront):
        al::startAction(this, "LandFront");
        return;
    case static_cast<s32>(BossWackun::RotateType::FallBack):
        al::startAction(this, "LandBack");
        return;
    case static_cast<s32>(BossWackun::RotateType::Side):
    case static_cast<s32>(BossWackun::RotateType::Rise):
        al::startAction(this, "Land");
        return;
    default:
        al::tryStartActionIfNotPlaying(this, "Wait");
        return;
    }
}

/** @brief Stands the body back up after it was knocked down. */
void BossWackunBody::startStandUp() {
    al::startAction(this, "StandUp");
    al::setNerve(this, &NrvBossWackunBodyRecover);
}

/** @brief Restores collision and sensors and brings the hand back. */
void BossWackunBody::startRevival() {
    al::validateCollisionParts(this);
    al::validateHitSensors(this);
    al::setNerve(this, &NrvBossWackunBodyWait);
    mHand->appear();
    mHand->startWait();
}

/**
 * @brief Knocks the body down.
 * @param rDir Direction the body turns towards while lying down.
 */
void BossWackunBody::startDown(const sead::Vector3f& rDir) {
    copyVec(&mDownDir, rDir);
    al::startAction(this, "Down");
    al::setNerve(this, &NrvBossWackunBodyDown);
}

/** @brief Follows the boss pose during the battle start demo. */
void BossWackunBody::exeStartDemo() {
    // The first-step check is kept but has no effect (its body was presumably debug-only).
    al::isFirstStep(this);
    updatePose();
}

/** @brief Places the body at its local pose relative to the boss. */
void BossWackunBody::updatePose() {
    al::multVecPose(al::getTransPtr(this), mBoss, mLocalTrans);
    al::getQuatPtr(this)->setMul(al::getQuat(mBoss), mLocalQuat);
}

/** @brief Follows the boss pose while waiting. */
void BossWackunBody::exeWait() {
    updatePose();
}

/** @brief Turns the body half a revolution around its local Y axis. */
void BossWackunBody::exeTurn() {
    if (al::isFirstStep(this)) {
        mTurnStartDegree = mTurnDegree;
        al::startSe(this, "Move");
    }

    f32 degree =
        al::calcNerveEaseInOutValue(this, cTurnStep, mTurnStartDegree, mTurnStartDegree + 180.0f);
    mTurnDegree = al::wrapAngle(degree);
    sead::Quatf turnQuat;
    turnQuat.setAxisAngle(sead::Vector3f::ey, mTurnDegree);
    mLocalQuat = turnQuat;
    updatePose();

    if (al::isGreaterEqualStep(this, cTurnStep)) {
        al::startHitReaction(this, "回転終了");
        mTurnDegree = mTurnDegree < 90.0f ? 0.0f : 180.0f;
        turnQuat.setAxisAngle(sead::Vector3f::ey, mTurnDegree);
        mLocalQuat = turnQuat;
        al::setNerve(this, &NrvBossWackunBodyMoveStart);
    }
}

/**
 * @brief Plays the slide action matching the move direction in body-local space.
 * @param pRight Action for sliding right.
 * @param pLeft Action for sliding left.
 * @param pDown Action for sliding down.
 * @param pUp Action for sliding up.
 */
ALWAYS_INLINE void BossWackunBody::startMoveAction(const char* pRight, const char* pLeft,
                                                   const char* pDown, const char* pUp) {
    sead::Vector3f moveDir = mMoveTargetTrans - mMoveStartTrans;
    sead::Quatf invQuat(mLocalQuat.w, -mLocalQuat.x, -mLocalQuat.y, -mLocalQuat.z);
    sead::Vector3f localDir;
    localDir.setRotated(invQuat, moveDir);

    if (sead::Mathf::abs(localDir.x) > sead::Mathf::abs(localDir.y)) {
        al::startAction(this, localDir.x > 0.0f ? pLeft : pRight);
    } else {
        al::startAction(this, localDir.y > 0.0f ? pUp : pDown);
    }
}

/** @brief Starts the hand pushing and waits until the slide begins. */
void BossWackunBody::exeMoveStart() {
    if (al::isFirstStep(this)) {
        mHand->startMove(mMoveTargetIndex);
        mMoveStartTrans = mLocalTrans;
        al::startSe(this, "Move");
        startMoveAction("MoveRight", "MoveLeft", "MoveDown", "MoveUp");
    }

    updatePose();

    if (al::isGreaterEqualStep(this, mMoveStep - 1)) {
        al::setNerve(this, &NrvBossWackunBodyMove);
    }
}

/** @brief Slides the body to the target corner, then continues or stops. */
void BossWackunBody::exeMove() {
    // The first-step check is kept but has no effect (its body was presumably debug-only).
    al::isFirstStep(this);
    f32 rate = al::calcNerveEaseInRate(this, 0, mMoveTime);
    al::lerpVec(&mLocalTrans, mMoveStartTrans, mMoveTargetTrans, rate);
    updatePose();

    if (al::isGreaterEqualStep(this, mMoveTime) && mHand->isEndMove()) {
        al::startHitReaction(this, "移動終了");
        startMoveAction("MoveEndRight", "MoveEndLeft", "MoveEndDown", "MoveEndUp");
        mMoveNum--;
        if (mMoveNum <= 0) {
            al::setNerve(this, &NrvBossWackunBodyMoveEnd);
            return;
        }

        nextMoveTarget(mIsMoveForward);
        al::setNerve(this, &NrvBossWackunBodyMoveStart);
    }
}

/** @brief Waits for the slide end animation, then returns to waiting. */
void BossWackunBody::exeMoveEnd() {
    updatePose();

    if (al::isActionEnd(this)) {
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvBossWackunBodyWait);
    }
}

/** @brief Gets pressed down together with the hand after a stomp. */
void BossWackunBody::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PressDown");
        mHand->startControled();
        al::startAction(mHand, "PressDown");
    }

    updatePose();
}

/** @brief Follows the boss pose while standing back up. */
void BossWackunBody::exeRecover() {
    updatePose();
}

/** @brief Lies knocked down, slowly turning towards the down direction. */
void BossWackunBody::exeDown() {
    if (al::isFirstStep(this)) {
        updatePose();
    }

    if (al::isGreaterEqualStep(this, cDownTurnStartStep)) {
        sead::Quatf* quat = al::getQuatPtr(this);
        al::turnQuatYDirRadian(quat, *quat, mDownDir, sead::Mathf::deg2rad(1.0f));
    }
}

/** @brief Destroys the body actor's base resources. */
BossWackunBody::~BossWackunBody() = default;

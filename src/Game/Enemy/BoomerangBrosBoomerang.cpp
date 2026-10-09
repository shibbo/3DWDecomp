#include "Enemy/BoomerangBrosBoomerang.hpp"

#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/AcquireItemFunc.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(BoomerangBrosBoomerang, MoveStart)
NERVE_DECL(BoomerangBrosBoomerang, Wait)
NERVE_DECL(BoomerangBrosBoomerang, Catch)
NERVE_DECL(BoomerangBrosBoomerang, MoveBrake)
NERVE_DECL(BoomerangBrosBoomerang, MoveStay)
NERVE_DECL(BoomerangBrosBoomerang, MoveBack)
NERVES_MAKE_NOSTRUCT(BoomerangBrosBoomerang, MoveStart, Wait, Catch, MoveBrake, MoveStay,
                     MoveBack)
}  // namespace

/**
 * @brief Checks whether the boomerang has been thrown and is still flying.
 * @return Whether the boomerang is in one of the flying nerves.
 */
inline bool BoomerangBrosBoomerang::isFlying() {
    return al::isNerve(this, &NrvBoomerangBrosBoomerangMoveStart) ||
           al::isNerve(this, &NrvBoomerangBrosBoomerangMoveBrake) ||
           al::isNerve(this, &NrvBoomerangBrosBoomerangMoveStay) ||
           al::isNerve(this, &NrvBoomerangBrosBoomerangMoveBack);
}

/** @brief Places the boomerang at its offset from the holder matrix. */
inline void BoomerangBrosBoomerang::updateAttachPose() {
    const sead::Matrix34f* hostMtx = mHostMtx;
    sead::Matrix34f rotateMtx;
    sead::Vector3f rotate = mRotate;
    rotateMtx.makeR({sead::Mathf::deg2rad(rotate.x), sead::Mathf::deg2rad(rotate.y),
                     sead::Mathf::deg2rad(rotate.z)});
    sead::Matrix34f transMtx;
    transMtx.makeRT({0.0f, 0.0f, 0.0f}, mTrans);
    sead::Matrix34f localMtx;
    localMtx.setMul(rotateMtx, transMtx);
    sead::Matrix34f poseMtx;
    poseMtx.setMul(*hostMtx, localMtx);
    al::updatePoseMtx(this, &poseMtx);
    al::resetPosition(this, false);
}

/** @brief Stops the boomerang and disables its sensors before it gets killed. */
inline void BoomerangBrosBoomerang::stopForKill() {
    al::setVelocityZero(this);
    al::invalidateHitSensors(this);
}

/**
 * @brief Plays a hit reaction and kills the boomerang.
 * @param pReaction Name of the hit reaction.
 * @param isAppearItem Whether a destroying reaction drops the item.
 */
inline void BoomerangBrosBoomerang::killWithHitReaction(const char* pReaction, bool isAppearItem) {
    al::startHitReaction(this, pReaction);
    bool isDestroy = al::isEqualString(pReaction, "燃える") || al::isEqualString(pReaction, "破壊");
    if (isDestroy && isAppearItem) {
        al::appearItem(this);
    }

    kill();
}

/**
 * @brief Stops the boomerang, plays a hit reaction and kills it.
 * @param pReaction Name of the hit reaction.
 * @param isAppearItem Whether a destroying reaction drops the item.
 */
inline void BoomerangBrosBoomerang::killByHitReaction(const char* pReaction, bool isAppearItem) {
    stopForKill();
    killWithHitReaction(pReaction, isAppearItem);
}

/**
 * @brief Constructs a boomerang.
 * @param pName Actor name.
 */
BoomerangBrosBoomerang::BoomerangBrosBoomerang(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model and nerve; the boomerang starts dead until it is attached.
 * @param rInfo Init info of the owner.
 */
void BoomerangBrosBoomerang::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BoomerangBrosBoomerang", nullptr);
    al::initNerve(this, &NrvBoomerangBrosBoomerangMoveStart, 0);
    al::invalidateClipping(this);
    al::invalidateHitSensors(this);
    makeActorDead();
}

/**
 * @brief Hits players, enemies and (in Bowser's Fury) map objects while flying.
 * @param pSelf Attack sensor of the boomerang.
 * @param pOther Contacted sensor.
 */
void BoomerangBrosBoomerang::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!isFlying()) {
        return;
    }

    sead::Vector3f up;
    al::calcUpDir(&up, this);
    if (!al::isHitCircleSensor(pOther, pSelf, up, al::getSensorRadius(pSelf), 5.0f)) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf)) {
        if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) &&
            al::sendMsgEnemyAttack(pOther, pSelf)) {
            killByHitReaction("プレイヤーヒット", true);
            return;
        }

        if ((al::isSensorEnemyBody(pOther) && al::sendMsgEnemyAttackBoomerang(pOther, pSelf)) ||
            (al::isSensorRide(pOther) && al::sendMsgEnemyAttackBoomerang(pOther, pSelf))) {
            killByHitReaction("敵ヒット", true);
        }

        return;
    }

    if (al::isSensorGoalItem(pOther)) {
        if (al::sendMsgEnemyAttackBoomerang(pOther, pSelf)) {
            killByHitReaction("敵ヒット", true);
        }

        return;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        (al::isSensorMapObj(pOther) || al::isSensorKickKoura(pOther)) &&
        al::sendMsgEnemyAttackBoomerang(pOther, pSelf)) {
        killByHitReaction("破壊", false);
    }
}

/**
 * @brief Gets destroyed by attacks while flying.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Receiving sensor of the boomerang.
 * @return Whether the message was handled.
 */
bool BoomerangBrosBoomerang::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                        al::HitSensor* pSelf) {
    if (!isFlying()) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (!EnemyStateUtil::isMsgBlowDown(pMsg) &&
        !EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) &&
        !rc::isMsgBubbleAttack(pMsg)) {
        return false;
    }

    if (al::isMsgEnemyAttackBoomerang(pMsg)) {
        return false;
    }

    rc::setAppearItemFactorByMsg(this, pMsg, pOther);
    if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg)) {
        killByHitReaction("燃える", true);
        return true;
    }

    bool isBubble = rc::isMsgBubbleAttack(pMsg);
    stopForKill();
    if (isBubble) {
        killWithHitReaction("燃える", false);
    } else {
        killWithHitReaction("破壊", true);
    }

    return true;
}

/**
 * @brief Gets destroyed by a touch-screen tap while flying.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the boomerang.
 * @return Whether the message was handled.
 */
bool BoomerangBrosBoomerang::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                                   al::ScreenPointer* pPointer,
                                                   al::ScreenPointTarget* pTarget) {
    if (!isFlying()) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        al::setAppearItemFactor(this, "間接攻撃", nullptr);
        killByHitReaction("破壊", true);
        return true;
    }

    return false;
}

/**
 * @brief Attaches the boomerang to the holder's hand and makes it appear.
 * @param pMtx Matrix to follow.
 * @param rTrans Offset from the matrix.
 * @param rRotate Rotation (degrees) relative to the matrix.
 */
void BoomerangBrosBoomerang::attach(const sead::Matrix34f* pMtx, const sead::Vector3f& rTrans,
                                    const sead::Vector3f& rRotate) {
    mHostMtx = pMtx;
    mTrans = rTrans;
    mRotate = rRotate;
    updateAttachPose();
    al::setVelocityZero(this);
    al::offCollide(this);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        al::validateClipping(this);
    }

    al::setNerve(this, &NrvBoomerangBrosBoomerangWait);
    makeActorAppeared();
}

/**
 * @brief Throws the boomerang.
 * @param rVelocity Initial velocity.
 */
void BoomerangBrosBoomerang::shoot(const sead::Vector3f& rVelocity) {
    al::setVelocity(this, rVelocity);
    al::onCollide(this);
    al::startAction(this, "Spin");
    al::setRotate(this, {0.0f, 0.0f, 0.0f});
    al::setNerve(this, &NrvBoomerangBrosBoomerangMoveStart);
    makeActorAppeared();
    mFlyDir.set(rVelocity);
    if (al::isNearZero(mFlyDir, 0.001f)) {
        al::calcFrontDir(&mFlyDir, this);
    }

    al::normalize(&mFlyDir);
    mFlySpeed = rVelocity.length();
    mFlyStep = 1000.0f / mFlySpeed;
}

/**
 * @brief Gets caught by the holder again.
 * @param pMtx Matrix to follow.
 * @param rTrans Offset from the matrix.
 * @param rRotate Rotation (degrees) relative to the matrix.
 */
void BoomerangBrosBoomerang::caught(const sead::Matrix34f* pMtx, const sead::Vector3f& rTrans,
                                    const sead::Vector3f& rRotate) {
    mHostMtx = pMtx;
    mTrans = rTrans;
    mRotate = rRotate;
    updateAttachPose();
    al::setNerve(this, &NrvBoomerangBrosBoomerangCatch);
    al::setVelocityZero(this);
    al::offCollide(this);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        al::validateClipping(this);
    }
}

/** @brief Follows the holder's hand. */
void BoomerangBrosBoomerang::exeWait() {
    updateAttachPose();
}

/** @brief Flies forward until the throw distance is covered. */
void BoomerangBrosBoomerang::exeMoveStart() {
    if (al::isFirstStep(this)) {
        al::validateHitSensors(this);
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
            al::invalidateClipping(this);
        }
    }

    if (al::isCollidedWall(this)) {
        killByHitReaction("破壊", false);
    }

    if (al::isGreaterStep(this, mFlyStep)) {
        al::setNerve(this, &NrvBoomerangBrosBoomerangMoveBrake);
    }
}

/** @brief Brakes until the boomerang stops or turns around. */
void BoomerangBrosBoomerang::exeMoveBrake() {
    sead::Vector3f flyDir = mFlyDir;
    al::addVelocity(this, flyDir * -0.95f);
    if (al::isNearZero(al::getVelocity(this), 0.001f) ||
        al::isReverseDirection(al::getVelocity(this), mFlyDir, 0.01f)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvBoomerangBrosBoomerangMoveStay);
    }
}

/** @brief Hovers in place for a moment. */
void BoomerangBrosBoomerang::exeMoveStay() {
    if (al::isGreaterStep(this, 30)) {
        al::setNerve(this, &NrvBoomerangBrosBoomerangMoveBack);
    }
}

/** @brief Flies back, vanishing after a while if nobody catches it. */
void BoomerangBrosBoomerang::exeMoveBack() {
    if (al::isCollidedWall(this)) {
        killByHitReaction("破壊", false);
    }

    sead::Vector3f flyDir = mFlyDir;
    al::tryAddVelocityLimit(this, flyDir * -0.95f, mFlySpeed);
    if (al::isGreaterStep(this, 180)) {
        killByHitReaction("自然消滅", true);
    }
}

/** @brief Plays the catch animation in the holder's hand, then disappears. */
void BoomerangBrosBoomerang::exeCatch() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensors(this);
        al::startAction(this, "Catch");
    }

    updateAttachPose();
    if (al::isActionEnd(this)) {
        kill();
    }
}

/**
 * @brief Checks whether the boomerang is flying back.
 * @return Whether the boomerang is in the MoveBack nerve.
 */
bool BoomerangBrosBoomerang::isMoveBack() {
    return al::isNerve(this, &NrvBoomerangBrosBoomerangMoveBack);
}

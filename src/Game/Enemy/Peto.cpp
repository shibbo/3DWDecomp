#include "Enemy/Peto.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Peto, Wait)
NERVE_DECL(Peto, Standby)
NERVE_DECL(Peto, BlowDown)
NERVE_DECL(Peto, SupportFreeze)
NERVE_DECL(Peto, KeyMoveAttack)
NERVE_DECL(Peto, PressDown)
NERVE_DECL(Peto, Attack)
NERVE_DECL(Peto, AttackOmen)
NERVE_DECL(Peto, AttackStart)
NERVE_DECL(Peto, AttackEnd)
// Non-const nerve objects: the game keeps them in .data.
PetoNrvAttackOmen NrvPetoAttackOmen;
PetoNrvAttackStart NrvPetoAttackStart;
PetoNrvAttack NrvPetoAttack;
PetoNrvKeyMoveAttack NrvPetoKeyMoveAttack;
PetoNrvAttackEnd NrvPetoAttackEnd;
PetoNrvWait NrvPetoWait;
PetoNrvStandby NrvPetoStandby;
PetoNrvBlowDown NrvPetoBlowDown;
PetoNrvSupportFreeze NrvPetoSupportFreeze;
PetoNrvPressDown NrvPetoPressDown;

EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));

/**
 * @brief Moves an eye rotation a step towards zero.
 * @param value Current rotation.
 * @return Rotation after the step.
 */
inline f32 approachZero(f32 value) {
    if (value > 0.0f) {
        value -= 0.25f;
    }

    if (value < 0.0f) {
        value += 0.25f;
    }

    return value;
}
}  // namespace

typedef al::FunctorV0M<Peto*, void (Peto::*)()> PetoFunctor;

/**
 * @brief Constructs a Peto.
 * @param pName Actor name.
 */
Peto::Peto(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Reads the placement parameters and initializes the model, key poses, states and
 * sensors.
 * @param rInfo Placement info of the actor.
 */
void Peto::init(const al::ActorInitInfo& rInfo) {
    al::tryGetArg(&mWaitTime, rInfo, "ScaleDownTime");
    al::tryGetArg(&mAttackTime, rInfo, "ScaleUpTime");
    al::tryGetArg(&mMoveSpeed, rInfo, "MoveSpeed");
    al::tryGetArg(&mDelay, rInfo, "Delay");
    al::tryGetArg(&mIsUseShadowMask, rInfo, "IsUseShadowMask");
    al::initActorWithArchiveName(this, rInfo, "Peto",
                                 mIsUseShadowMask ? "UseShadowMask" : "UseDepthShadow");
    al::initNerve(this, &NrvPetoWait, 2);
    if (mDelay > 0) {
        al::setNerve(this, &NrvPetoStandby);
    }

    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    f32 distance = al::calcDistanceNextKeyTrans(mKeyPoseKeeper);
    if (al::getKeyPoseCount(mKeyPoseKeeper) >= 2) {
        if (distance < 1.0f) {
            makeActorDead();
            return;
        }

        f32 radius = 0.0f;
        al::calcKeyMoveClippingInfo(&mClippingTrans, &radius, mKeyPoseKeeper, 0.0f);
        radius += al::getClippingRadius(this);
        al::setClippingInfo(this, radius, &mClippingTrans);
    }

    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateBlowDown, &NrvPetoBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvPetoSupportFreeze, "[state]タッチ拘束");
    mMtxConnector = al::createMtxConnector(this);
    al::listenStageSwitchOnKill(this, PetoFunctor(this, &Peto::killBySwitch));
    al::calcFrontDir(&mFrontDir, this);
    al::calcSideDir(&mSideDir, this);
    if (al::isExistJoint(this, "AttackEyes")) {
        al::initJointControllerKeeper(this, 3);
        al::initJointLocalRotator(this, &mEyeRotate, "AttackEyes");
    }

    if (mIsUseShadowMask) {
        mShadowMaskSize.set(mShadowMaskSizeInit);
    }

    al::offCollide(this);
    al::tryListenStageSwitchAppear(this);
}

/** @brief Dies when the kill stage switch turns on. */
void Peto::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }

    kill();
}

/** @brief Attaches the actor to the collision below it. */
void Peto::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
}

/** @brief Follows the connected collision, moves the eyes and notifies the floor it touches. */
void Peto::control() {
    if (al::isNerve(this, &NrvPetoBlowDown)) {
        return;
    }

    if (al::getKeyPoseCount(mKeyPoseKeeper) == 1) {
        if (mMtxConnector != nullptr) {
            al::connectPoseQT(this, mMtxConnector);
        }
    } else if (mMtxConnector != nullptr) {
        if (al::isNerve(this, &NrvPetoKeyMoveAttack) && al::isGreaterEqualStep(this, 1)) {
            al::connectPoseQT(this, mMtxConnector, al::getConnectBaseQuat(mMtxConnector),
                              al::getTrans(this));
        } else {
            al::connectPoseQT(this, mMtxConnector, al::getConnectBaseQuat(mMtxConnector),
                              al::getCurrentKeyTrans(mKeyPoseKeeper));
        }
    }

    if (al::isNerve(this, &NrvPetoPressDown) || al::isNerve(this, &NrvPetoBlowDown)) {
        return;
    }

    if (mIsUseShadowMask) {
        al::setShadowDropDirActorDown(this);
    }

    moveEyesToMoveDir();

    al::Triangle triangle;
    sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f up = sead::Vector3f::ey;
    al::calcQuatUp(&up, al::getQuat(this));
    if (!alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle,
                                              al::getTrans(this) + up * 20.0f, up * -50.0f,
                                              nullptr, nullptr)) {
        return;
    }

    al::HitSensor* floorSensor = triangle.getSensor();
    if (floorSensor != nullptr) {
        al::sendMsgEnemyFloorTouch(floorSensor, al::getHitSensor(this, "Body"));
    }
}

/** @brief Turns the eyes towards the direction to the next key pose. */
void Peto::moveEyesToMoveDir() {
    if (al::getKeyPoseCount(mKeyPoseKeeper) == 1) {
        return;
    }

    sead::Vector3f dir = sead::Vector3f::ez;
    al::calcDirToNextKey(&dir, mKeyPoseKeeper);
    if (al::isNearZero(dir, 0.001f)) {
        return;
    }

    f32 frontDot = mFrontDir.dot(dir);
    f32 sideDot = dir.dot(mSideDir);
    f32 eyeRotateY = sead::Mathf::clampMax(mEyeRotate.y - frontDot * 0.25f, 2.5f);
    f32 eyeRotateZ = sead::Mathf::clampMax(mEyeRotate.z - sideDot * 0.25f, 2.5f);
    mEyeRotate.y = sead::Mathf::clampMin(eyeRotateY, -2.5f);
    mEyeRotate.z = sead::Mathf::clampMin(eyeRotateZ, -2.5f);
}

/** @brief Dies with a hit reaction. */
void Peto::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/**
 * @brief Checks whether the other sensor is inside the area slammed by the attack.
 * @param pSelf Sensor of the Peto.
 * @param pOther Other sensor.
 * @return Whether the other sensor is inside the attack area.
 */
ALWAYS_INLINE bool Peto::isInAttackRange(al::HitSensor* pSelf, al::HitSensor* pOther) {
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    if (sead::Mathf::abs(al::calcDistanceV(up, pSelf, pOther)) > 90.0f) {
        return false;
    }

    sead::Vector3f diff = al::getSensorPos(pOther) - al::getTrans(this);
    al::verticalizeVec(&diff, -up, diff);
    if (sead::Mathf::abs(mFrontDir.dot(diff)) > 220.0f) {
        return false;
    }

    return !(sead::Mathf::abs(diff.dot(mSideDir)) > 220.0f);
}

/**
 * @brief Pushes enemies and damages players under the Peto while it is slamming the floor.
 * @param pSelf Sensor of the Peto.
 * @param pOther Sensor that was hit.
 */
void Peto::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvPetoPressDown) || al::isNerve(this, &NrvPetoBlowDown)) {
        return;
    }

    if (al::isSensorEnemy(pSelf) && al::isSensorEnemyAttack(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }

    if ((al::isNerve(this, &NrvPetoAttackStart) && al::isGreaterEqualStep(this, 14)) ||
        al::isNerve(this, &NrvPetoAttack) || al::isNerve(this, &NrvPetoKeyMoveAttack)) {
        if (!al::isSensorEnemyBody(pSelf) || !al::isSensorPlayer(pOther)) {
            return;
        }

        if (isInAttackRange(pSelf, pOther)) {
            al::sendMsgEnemyAttack(pOther, pSelf);
        }

        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
    }
}

/**
 * @brief Handles stomps and knockbacks. While slamming the floor only player attacks from close
 * by knock it away.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Peto that received the message.
 * @return Whether the message was handled.
 */
bool Peto::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvPetoPressDown) || al::isNerve(this, &NrvPetoBlowDown)) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if ((al::isNerve(this, &NrvPetoAttackStart) && al::isGreaterEqualStep(this, 14)) ||
        al::isNerve(this, &NrvPetoAttack) || al::isNerve(this, &NrvPetoKeyMoveAttack)) {
        if (!isInAttackRange(pSelf, pOther)) {
            return false;
        }

        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
            al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgExplosion(pMsg)) {
            EnemyStateUtil::requestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, true);
            al::setNerve(this, &NrvPetoBlowDown);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }

        return false;
    }

    if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                        &NrvPetoPressDown, true) ||
        EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mStateBlowDown,
                                                       &NrvPetoBlowDown, true)) {
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        return true;
    }

    return false;
}

/**
 * @brief Handles the touch screen: touching the Peto while it waits freezes it, and burn assists
 * on the attacking Peto are consumed when they come from above.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the Peto.
 * @return Whether the message was handled.
 */
bool Peto::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) {
    if ((al::isNerve(this, &NrvPetoAttack) || al::isNerve(this, &NrvPetoKeyMoveAttack)) &&
        rc::isMsgTouchAssistBurnPeto(pMsg) && al::isScreenPointTargetName(pTarget, "Touch")) {
        sead::Vector3f trans = al::getTrans(this);
        sead::Vector3f dir = al::getHitScreenPointTargetPos(pPointer);
        dir -= trans;
        if (al::isNearZero(dir, 0.001f)) {
            return true;
        }

        sead::Vector3f up = sead::Vector3f::ey;
        al::calcUpDir(&up, this);
        sead::Vector3f normDir = dir;
        al::normalize(&normDir);
        if (normDir.dot(up) * dir.length() < 0.0f) {
            return false;
        }

        return true;
    }

    if (!al::isNerve(this, &NrvPetoStandby) && !al::isNerve(this, &NrvPetoWait) &&
        !al::isNerve(this, &NrvPetoSupportFreeze)) {
        return false;
    }

    if (!al::isScreenPointTargetName(pTarget, "Body") ||
        !mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (al::isNerve(this, &NrvPetoStandby) || al::isNerve(this, &NrvPetoWait)) {
        al::setNerve(this, &NrvPetoSupportFreeze);
    }

    return true;
}

/** @brief Waits for the placement delay before starting the attack cycle. */
void Peto::exeStandby() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    mDelay--;
    if (mDelay <= 0) {
        al::setNerve(this, &NrvPetoWait);
    }
}

/** @brief Waits before the next attack. */
void Peto::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isGreaterEqualStep(this, mWaitTime)) {
        al::setNerve(this, &NrvPetoAttackOmen);
    }
}

/** @brief Plays the attack warning animation. */
void Peto::exeAttackOmen() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackOmen");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPetoAttackStart);
    }
}

/** @brief Slams onto the floor, growing the shadow, and enables the floor attack. */
void Peto::exeAttackStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackFloor");
    }

    if (mIsUseShadowMask) {
        al::setShadowMaskSize(this, "体影", mShadowMaskSize);
        if (al::isGreaterEqualStep(this, 10) && al::isLessStep(this, 16)) {
            mShadowMaskSize += sead::Vector3f(40.0f, 0.0f, 40.0f);
        }
    }

    if (al::isStep(this, 14)) {
        changeToAttackSensor();
    }

    if (al::isActionEnd(this)) {
        if (al::getKeyPoseCount(mKeyPoseKeeper) >= 2) {
            al::setNerve(this, &NrvPetoKeyMoveAttack);
        } else {
            al::setNerve(this, &NrvPetoAttack);
        }
    }
}

/** @brief Disables the attack sensor and grows the body sensor to cover the slammed area. */
void Peto::changeToAttackSensor() {
    al::invalidateHitSensor(this, "Attack");
    al::setSensorFollowPosOffset(this, "Body", sead::Vector3f(0.0f, 0.0f, 0.0f));
    al::setSensorRadius(this, "Body", 250.0f);
}

/** @brief Stays on the floor for a while. */
void Peto::exeAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FloorDamage");
    }

    if (al::isGreaterEqualStep(this, mAttackTime)) {
        al::setNerve(this, &NrvPetoAttackEnd);
        changeToWaitSensor();
    }
}

/** @brief Enables the attack sensor and restores the body sensor. */
void Peto::changeToWaitSensor() {
    al::validateHitSensor(this, "Attack");
    al::setSensorFollowPosOffset(this, "Body", sead::Vector3f(0.0f, 70.0f, 0.0f));
    al::setSensorRadius(this, "Body", 120.0f);
}

/** @brief Slides along the floor to the next key pose. */
void Peto::exeKeyMoveAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FloorDamage");
        mKeyMoveTime = al::calcTimeToNextKeyMove(mKeyPoseKeeper, mMoveSpeed);
    }

    f32 rate = al::calcNerveRate(this, mKeyMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::holdSe(this, "PgMove", nullptr);
    if (al::isGreaterEqualStep(this, mKeyMoveTime)) {
        al::nextKeyPose(mKeyPoseKeeper);
        al::setNerve(this, &NrvPetoAttackEnd);
        changeToWaitSensor();
    }
}

/** @brief Rises back up, shrinking the shadow and recentering the eyes. */
void Peto::exeAttackEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ReturnWait");
    }

    moveEyesToInitPos();
    if (mIsUseShadowMask) {
        al::setShadowMaskSize(this, "体影", mShadowMaskSize);
        if (al::isGreaterEqualStep(this, 9) && al::isLessStep(this, 14)) {
            mShadowMaskSize.x -= 47.8f;
            mShadowMaskSize.z -= 47.8f;
        }
    }

    if (al::isActionEnd(this)) {
        mEyeRotate = {0.0f, 0.0f, 0.0f};
        al::setNerve(this, &NrvPetoWait);
        if (mIsUseShadowMask) {
            mShadowMaskSize.set(mShadowMaskSizeInit);
            al::setShadowMaskSize(this, "体影", mShadowMaskSize);
        }
    }
}

/** @brief Moves the eyes a step back towards their initial direction. */
void Peto::moveEyesToInitPos() {
    if (al::getKeyPoseCount(mKeyPoseKeeper) == 1) {
        return;
    }

    f32 eyeRotateY = mEyeRotate.y;
    f32 eyeRotateZ = mEyeRotate.z;
    eyeRotateY = approachZero(eyeRotateY);
    eyeRotateZ = approachZero(eyeRotateZ);
    mEyeRotate.y = eyeRotateY;
    mEyeRotate.z = eyeRotateZ;
}

/** @brief Stays frozen by the touch screen, then resumes waiting. */
void Peto::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        if (mDelay > 0) {
            al::setNerve(this, &NrvPetoStandby);
        } else {
            al::setNerve(this, &NrvPetoWait);
        }
    }
}

/** @brief Gets squashed and dies. */
void Peto::exePressDown() {
    if (al::isFirstStep(this)) {
        al::changeEnvTextureStamp(this);
        al::startAction(this, "PressDown");
    }

    rc::tryAppearItemPressDown(this, nullptr);
    if (al::isActionEnd(this)) {
        al::resetEnvTexture(this);
        kill();
    }
}

/** @brief Gets knocked away and dies. */
void Peto::exeBlowDown() {
    if (al::isFirstStep(this) && mIsUseShadowMask) {
        al::setShadowMaskSize(this, "体影", mShadowMaskSizeInit);
        al::setShadowDropDir(this, -sead::Vector3f::ey);
        al::setShadowDropLength(this, 430.0f, "体影");
    }

    if (al::updateNerveState(this)) {
        al::appearItem(this);
        kill();
    }
}

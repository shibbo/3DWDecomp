#include "Enemy/KuriboClimbRail.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Enemy/ActorMicRumbler.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/KuriboClimbParam.hpp"
#include "Enemy/KuriboClimbStateBodyAttack.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ActorItemKeeper.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define KURIBO_CLIMB_RAIL_NERVE_SHARED_DECL(Action, ExeFunc)                                       \
    class KuriboClimbRailNrv##Action : public al::Nerve {                                          \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<KuriboClimbRail>())->exe##ExeFunc();                               \
        }                                                                                          \
    };

namespace {
NERVE_DECL(KuriboClimbRail, Walk)
NERVE_DECL(KuriboClimbRail, BodyAttack)
NERVE_DECL(KuriboClimbRail, BlowDown)
NERVE_DECL(KuriboClimbRail, SupportFreeze)
NERVE_DECL(KuriboClimbRail, PressDown)
KURIBO_CLIMB_RAIL_NERVE_SHARED_DECL(TurnToBodyAttack, Turn)
NERVE_DECL(KuriboClimbRail, BodyAttackReady)
NERVE_DECL(KuriboClimbRail, Turn)
NERVE_DECL(KuriboClimbRail, RecoverTurn)
NERVE_DECL(KuriboClimbRail, Recover)
// Non-const nerve objects: the game keeps them in .data in this order.
KuriboClimbRailNrvWalk NrvKuriboClimbRailWalk;
KuriboClimbRailNrvBodyAttack NrvKuriboClimbRailBodyAttack;
KuriboClimbRailNrvBlowDown NrvKuriboClimbRailBlowDown;
KuriboClimbRailNrvSupportFreeze NrvKuriboClimbRailSupportFreeze;
KuriboClimbRailNrvPressDown NrvKuriboClimbRailPressDown;
KuriboClimbRailNrvTurnToBodyAttack NrvKuriboClimbRailTurnToBodyAttack;
KuriboClimbRailNrvBodyAttackReady NrvKuriboClimbRailBodyAttackReady;
KuriboClimbRailNrvTurn NrvKuriboClimbRailTurn;
KuriboClimbRailNrvRecoverTurn NrvKuriboClimbRailRecoverTurn;
KuriboClimbRailNrvRecover NrvKuriboClimbRailRecover;

/** @brief Settings of one tail joint spring. */
struct TailSpringParam {
    sead::Vector3f mChildLocalPos;
    f32 mStability;
    f32 mFriction;
    f32 mLimitDegree;
};

KuriboClimbParam sParam;
EnemyStateBlowDownParam sBlowDownParam(false);
TailSpringParam sTailSpringParams[] = {
    {sead::Vector3f(0.0f, -200.0f, 0.0f), 0.04f, 0.9f, 30.0f},
    {sead::Vector3f(0.0f, -200.0f, 0.0f), 0.025f, 0.9f, 40.0f},
};

/**
 * @brief Creates the spring controller of one tail joint.
 * @param pHost Climbing Goomba.
 * @param pJointName Name of the tail joint.
 * @param rParam Spring settings.
 */
inline void initTailSpring(al::LiveActor* pHost, const char* pJointName,
                           const TailSpringParam& rParam) {
    al::JointSpringController* spring = al::initJointSpringController(pHost, pJointName);
    spring->setChildLocalPos(rParam.mChildLocalPos);
    spring->setStability(rParam.mStability);
    spring->setFriction(rParam.mFriction);
    spring->setLimitDegree(rParam.mLimitDegree);
}

/**
 * @brief Finds the ground below the current rail position.
 * @param pHost Climbing Goomba.
 * @param pHitPos Output hit position (unchanged when nothing is hit).
 * @param pTriangle Output hit polygon, or nullptr.
 * @return Whether a polygon was hit.
 */
inline bool findGroundUnderRail(al::LiveActor* pHost, sead::Vector3f* pHitPos,
                                al::Triangle* pTriangle) {
    sead::Vector3f start = al::getRailPos(pHost);
    start.y += 150.0f;
    return alCollisionUtil::getFirstPolyOnArrow(pHost, pHitPos, pTriangle, start,
                                                sead::Vector3f::ey * -300.0f, nullptr, nullptr);
}

/**
 * @brief Calculates the horizontal length of a vector.
 * @param rVec Vector.
 * @return Length on the XZ plane.
 */
inline f32 calcLengthH(const sead::Vector3f& rVec) {
    return sead::Mathf::sqrt(rVec.x * rVec.x + rVec.z * rVec.z);
}

/**
 * @brief Calculates the horizontal distance between two points.
 * @param rA First point.
 * @param rB Second point.
 * @return Distance on the XZ plane.
 */
inline f32 calcDistanceH(const sead::Vector3f& rA, const sead::Vector3f& rB) {
    sead::Vector3f diff = rA - rB;
    return calcLengthH(diff);
}
}  // namespace

/** @brief Drops the item in front of the Goomba, crediting the attacker. */
inline void KuriboClimbRail::appearItemToAttacker() {
    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, this);
    al::appearItemTiming(this, mItemType, al::getTrans(this), front,
                         getActorItemKeeper()->getAttackerSensor(), false);
}

/**
 * @brief Constructs a KuriboClimbRail.
 * @param pName Actor name.
 */
KuriboClimbRail::KuriboClimbRail(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, states, rail pose, tail springs and the mic rumbler.
 * @param rInfo Placement info of the actor.
 */
void KuriboClimbRail::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "KuriboClimb", nullptr);
    al::initNerve(this, &NrvKuriboClimbRailWalk, 3);
    mStateBodyAttack = new KuriboClimbStateBodyAttack(this);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, sParam.getParamSupportFreeze());
    al::initNerveState(this, mStateBodyAttack, &NrvKuriboClimbRailBodyAttack,
                       "[state]ボディアタック");
    al::initNerveState(this, mStateBlowDown, &NrvKuriboClimbRailBlowDown, "吹き飛び死亡");
    al::initNerveState(this, mStateSupportFreeze, &NrvKuriboClimbRailSupportFreeze, "DRC拘束");
    al::tryGetStringArg(&mItemType, rInfo, "ItemType");
    al::setSyncRailToStart(this);

    sead::Vector3f railDir = sead::Vector3f::ez;
    al::calcRailMoveDir(&railDir, this);
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatFrontUp(&quat, railDir, sead::Vector3f::ey);
    al::updatePoseQuat(this, quat);
    al::setRailClippingInfo(&mClippingPos, this, 100.0f, 100.0f);
    al::tryGetArg(&mIsPlacementSlope, rInfo, "IsPlacementSlope");
    al::tryGetArg(&mIsControlShadowLength, rInfo, "IsControlShadowLength");

    bool isBodyAttackImmediately = false;
    if (al::tryGetArg(&isBodyAttackImmediately, rInfo, "IsBodyAttackImmediately") &&
        isBodyAttackImmediately) {
        al::setNerve(this, &NrvKuriboClimbRailBodyAttack);
    }

    al::initJointControllerKeeper(this, 2);
    initTailSpring(this, "Tail1", sTailSpringParams[0]);
    initTailSpring(this, "Tail2", sTailSpringParams[1]);
    mMicRumbler = new ActorMicRumbler(this, nullptr);
    makeActorAppeared();
}

/** @brief Snaps the Goomba onto the ground below its rail and remembers that ground position. */
void KuriboClimbRail::initAfterPlacement() {
    findGroundUnderRail(this, al::getTransPtr(this), nullptr);
    mGroundPos.e = al::getTrans(this).e;
}

/** @brief Handles kill areas, the mic rumbler and the shadow length. */
void KuriboClimbRail::control() {
    if (EnemyStateUtil::tryKillByAreaOrMaterialCode(this)) {
        return;
    }

    if (!al::isNerve(this, &NrvKuriboClimbRailPressDown) &&
        !al::isNerve(this, &NrvKuriboClimbRailBlowDown) &&
        !al::isNerve(this, &NrvKuriboClimbRailSupportFreeze)) {
        mMicRumbler->update();
    }

    if (!mIsControlShadowLength) {
        return;
    }

    sead::Vector3f rootPos = {0.0f, 0.0f, 0.0f};
    al::calcJointPos(&rootPos, this, "JointRoot");
    f32 length = sead::Mathf::abs(rootPos.y - mGroundPos.y) + 100.0f;
    al::setShadowDropLength(this, sead::Mathf::min(length, 500.0f), "JointRoot");
}

/** @brief Plays the death hit reaction and kills the Goomba. */
void KuriboClimbRail::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
}

/**
 * @brief Pushes other enemies and attacks players.
 * @param pSelf Sensor of the Goomba.
 * @param pOther Sensor that was hit.
 */
void KuriboClimbRail::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvKuriboClimbRailPressDown) ||
        al::isNerve(this, &NrvKuriboClimbRailBlowDown)) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorEnemyBody(pOther)) {
        al::sendMsgPush(pSelf, pOther);
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
    }
}

/**
 * @brief Handles pushes, Piranha Plant bites, stomps and blow downs.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the Goomba.
 * @return Whether the message was handled.
 */
bool KuriboClimbRail::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvKuriboClimbRailPressDown) ||
        al::isNerve(this, &NrvKuriboClimbRailBlowDown)) {
        return false;
    }

    if (!al::isNerve(this, &NrvKuriboClimbRailBodyAttack)) {
        if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf,
                                                sParam.getPushPower())) {
            return true;
        }

        if (rc::isMsgPackunEatStart(pMsg)) {
            return true;
        }

        if (rc::isMsgPackunEat(pMsg)) {
            rc::addScore(this, pOther, 100.0f, 0);
            kill();
            return true;
        }
    }

    if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this,
                                                        &NrvKuriboClimbRailPressDown, true)) {
        if (al::isMsgPlayerObjHipDropAll(pMsg)) {
            mHipDropSensor = pOther;
        }
    } else if (!EnemyStateUtil::tryRequestBlowDownAndNextNerve(
                   pMsg, pOther, pSelf, mStateBlowDown, &NrvKuriboClimbRailBlowDown, true)) {
        return false;
    }

    rc::addScoreCombo(this, pOther, pMsg, 100.0f);
    return true;
}

/**
 * @brief Freezes the Goomba when it is touched on the screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool KuriboClimbRail::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                            al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvKuriboClimbRailPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvKuriboClimbRailBlowDown)) {
        return false;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvKuriboClimbRailSupportFreeze)) {
        mNerveBeforeFreeze = getNerveKeeper()->getCurrentNerve();
        al::setNerve(this, &NrvKuriboClimbRailSupportFreeze);
    }

    return true;
}

/** @brief Turns around on the wall, then walks on or body attacks after reaching the rail end. */
void KuriboClimbRail::exeTurn() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Walk");
    }

    sead::Vector3f front = sead::Vector3f::ez;
    sead::Vector3f up = sead::Vector3f::ey;
    sead::Vector3f railDir = sead::Vector3f::ez;
    al::calcFrontDir(&front, this);
    al::calcUpDir(&up, this);
    al::calcRailMoveDir(&railDir, this);
    railDir.y = 0.0f;
    al::normalizeOrDirZ(&railDir);

    sead::Vector3f side = railDir.cross(sead::Vector3f::ey);
    sead::Vector3f targetFront = up.cross(side);
    if (al::isReverseDirection(front, targetFront, 0.01f)) {
        al::rotateVectorDegree(&front, front, up, 0.1f);
    }

    sead::Vector3f newFront = sead::Vector3f::ez;
    f32 cosTurn = sead::Mathf::cos(sead::Mathf::deg2rad(sParam.getParamWander()->mTurnRate));
    bool isTurnEnd = al::turnVecToVecCosOnPlane(&newFront, front, targetFront, up, cosTurn);
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatFrontUp(&quat, newFront, up);
    al::updatePoseQuat(this, quat);
    if (!isTurnEnd) {
        return;
    }

    if (al::isNerve(this, &NrvKuriboClimbRailTurnToBodyAttack)) {
        if (mBodyAttackController != nullptr) {
            al::setNerve(this, &NrvKuriboClimbRailBodyAttackReady);
        } else {
            al::setNerve(this, &NrvKuriboClimbRailBodyAttack);
        }
    } else {
        al::setNerve(this, &NrvKuriboClimbRailWalk);
    }
}

/** @brief Walks along the rail, slowing down on steep slopes and sticking to the ground. */
void KuriboClimbRail::exeWalk() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Walk");
        al::offCollide(this);
        mWalkSpeed = 0.0f;
    }

    f32 walkSpeed = mWalkSpeed;
    f32 accel = sParam.getParamWander()->mAccel;
    f32 turnRate = sParam.getParamWander()->mTurnRate;
    sead::Vector3f up = {0.0f, 0.0f, 0.0f};
    sead::Vector3f side = {0.0f, 0.0f, 0.0f};
    al::calcUpDir(&up, this);
    al::calcSideDir(&side, this);
    f32 slopeAngle = al::calcAngleOnPlaneDegree(up, sead::Vector3f::ey, side);
    f32 speed = (walkSpeed + accel) * sParam.getParamBase()->mGroundFriction;
    f32 moveSpeed = speed + speed * (al::normalize(slopeAngle, 0.0f, 45.0f) * -0.25f);
    al::moveRail(this, moveSpeed);
    al::turnToRailDir(this, turnRate);

    sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
    al::Triangle triangle;
    if (findGroundUnderRail(this, &hitPos, &triangle)) {
        sead::Vector3f normal = *triangle.getNormal(0);
        f32 angle = al::calcAngleOnPlaneDegree(up, normal, side);
        f32 rotateAngle = angle > 0.0f ? sead::Mathf::clampMax(angle, 2.0f) :
                                         sead::Mathf::clampMin(angle, -2.0f);
        al::rotateVectorDegree(&up, up, side, rotateAngle);
        sead::Quatf quat;
        al::makeQuatSideUp(&quat, side, up);
        al::updatePoseQuat(this, quat);
        al::setTrans(this, hitPos);
    } else {
        al::moveRail(this, -moveSpeed);
        speed = -1.0f;
    }

    mWalkSpeed = speed;
    if (speed < 0.0f || al::isRailReachedGoal(this)) {
        al::reverseRail(this);
        al::setNerve(this, &NrvKuriboClimbRailTurnToBodyAttack);
    }
}

/** @brief Waits until the body attack is started from outside. */
void KuriboClimbRail::exeBodyAttackReady() {}

/** @brief Jumps off the wall at the player and goes back to the rail afterwards. */
void KuriboClimbRail::exeBodyAttack() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        _160 = 0;
        _164 = 0;
    }

    sead::Vector3f front = sead::Vector3f::ez;
    sead::Vector3f targetFront;
    sead::Vector3f side = sead::Vector3f::ex;
    al::calcFrontDir(&front, this);
    al::calcSideDir(&side, this);
    targetFront.setCross(side, sead::Vector3f::ey);
    if (!al::isNearDirection(front, targetFront, 0.01f)) {
        sead::Vector3f newFront = sead::Vector3f::ez;
        al::turnVecToVecDegree(&newFront, front, targetFront, 1.0f);
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatFrontUp(&quat, newFront, sead::Vector3f::ey);
        al::updatePoseQuat(this, quat);
    }

    if (mIsControlShadowLength) {
        findGroundUnderRail(this, &mGroundPos, nullptr);
    }

    al::moveRail(this, calcLengthH(al::getVelocity(this)));
    if (!al::updateNerveState(this)) {
        return;
    }

    if (mIsPlacementSlope) {
        al::reverseRail(this);
    }

    al::setRailPosToNearestPos(this, al::getTrans(this));
    if (calcDistanceH(al::getTrans(this), al::getRailPos(this)) < 10.0f) {
        if (mIsPlacementSlope) {
            al::setNerve(this, &NrvKuriboClimbRailTurn);
        } else {
            al::setNerve(this, &NrvKuriboClimbRailRecoverTurn);
        }
    } else {
        al::setNerve(this, &NrvKuriboClimbRailWalk);
    }
}

/** @brief Turns towards the rail after a body attack. */
void KuriboClimbRail::exeRecoverTurn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Walk");
    }

    sead::Vector3f front = sead::Vector3f::ez;
    al::calcFrontDir(&front, this);
    bool isTurnEnd = al::turnDirectionToTargetDegree(this, &front, al::getRailPos(this),
                                                     sParam.getParamWander()->mTurnRate);
    sead::Quatf quat = sead::Quatf::unit;
    al::makeQuatUpFront(&quat, sead::Vector3f::ey, front);
    al::updatePoseQuat(this, quat);
    if (isTurnEnd) {
        al::setNerve(this, &NrvKuriboClimbRailRecover);
    }
}

/** @brief Walks back to the rail after a body attack. */
void KuriboClimbRail::exeRecover() {
    if (al::isFirstStep(this)) {
        mWalkSpeed = 0.0f;
        al::tryStartActionIfNotPlaying(this, "Walk");
    }

    al::turnToTarget(this, al::getRailPos(this), sParam.getParamWander()->mTurnRate);
    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, this);
    al::addVelocityToDirection(this, front, sParam.getParamWander()->mAccel);
    WalkerStateFunction::calcPassiveMovement(this, sParam.getParamBase());
    f32 distance = calcDistanceH(al::getTrans(this), al::getRailPos(this));
    if (distance <= calcLengthH(al::getVelocity(this))) {
        mWalkSpeed = calcLengthH(al::getVelocity(this));
        al::setNerve(this, &NrvKuriboClimbRailWalk);
        al::setVelocityZero(this);
    }
}

/** @brief Gets stomped flat, drops its item and dies. */
void KuriboClimbRail::exePressDown() {
    bool isSuperBell = mItemType != nullptr && al::isEqualString(mItemType, "SuperBell");
    if (al::isFirstStep(this)) {
        al::startAction(this, "PressDown");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
        mMicRumbler->stopAndReset();
    }

    if (!isSuperBell && al::isStep(this, rc::getStepAppearItemPressDown())) {
        appearItemToAttacker();
    }

    if (mHipDropSensor != nullptr &&
        al::getSensorPos(mHipDropSensor).y < al::getTrans(this).y) {
        al::setTransY(this, al::getSensorPos(mHipDropSensor).y);
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    if (isSuperBell) {
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&front, this);
        al::appearItemTiming(this, mItemType, al::getTrans(this), front);
    }

    al::resetEnvTexture(this);
    kill();
}

/** @brief Gets blown away, drops its item and dies. */
void KuriboClimbRail::exeBlowDown() {
    if (al::isFirstStep(this)) {
        mMicRumbler->stopAndReset();
    }

    if (mIsControlShadowLength) {
        findGroundUnderRail(this, &mGroundPos, nullptr);
    }

    if (al::updateNerveState(this)) {
        appearItemToAttacker();
        kill();
    }
}

/** @brief Stays frozen by the touch screen, then goes back to what it was doing. */
void KuriboClimbRail::exeSupportFreeze() {
    if (!al::updateNerveState(this)) {
        return;
    }

    const al::Nerve* nerve = mNerveBeforeFreeze;
    if (nerve == &NrvKuriboClimbRailTurn || nerve == &NrvKuriboClimbRailBodyAttackReady ||
        nerve == &NrvKuriboClimbRailTurnToBodyAttack || nerve == &NrvKuriboClimbRailRecoverTurn ||
        nerve == &NrvKuriboClimbRailRecover) {
        al::setNerve(this, nerve);
        return;
    }

    if (nerve != &NrvKuriboClimbRailBodyAttack) {
        al::setNerve(this, &NrvKuriboClimbRailWalk);
        return;
    }

    al::setNerve(this, &NrvKuriboClimbRailBodyAttack);
    if (!mStateBodyAttack->isWait()) {
        mStateBodyAttack->requestResume();
    }
}

/** @brief Falls until it lands, then turns to walk along the rail again. */
void KuriboClimbRail::exeFall() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
    }

    al::addVelocityToGravity(this, sParam.getParamBase()->mGravity);
    al::scaleVelocity(this, sParam.getParamBase()->mAirFriction);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::offCollide(this);
        al::setVelocityZero(this);
        al::setNerve(this, &NrvKuriboClimbRailTurn);
    }
}

/**
 * @brief Checks whether the Goomba is ready to start a body attack.
 * @return Whether it waits for the body attack.
 */
bool KuriboClimbRail::isEnableBodyAttack() const {
    return al::isNerve(this, &NrvKuriboClimbRailBodyAttackReady);
}

/** @brief Starts the body attack. */
void KuriboClimbRail::startBodyAttack() {
    al::setNerve(this, &NrvKuriboClimbRailBodyAttack);
}

#include "Raidon/SkateShoes.hpp"

#include <prim/seadSafeString.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(SkateShoes, Wait)
NERVE_DECL(SkateShoes, TakeOff)
NERVE_DECL(SkateShoes, GetOn)
NERVE_DECL(SkateShoes, InstantTakeOff)
NERVE_DECL(SkateShoes, Accel)
NERVE_DECL(SkateShoes, Dash)
NERVE_DECL(SkateShoes, Ride)
NERVE_DECL(SkateShoes, Fall)
NERVE_DECL(SkateShoes, Jump)
NERVE_DECL(SkateShoes, Appear)
NERVES_MAKE_NOSTRUCT(SkateShoes, Appear)
NERVE_DECL(SkateShoes, Respawn)
NERVES_MAKE_NOSTRUCT(SkateShoes, Respawn)

// Non-const so that LLVM's global merging packs these nerves and the bind end parameter into one
// block, which the target addresses relative to the TakeOff nerve.
SkateShoesNrvWait NrvSkateShoesWait;
SkateShoesNrvTakeOff NrvSkateShoesTakeOff;
SkateShoesNrvGetOn NrvSkateShoesGetOn;
SkateShoesNrvInstantTakeOff NrvSkateShoesInstantTakeOff;
SkateShoesNrvAccel NrvSkateShoesAccel;
SkateShoesNrvDash NrvSkateShoesDash;
SkateShoesNrvRide NrvSkateShoesRide;
SkateShoesNrvFall NrvSkateShoesFall;
SkateShoesNrvJump NrvSkateShoesJump;

/// How the rider is thrown off when the skates are knocked off their feet.
PlayerBindEndParam sBindEndParamTakeOff = {{}, 0, 40, true, false, true, 0, -1.0f, 15, false, {}};

/// Offset of the rider from the ankle joint.
const sead::Vector3f cPuppetOffset(0.0f, -20.0f, 10.0f);

/// Offset of a mini rider from the ankle joint.
const sead::Vector3f cPuppetOffsetMini(0.0f, -0.0f, 10.0f);
}  // namespace

/** @param pName Actor name. */
SkateShoes::SkateShoes(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the model, the placement flags and the wait nerve.
 * @param rInfo Actor init info.
 */
void SkateShoes::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    al::initActorWithArchiveName(this, rInfo, "SkateShoes", mIsSingleMode ? "SM" : nullptr);
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalZRotator(this, &mRotateZ, "AllRoot");
    rc::setPlayerColorAnimDefault(this, "Color");
    al::tryGetArg(&mIsNoTakeOff, rInfo, "IsNoTakeOff");
    al::tryGetArg(&mIsEnableReset, rInfo, "IsEnableReset");
    al::tryGetArg(&mIsIgnoreInitialGravity, rInfo, "IsIgnoreInitialGravity");
    al::hideSilhouetteModel(this);
    al::initNerve(this, &NrvSkateShoesWait, 0);
    makeActorAppeared();

    if (mIsEnableReset) {
        mRespawnTrans = al::getTrans(this);
    }
}

/** @brief Attacks whatever the skates run into while ridden.
 * @param pSelf Own sensor.
 * @param pOther Sensor that was hit.
 */
void SkateShoes::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isEnableAttack()) {
        if (rc::sendMsgPlayerCheckpointTouch(pOther, pSelf)) {
            return;
        }

        rc::sendMsgSkateShoesAttack(pOther, pSelf);
    }

    if (mIsSingleMode && al::isSensorDoorKey(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/** @return Whether the skates are being ridden. */
bool SkateShoes::isEnableAttack() const {
    return al::isNerve(this, &NrvSkateShoesAccel) || al::isNerve(this, &NrvSkateShoesDash) ||
           al::isNerve(this, &NrvSkateShoesRide) || al::isNerve(this, &NrvSkateShoesFall) ||
           al::isNerve(this, &NrvSkateShoesJump);
}

/** @brief Handles binding the rider, attacks and pushes.
 * @param pMsg Received message.
 * @param pSelf Own sensor.
 * @param pOther Sending sensor.
 * @return Whether the message was handled.
 */
bool SkateShoes::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                            al::HitSensor* pOther) {
    if (rc::isMsgIsEnableExitStage(pMsg) || rc::isMsgIsEnableIslandWarp(pMsg)) {
        return onGround(4);
    }

    if (al::isMsgNekoPush(pMsg)) {
        al::pushAndAddVelocity(this, pSelf, pOther, 8.0f);
        return true;
    }

    if (mPuppet != nullptr) {
        if (rc::isMsgAskControlUserId(pMsg, rc::getPuppetSensor(mPuppet))) {
            return true;
        }

        if (rc::tryRelayRequestPlayerGetReactionMsg(pMsg, pSelf, rc::getPuppetSensor(mPuppet))) {
            return true;
        }
    }

    if ((al::isMsgEnemyAttack(pMsg) || al::isMsgLaserAttack(pMsg)) && isEnableAttack()) {
        rc::startPuppetSe(mPuppet, "PgItemTakenOff");
        if (mIsSingleMode) {
            al::startSe(this, "PgRemoved");
        }

        al::calcDirBetweenSensorsH(al::getFrontPtr(this), pOther, pSelf);
        takeOffPuppet();
        rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamTakeOff);
        al::setNerve(this, &NrvSkateShoesTakeOff);
        return true;
    }

    if (rc::isMsgDashPanel(pMsg) && isEnableAttack()) {
        rc::tryGetDashPanelTime(&mDashPanelTime, pMsg);
        return true;
    }

    if (mActorSceneInfo->isSingleMode) {
        if (al::isMsgPush(pMsg) &&
            al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pSelf, pOther, 8.0f)) {
            return true;
        }

        if (al::isMsgDisasterSpikePush(pMsg)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pSelf, pOther);
            al::addVelocityToDirection(this, dir, 10.0f);
        }
    }

    if (al::isMsgBindStart(pMsg)) {
        return al::isNerve(this, &NrvSkateShoesWait);
    }

    if (al::isMsgBindInit(pMsg)) {
        al::invalidateClipping(this);
        mPuppet = rc::startPuppet(pOther, pSelf);
        rc::hidePuppetShadow(mPuppet);
        al::getFrontPtr(this)->set(rc::getPuppetFrontVec(mPuppet));
        al::sendMsgHoldCancel(pSelf, pOther);
        al::setNerve(this, &NrvSkateShoesGetOn);
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mPuppet = nullptr;
        if (al::isMsgBindCancelForGoal(pMsg)) {
            mIsCancelForGoal = true;
            al::setVelocityZero(this);
            al::setNerve(this, &NrvSkateShoesWait);
            return true;
        }

        if (al::isMsgBindCancelForWarp(pMsg)) {
            al::setVelocityZero(this);
            al::setNerve(this, &NrvSkateShoesInstantTakeOff);
            return true;
        }

        if (mIsSingleMode) {
            al::startSe(this, "PgRemoved");
        }

        al::setNerve(this, &NrvSkateShoesTakeOff);
        return true;
    }

    if (isEnableWind(pMsg)) {
        sead::Vector3f windPower = sead::Vector3f::zero;
        if (rc::tryGetWindPower(&windPower, pMsg)) {
            al::addVelocity(this, windPower * 0.8f);
            return true;
        }
    }

    return false;
}

/**
 * @param checkFrame Number of frames the skates may have left the ground.
 * @return Whether the skates are on the ground.
 */
bool SkateShoes::onGround(u32 checkFrame) const {
    if (mIsSingleMode) {
        return al::isOnGround(this, checkFrame, 0.001f);
    }

    return al::isOnGround(this, checkFrame, 0.0f);
}

/** @brief Places the rider on the ankle joint of the skates. */
void SkateShoes::setPuppetQT() {
    sead::Matrix34f mtx;
    al::addTransMtxLocalOffset(&mtx, *al::getJointMtxPtr(this, "Ancle"),
                               rc::isPlayerMini(rc::getPuppetSensor(mPuppet)) ? cPuppetOffsetMini :
                                                                                cPuppetOffset);
    rc::setPuppetMtx(mPuppet, &mtx);
}

/** @brief Throws the rider backwards off the skates. */
inline void SkateShoes::takeOffPuppet() {
    sead::Vector3f velocity;
    al::calcFrontDir(&velocity, this);
    velocity.x *= -7.5f;
    velocity.z *= -7.5f;
    velocity.y = 15.0f;
    setPuppetQT();
    rc::setPuppetVelocity(mPuppet, velocity);
}

/**
 * @param pMsg Received message.
 * @return Whether the skates are pushed by a wind message.
 */
bool SkateShoes::isEnableWind(const al::SensorMsg* pMsg) const {
    if (rc::isMsgByugoWind(pMsg) && mPuppet != nullptr) {
        if (rc::isPlayerRaccoonDogWhite(rc::getPuppetSensor(mPuppet))) {
            return false;
        }

        if (rc::isPlayerClimbWhite(rc::getPuppetSensor(mPuppet))) {
            return false;
        }

        if (rc::isPlayerInvincible(this, rc::getPuppetSensor(mPuppet))) {
            return false;
        }
    }

    return isEnableAttack();
}

/** @brief Lets the touch assist grab the skates while they wait.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Screen point target.
 * @return Whether the message was handled.
 */
bool SkateShoes::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssist(pMsg) && al::isNerve(this, &NrvSkateShoesWait)) {
        return true;
    }

    return false;
}

/** @brief Updates the animation and keeps the rider on the skates. */
void SkateShoes::calcAnim() {
    al::LiveActor::calcAnim();
    if (al::isDead(this)) {
        return;
    }

    if (al::isNerve(this, &NrvSkateShoesGetOn) || isEnableAttack()) {
        setPuppetQT();
    }
}

/** @brief Counts down the dash panel boost and throws the rider off in water. */
void SkateShoes::control() {
    if (mDashPanelTime > 0) {
        mDashPanelTime--;
    }

    if (isEnableAttack() && rc::isInWaterAreaNoSink(this, al::getTrans(this))) {
        rc::startPuppetSe(mPuppet, "PgItemTakenOff");
        takeOffPuppet();
        rc::changePuppetMaterialCode(mPuppet, "Water");
        rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamTakeOff);
        al::startHitReaction(this, "WaterHit");
        al::setNerve(this, &NrvSkateShoesTakeOff);
    }
}

/** @return Extra acceleration while boosted by a dash panel. */
f32 SkateShoes::calcDashAccel() const {
    return mDashPanelTime > 0 ? 2.0f : 0.0f;
}

/** @return Extra turn speed while boosted by a dash panel. */
f32 SkateShoes::calcDashTurnSpeed() const {
    return mDashPanelTime > 0 ? 1.5f : 0.0f;
}

/** @brief Makes the skates pop out of an object.
 * @param rTrans Appear position.
 * @param rVelocity Initial velocity.
 */
void SkateShoes::appearFromObject(const sead::Vector3f& rTrans, const sead::Vector3f& rVelocity) {
    al::setNerve(this, &NrvSkateShoesAppear);
    al::setVelocity(this, rVelocity);
    al::resetPosition(this, rTrans, false);
    appear();
}

/** @brief Drops the rider and kills the skates. */
void SkateShoes::reset() {
    if (mPuppet != nullptr) {
        rc::endBindAndPuppetNull(&mPuppet, nullptr);
    }

    rc::setPlayerColorAnimDefault(this, "Color");
    al::hideSilhouetteModel(this);
    makeActorDead();
}

/** @brief Falls after popping out of an object until the skates land. */
void SkateShoes::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }

    bool isOnGround = onGround(0);
    updateVelocity(isOnGround);
    if (isOnGround) {
        al::setVelocityZero(this);
        al::startHitReaction(this, "出現後着地");
        al::setNerve(this, &NrvSkateShoesWait);
    }
}

/** @brief Applies gravity, friction and collision rebound.
 * @param isOnGround Whether the skates are on the ground.
 */
void SkateShoes::updateVelocity(bool isOnGround) {
    al::addVelocityToGravity(this, 1.5f);
    al::scaleVelocityHV(this, 0.94f, 0.995f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
}

/** @brief Waits for a rider. */
void SkateShoes::exeWait() {
    if (al::isFirstStep(this) && !mIsCancelForGoal) {
        if (mIsRespawned) {
            mIsRespawned = false;
            al::showModelIfHide(this);
            al::emitEffect(this, "Appear", nullptr);
            al::startSe(this, "PgReappear");
        }

        if (mIsEnableReset) {
            al::validateHitSensors(this);
        }

        if (!al::isActionPlaying(this, "ItemWait")) {
            al::startAction(this, "ItemWait");
        }
    }

    if (!mIsCancelForGoal && !mIsIgnoreInitialGravity) {
        updateVelocity(onGround(4));
    }

    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        mIsCancelForGoal = false;
    }
}

/** @brief Puts the rider on the skates. */
void SkateShoes::exeGetOn() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "GetOn");
        rc::startPuppetSe(mPuppet, "PgSkateGetOn");
        al::showSilhouetteModel(this);
        mSpeed = 1.2f;
    }

    if (al::isStep(this, 5)) {
        rc::setPlayerColorAnimBySensor(this, rc::getPuppetSensor(mPuppet), "Color");
    }

    updateTurn(6.0f);
    updateVelocity(onGround(4));
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkateShoesRide);
    }
}

/** @brief Steers the skates with the rider's stick and leans them into the turn.
 * @param turnSpeed Turn speed in degrees per frame.
 */
void SkateShoes::updateTurn(f32 turnSpeed) {
    sead::Vector3f side;
    al::calcSideDir(&side, this);
    sead::Vector3f stick = rc::getPuppetStickWorldWithoutSnap(mPuppet);
    f32 targetRotate = 0.0f;
    if (!al::normalizeOrZero(&stick)) {
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        f32 rotate = al::lerpValue(front.dot(stick), 0.7f, 1.0f, 25.0f, 0.0f);
        targetRotate = stick.dot(side) > 0.0f ? -rotate : rotate;
    }

    mRotateZ = al::converge(mRotateZ, targetRotate, 1.0f);
    al::turnDirectionDegree(this, al::getFrontPtr(this), stick, calcDashTurnSpeed() + turnSpeed);
}

/** @brief Accelerates from the rider's dash. */
void SkateShoes::exeAccel() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Accel");
        rc::startPuppetSe(mPuppet, "PgSkateAccel");
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(3.0f);
    mSpeed = al::calcNerveValue(this, 4, 0.0f, 1.6f);
    bool isOnGround = onGround(4);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(isOnGround);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkateShoesRide);
    }
}

/**
 * @brief Ends the ride when the skates fall into a death area or crash into a wall head-on.
 * @return Whether the ride ended.
 */
bool SkateShoes::tryWallHitEnd() {
    if (rc::isInDeathArea(this)) {
        rc::startPuppetAction(mPuppet, "Fall");
        rc::setPuppetVelocity(mPuppet, al::getVelocity(this));
        rc::endBindAndPuppetNull(&mPuppet, nullptr);
        al::startHitReactionBreak(this);
        kill();
        return true;
    }

    if (!al::isCollidedWallVelocity(this)) {
        return false;
    }

    al::sendMsgBallAttackCollide(al::tryGetCollidedWallSensor(this), al::getHitSensor(this, "Body"));
    sead::Vector3f wallNormal = al::getCollidedWallNormal(this);
    if (wallNormal.dot(al::getFront(this)) >= -0.707f) {
        return false;
    }

    mIsInKeepSkateShoeArea =
        rc::isInAreaObj(this, rc::AreaObjType::KeepSkateShoeArea, al::getTrans(this));
    if (mIsInKeepSkateShoeArea || mIsNoTakeOff) {
        return false;
    }

    al::startHitReaction(this, "壁ヒット");
    rc::startPuppetSe(mPuppet, "PgItemTakenOff");
    wallNormal.y = 0.0f;
    if (!al::normalizeOrZero(&wallNormal)) {
        al::getFrontPtr(this)->set(-wallNormal.x, -wallNormal.y, -wallNormal.z);
    }

    sead::Vector3f velocity;
    al::calcFrontDir(&velocity, this);
    velocity.x *= -7.5f;
    velocity.z *= -7.5f;
    velocity.y = 15.0f;
    setPuppetQT();
    rc::setPuppetVelocity(mPuppet, velocity);
    rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamTakeOff);
    al::setNerve(this, &NrvSkateShoesTakeOff);
    return true;
}

/** @brief Runs at dash panel speed. */
void SkateShoes::exeDash() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Accel");
        rc::startPuppetSe(mPuppet, "PgSkateAccel");
        mSpeed = 1.6f;
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(3.0f);
    bool isOnGround = onGround(4);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(isOnGround);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkateShoesRide);
    }
}

/** @brief Rolls along with the rider, who can jump or dash. */
void SkateShoes::exeRide() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Run");
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(3.0f);
    mSpeed -= 0.007f;
    f32 maxSpeed = rc::isPuppetHoldDashButton(mPuppet) ? 1.6f : 1.2f;
    if (mSpeed < maxSpeed) {
        mSpeed += 0.05f;
        if (mSpeed > maxSpeed) {
            mSpeed = maxSpeed;
        }
    }

    bool isOnGround = onGround(4);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(isOnGround);
    if (!al::isOnGroundNoVelocity(this, 12)) {
        al::setNerve(this, &NrvSkateShoesFall);
        return;
    }

    if (al::isCollidedGround(this)) {
        al::sendMsgPlayerFloorTouchToColliderGround(this, rc::getPuppetSensor(mPuppet));
    }

    if (isOnGround) {
        if (rc::isPuppetTrigJumpButton(mPuppet)) {
            al::getVelocityPtr(this)->y = 30.0f;
            al::setNerve(this, &NrvSkateShoesJump);
            return;
        }

        if (rc::isPuppetTrigDashButton(mPuppet)) {
            al::setNerve(this, &NrvSkateShoesAccel);
            return;
        }
    }

    if (onGround(10)) {
        f32 speedH = al::calcSpeedH(this);
        al::holdSeWithParam(this, "PgMove", speedH);
        al::holdSeWithParam(this, "PgMoveCurve", mRotateZ * (speedH / 35.0f + 0.3f));
    }
}

/** @brief Falls with the rider until the skates land. */
void SkateShoes::exeFall() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Fall");
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(6.0f);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(false);
    if (onGround(0)) {
        al::startHitReaction(this, "着地");
        al::setNerve(this, &NrvSkateShoesRide);
    }
}

/** @brief Jumps with the rider until the skates land. */
void SkateShoes::exeJump() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Jump");
        rc::startPuppetSe(mPuppet, "PgSkateJump");
    }

    if (al::isCollidedCeilingVelocity(this)) {
        al::sendMsgPlayerUpperPunchToColliderCeiling(this, rc::getPuppetSensor(mPuppet));
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(6.0f);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(false);
    if (onGround(0)) {
        al::startHitReaction(this, "着地");
        al::setNerve(this, &NrvSkateShoesRide);
    }
}

/** @brief Blows the empty skates away, then respawns or kills them. */
void SkateShoes::exeTakeOff() {
    if (al::isFirstStep(this)) {
        al::hideSilhouetteModel(this);
        al::startAction(this, "BlowDown");
        al::setVelocity(this, sead::Vector3f(0.0f, 35.0f, 0.0f));
    }

    updateVelocity(false);
    if (al::isActionEnd(this)) {
        al::startHitReactionBreak(this);
        if (mIsEnableReset) {
            al::setVelocity(this, sead::Vector3f::zero);
            al::setNerve(this, &NrvSkateShoesRespawn);
        } else {
            kill();
        }
    }
}

/** @brief Removes the skates right away, then respawns or kills them. */
void SkateShoes::exeInstantTakeOff() {
    if (al::isFirstStep(this)) {
        al::hideSilhouetteModel(this);
        al::setVelocity(this, sead::Vector3f::zero);
    }

    if (mIsEnableReset) {
        al::setNerve(this, &NrvSkateShoesRespawn);
    } else {
        kill();
    }
}

/** @brief Hides the skates for a while, then puts them back at their respawn position. */
void SkateShoes::exeRespawn() {
    if (al::isFirstStep(this)) {
        mRotateZ = 0.0f;
        if (mPuppet != nullptr) {
            rc::endBindAndPuppetNull(&mPuppet, nullptr);
        }

        rc::setPlayerColorAnimDefault(this, "Color");
        al::offCollide(this);
        al::hideModelIfShow(this);
        al::invalidateHitSensors(this);
        al::invalidateClipping(this);
        al::startAction(this, "DummyWait");
    }

    if (al::isGreaterStep(this, 30)) {
        mIsRespawned = true;
        al::resetPosition(this, mRespawnTrans, false);
        al::setVelocity(this, sead::Vector3f::zero);
        al::setNerve(this, &NrvSkateShoesWait);
        al::onCollide(this);
        al::validateClipping(this);
    }
}

/** @return Whether the skates were hidden; they stay visible while ridden. */
bool SkateShoes::hideActor() {
    if (mPuppet != nullptr) {
        return false;
    }

    return al::LiveActor::hideActor();
}

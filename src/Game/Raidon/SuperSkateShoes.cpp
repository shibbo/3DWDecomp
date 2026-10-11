#include "Raidon/SuperSkateShoes.hpp"

#include <math/seadMatrix.h>
#include <prim/seadSafeString.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/SuperSkateRail.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(SuperSkateShoes, Wait)
NERVE_DECL(SuperSkateShoes, TakeOff)
NERVE_DECL(SuperSkateShoes, GetOn)
NERVE_DECL(SuperSkateShoes, Accel)
NERVE_DECL(SuperSkateShoes, Dash)
NERVE_DECL(SuperSkateShoes, Ride)
NERVE_DECL(SuperSkateShoes, Fall)
NERVE_DECL(SuperSkateShoes, Jump)
NERVE_DECL(SuperSkateShoes, Grind)
NERVE_DECL(SuperSkateShoes, Appear)
NERVES_MAKE_NOSTRUCT(SuperSkateShoes, Appear)
NERVE_DECL(SuperSkateShoes, Respawn)
NERVES_MAKE_NOSTRUCT(SuperSkateShoes, Respawn)

// Non-const so that LLVM's global merging packs these nerves and the bind end parameter into one
// block, which the target addresses relative to the TakeOff nerve.
SuperSkateShoesNrvWait NrvSuperSkateShoesWait;
SuperSkateShoesNrvTakeOff NrvSuperSkateShoesTakeOff;
SuperSkateShoesNrvGetOn NrvSuperSkateShoesGetOn;
SuperSkateShoesNrvAccel NrvSuperSkateShoesAccel;
SuperSkateShoesNrvDash NrvSuperSkateShoesDash;
SuperSkateShoesNrvRide NrvSuperSkateShoesRide;
SuperSkateShoesNrvFall NrvSuperSkateShoesFall;
SuperSkateShoesNrvJump NrvSuperSkateShoesJump;
SuperSkateShoesNrvGrind NrvSuperSkateShoesGrind;

/// How the rider is thrown off when the skates are knocked off their feet.
PlayerBindEndParam sBindEndParamTakeOff = {{}, 0, 40, true, false, true, 0, -1.0f, 15, false, {}};

/// Offset of the rider from the ankle joint.
const sead::Vector3f cPuppetOffset(0.0f, -20.0f, 10.0f);

/// Offset of a mini rider from the ankle joint.
const sead::Vector3f cPuppetOffsetMini(0.0f, -0.0f, 10.0f);

/// Turns the rail direction into the facing direction while grinding (sideways, slightly back).
const sead::Matrix33f cGrindFrontMtx(-0.44807363f, 0.0f, 0.89399666f,  //
                                     -0.0f, 1.0f, 0.0f,                 //
                                     -0.89399666f, -0.0f, -0.44807363f);
}  // namespace

/** @param pName Actor name. */
SuperSkateShoes::SuperSkateShoes(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the model, the placement parameters and the wait nerve.
 * @param rInfo Actor init info.
 */
void SuperSkateShoes::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "SuperSkateShoes", nullptr);
    al::initJointControllerKeeper(this, 1);
    al::initJointLocalZRotator(this, &mRotateZ, "AllRoot");
    rc::setPlayerColorAnimDefault(this, "Color");
    al::tryGetArg(&mIsNoTakeOff, rInfo, "IsNoTakeOff");
    al::tryGetArg(&mSpeedScale, rInfo, "SpeedScale");
    al::hideSilhouetteModel(this);
    al::initNerve(this, &NrvSuperSkateShoesWait, 0);
    mRespawnTrans = al::getTrans(this);
    makeActorAppeared();
}

/** @brief Attacks whatever the skates run into while ridden.
 * @param pSelf Own sensor.
 * @param pOther Sensor that was hit.
 */
void SuperSkateShoes::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!isEnableAttack()) {
        return;
    }

    if (rc::sendMsgPlayerCheckpointTouch(pOther, pSelf)) {
        return;
    }

    rc::sendMsgSkateShoesAttack(pOther, pSelf);
}

/** @return Whether the skates are being ridden. */
bool SuperSkateShoes::isEnableAttack() const {
    return isRolling() || isGrinding();
}

/** @brief Handles binding the rider, attacks, dash panels and wind.
 * @param pMsg Received message.
 * @param pSelf Own sensor.
 * @param pOther Sending sensor.
 * @return Whether the message was handled.
 */
bool SuperSkateShoes::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                 al::HitSensor* pOther) {
    if (rc::isMsgIsEnableExitStage(pMsg) || rc::isMsgIsEnableIslandWarp(pMsg)) {
        return al::isOnGround(this, 4, 0.0f);
    }

    if (mPuppet != nullptr) {
        if (rc::isMsgAskControlUserId(pMsg, rc::getPuppetSensor(mPuppet))) {
            return true;
        }

        if (rc::tryRelayRequestPlayerGetReactionMsg(pMsg, pSelf, rc::getPuppetSensor(mPuppet))) {
            return true;
        }
    }

    if (al::isMsgEnemyAttack(pMsg) && isEnableAttack()) {
        rc::startPuppetSe(mPuppet, "PgItemTakenOff");
        al::calcDirBetweenSensorsH(al::getFrontPtr(this), pOther, pSelf);
        takeOffPuppet();
        rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamTakeOff);
        al::setNerve(this, &NrvSuperSkateShoesTakeOff);
        return true;
    }

    if (rc::isMsgDashPanel(pMsg) && isEnableAttack()) {
        rc::tryGetDashPanelTime(&mDashPanelTime, pMsg);
        return true;
    }

    if (al::isMsgBindStart(pMsg)) {
        return al::isNerve(this, &NrvSuperSkateShoesWait);
    }

    if (al::isMsgBindInit(pMsg)) {
        al::invalidateClipping(this);
        mPuppet = rc::startPuppet(pOther, pSelf);
        rc::hidePuppetShadow(mPuppet);
        al::getFrontPtr(this)->set(rc::getPuppetFrontVec(mPuppet));
        al::sendMsgHoldCancel(pSelf, pOther);
        al::setNerve(this, &NrvSuperSkateShoesGetOn);
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mPuppet = nullptr;
        al::setNerve(this, &NrvSuperSkateShoesTakeOff);
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

// Defined after receiveMsg so the TakeOff nerve is emitted (and merged) right after Wait.

/** @return Whether the skates roll on the ground or in the air with a rider. */
inline bool SuperSkateShoes::isRolling() const {
    return al::isNerve(this, &NrvSuperSkateShoesAccel) ||
           al::isNerve(this, &NrvSuperSkateShoesDash) ||
           al::isNerve(this, &NrvSuperSkateShoesRide) ||
           al::isNerve(this, &NrvSuperSkateShoesFall) || al::isNerve(this, &NrvSuperSkateShoesJump);
}

/** @return Whether the skates slide along a rail. */
inline bool SuperSkateShoes::isGrinding() const {
    return al::isNerve(this, &NrvSuperSkateShoesGrind);
}

/** @brief Places the rider on the ankle joint of the skates. */
void SuperSkateShoes::setPuppetQT() {
    sead::Matrix34f mtx;
    al::addTransMtxLocalOffset(&mtx, *al::getJointMtxPtr(this, "Ancle"),
                               rc::isPlayerMini(rc::getPuppetSensor(mPuppet)) ? cPuppetOffsetMini :
                                                                                cPuppetOffset);
    rc::setPuppetMtx(mPuppet, &mtx);
}

/** @brief Throws the rider backwards off the skates. */
inline void SuperSkateShoes::takeOffPuppet() {
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
bool SuperSkateShoes::isEnableWind(const al::SensorMsg* pMsg) const {
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

    return isRolling();
}

/** @brief Lets the touch assist grab the skates while they wait.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Screen point target.
 * @return Whether the message was handled.
 */
bool SuperSkateShoes::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssist(pMsg) && al::isNerve(this, &NrvSuperSkateShoesWait)) {
        return true;
    }

    return false;
}

/** @brief Updates the animation and keeps the rider on the skates. */
void SuperSkateShoes::calcAnim() {
    al::LiveActor::calcAnim();
    if (al::isDead(this)) {
        return;
    }

    if (al::isNerve(this, &NrvSuperSkateShoesGetOn) || isEnableAttack()) {
        setPuppetQT();
    }
}

/** @return Whether the skates were hidden; they stay visible while ridden. */
bool SuperSkateShoes::hideActor() {
    if (mPuppet != nullptr) {
        return false;
    }

    return al::LiveActor::hideActor();
}

/** @brief Counts down the dash panel boost. */
void SuperSkateShoes::control() {
    if (mDashPanelTime > 0) {
        mDashPanelTime--;
    }
}

/** @return Extra acceleration while boosted by a dash panel. */
f32 SuperSkateShoes::calcDashAccel() const {
    return mDashPanelTime > 0 ? 2.0f : 0.0f;
}

/** @return Extra turn speed while boosted by a dash panel. */
f32 SuperSkateShoes::calcDashTurnSpeed() const {
    return mDashPanelTime > 0 ? 1.5f : 0.0f;
}

/** @brief Makes the skates pop out of an object.
 * @param rTrans Appear position.
 * @param rVelocity Initial velocity.
 */
void SuperSkateShoes::appearFromObject(const sead::Vector3f& rTrans,
                                       const sead::Vector3f& rVelocity) {
    al::setNerve(this, &NrvSuperSkateShoesAppear);
    al::setVelocity(this, rVelocity);
    al::resetPosition(this, rTrans, false);
    appear();
}

/** @brief Drops the rider and kills the skates. */
void SuperSkateShoes::reset() {
    if (mPuppet != nullptr) {
        rc::endBindAndPuppetNull(&mPuppet, nullptr);
    }

    rc::setPlayerColorAnimDefault(this, "Color");
    al::hideSilhouetteModel(this);
    makeActorDead();
}

/** @brief Falls after popping out of an object until the skates land. */
void SuperSkateShoes::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }

    bool isOnGround = al::isOnGround(this, 0, 0.0f);
    updateVelocity(isOnGround);
    if (isOnGround) {
        al::setVelocityZero(this);
        al::startHitReaction(this, "出現後着地");
        al::setNerve(this, &NrvSuperSkateShoesWait);
    }
}

/** @brief Applies gravity, friction and collision rebound.
 * @param isOnGround Whether the skates are on the ground.
 */
void SuperSkateShoes::updateVelocity(bool isOnGround) {
    al::addVelocityToGravity(this, 1.5f);
    al::scaleVelocityHV(this, 0.94f, 0.995f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
}

/** @brief Waits for a rider. */
void SuperSkateShoes::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ItemWait");
    }

    updateVelocity(al::isOnGround(this, 4, 0.0f));
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
    }
}

/** @brief Puts the rider on the skates. */
void SuperSkateShoes::exeGetOn() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "GetOn");
        rc::startPuppetSe(mPuppet, "PgSkateGetOn");
        al::showSilhouetteModel(this);
        mSpeed = mSpeedScale;
    }

    if (al::isStep(this, 5)) {
        rc::setPlayerColorAnimBySensor(this, rc::getPuppetSensor(mPuppet), "Color");
    }

    updateTurn(6.0f);
    updateVelocity(al::isOnGround(this, 4, 0.0f));
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSuperSkateShoesRide);
    }
}

/** @brief Steers the skates with the rider's stick and leans them into the turn.
 * @param turnSpeed Turn speed in degrees per frame.
 */
void SuperSkateShoes::updateTurn(f32 turnSpeed) {
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
void SuperSkateShoes::exeAccel() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Accel");
        rc::startPuppetSe(mPuppet, "PgSkateAccel");
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(3.0f);
    mSpeed = al::calcNerveValue(this, 4, 0.0f, mSpeedScale * 1.6f);
    bool isOnGround = al::isOnGround(this, 4, 0.0f);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(isOnGround);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSuperSkateShoesRide);
    }
}

/**
 * @brief Ends the ride when the skates fall into a death area or crash into a wall head-on.
 * @return Whether the ride ended.
 */
bool SuperSkateShoes::tryWallHitEnd() {
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

    if (mIsNoTakeOff) {
        return false;
    }

    al::startHitReaction(this, "壁ヒット");
    rc::startPuppetSe(mPuppet, "PgItemTakenOff");
    wallNormal.y = 0.0f;
    if (!al::normalizeOrZero(&wallNormal)) {
        al::getFrontPtr(this)->set(-wallNormal);
    }

    takeOffPuppet();
    rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamTakeOff);
    al::setNerve(this, &NrvSuperSkateShoesTakeOff);
    return true;
}

/** @brief Runs at dash panel speed. */
void SuperSkateShoes::exeDash() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Accel");
        rc::startPuppetSe(mPuppet, "PgSkateAccel");
        mSpeed = mSpeedScale * 1.6f;
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(3.0f);
    bool isOnGround = al::isOnGround(this, 4, 0.0f);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(isOnGround);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSuperSkateShoesRide);
    }
}

/** @brief Rolls along with the rider, who can jump or dash. */
void SuperSkateShoes::exeRide() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Run");
    }

    if (tryWallHitEnd()) {
        return;
    }

    updateTurn(3.0f);
    mSpeed -= 0.007f;
    f32 maxSpeed = (rc::isPuppetHoldDashButton(mPuppet) ? 1.4f : 1.0f) * mSpeedScale;
    if (mSpeed < maxSpeed) {
        mSpeed += 0.05f;
        if (mSpeed > maxSpeed) {
            mSpeed = maxSpeed;
        }
    }

    bool isOnGround = al::isOnGround(this, 4, 0.0f);
    al::addVelocityToDirection(this, al::getFront(this), mSpeed + calcDashAccel());
    updateVelocity(isOnGround);
    if (!al::isOnGroundNoVelocity(this, 12)) {
        al::setNerve(this, &NrvSuperSkateShoesFall);
        return;
    }

    if (al::isCollidedGround(this)) {
        al::sendMsgPlayerFloorTouchToColliderGround(this, rc::getPuppetSensor(mPuppet));
    }

    if (isOnGround) {
        if (rc::isPuppetTrigJumpButton(mPuppet)) {
            al::getVelocityPtr(this)->y = 30.0f;
            al::setNerve(this, &NrvSuperSkateShoesJump);
            return;
        }

        if (rc::isPuppetTrigDashButton(mPuppet)) {
            al::setNerve(this, &NrvSuperSkateShoesAccel);
            return;
        }
    }

    if (al::isOnGround(this, 10, 0.0f)) {
        f32 speedH = al::calcSpeedH(this);
        al::holdSeWithParam(this, "PgMove", speedH);
        al::holdSeWithParam(this, "PgMoveCurve", mRotateZ * (speedH / 35.0f + 0.3f));
    }
}

/** @brief Falls with the rider until the skates land. */
void SuperSkateShoes::exeFall() {
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
    if (al::isOnGround(this, 0, 0.0f)) {
        al::startHitReaction(this, "着地");
        al::setNerve(this, &NrvSuperSkateShoesRide);
    }
}

/** @brief Jumps with the rider until the skates land. */
void SuperSkateShoes::exeJump() {
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
    if (al::isOnGround(this, 0, 0.0f)) {
        al::startHitReaction(this, "着地");
        al::setNerve(this, &NrvSuperSkateShoesRide);
    }
}

/** @brief Blows the empty skates away, then respawns them. */
void SuperSkateShoes::exeTakeOff() {
    if (al::isFirstStep(this)) {
        al::hideSilhouetteModel(this);
        al::startAction(this, "BlowDown");
        al::setVelocity(this, sead::Vector3f(0.0f, 35.0f, 0.0f));
    }

    updateVelocity(false);
    if (al::isActionEnd(this)) {
        al::startHitReactionBreak(this);
        al::setNerve(this, &NrvSuperSkateShoesRespawn);
    }
}

/** @brief Hides the skates for a while, then puts them back at their respawn position. */
void SuperSkateShoes::exeRespawn() {
    if (al::isFirstStep(this)) {
        if (mPuppet != nullptr) {
            rc::endBindAndPuppetNull(&mPuppet, nullptr);
        }

        rc::setPlayerColorAnimDefault(this, "Color");
        al::offCollide(this);
        al::hideModel(this);
        return;
    }

    if (al::isGreaterStep(this, 30)) {
        al::setTrans(this, mRespawnTrans);
        al::setNerve(this, &NrvSuperSkateShoesWait);
        al::onCollide(this);
        al::showModel(this);
    }
}

/** @brief Slides along the rail until its end, where the skates fall off. */
void SuperSkateShoes::exeGrind() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "SkateShoesWait");
        al::startAction(this, "Accel");
        rc::startPuppetSe(mPuppet, "PgSkateAccel");
        mSpeed = mSpeedScale * 1.6f;
        al::startSe(this, "PgMoveCurve");
    }

    if (rc::isPuppetTrigJumpButton(mPuppet)) {
        stopGrind();
        al::getVelocityPtr(this)->y = 30.0f;
        al::setNerve(this, &NrvSuperSkateShoesJump);
        return;
    }

    if (tryWallHitEnd()) {
        stopGrind();
        return;
    }

    if (al::getNerveStep(this) % 10 == 0) {
        al::startHitReaction(this, "Grind");
    }

    s32 grindFrames =
        (mIsGrindReverse ? mRail->getReverseDistance() : mRail->getDistance()) * 0.03f;
    if (!al::isLessStep(this, grindFrames)) {
        stopGrind();
        al::setNerve(this, &NrvSuperSkateShoesFall);
        return;
    }

    f32 rate = al::getNerveStep(this) / static_cast<f32>(grindFrames);
    sead::Vector3f trans = mIsGrindReverse ? mRail->getPreviousRail()->lerp(1.0f - rate) :
                                             mRail->lerp(rate);
    al::setTrans(this, trans);
}

/** @brief Leaves the current rail. */
void SuperSkateShoes::stopGrind() {
    mRail = nullptr;
    al::tryStopSe(this, "PgMoveCurve");
}

/** @brief Starts grinding on a rail, in the direction the skates move along it.
 * @param pRail Rail that was touched.
 */
void SuperSkateShoes::startGrind(SuperSkateRail* pRail) {
    if (pRail->getNextRail() == nullptr) {
        if (al::isNerve(this, &NrvSuperSkateShoesGrind)) {
            stopGrind();
            al::setNerve(this, &NrvSuperSkateShoesFall);
        }

        return;
    }

    if (mRail == pRail) {
        return;
    }

    const sead::Vector3f& velocity = al::getVelocity(this);
    mIsGrindReverse = velocity.dot(pRail->getDirection()) < 0.0f;
    if (mIsGrindReverse && pRail->getPreviousRail() == nullptr) {
        return;
    }

    mGrindSpeed = sead::Mathf::max(1.0f, al::calcSpeedV(this));
    al::setVelocity(this, 0.0f, 0.0f, 0.0f);
    sead::Vector3f front;
    front.setRotated(cGrindFrontMtx,
                     mIsGrindReverse ? pRail->getReverseDirection() : pRail->getDirection());
    al::setFront(this, front);
    mRail = pRail;
    al::setTrans(this, al::getTrans(pRail));
    al::setNerve(this, &NrvSuperSkateShoesGrind);
}

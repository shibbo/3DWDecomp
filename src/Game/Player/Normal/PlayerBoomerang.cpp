#include "Player/Normal/PlayerBoomerang.hpp"

#include <prim/seadSafeString.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounterWithSe.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/ActorParamHolder.hpp"
#include "Library/LiveActor/ActorParamHolderUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

/// Declares a boomerang nerve whose state shares the execute function of another state.
#define PLAYER_BOOMERANG_NERVE_DECL(Action, ExeAction)                                             \
    class PlayerBoomerangNrv##Action : public al::Nerve {                                          \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<PlayerBoomerang>())->exe##ExeAction();                             \
        }                                                                                          \
    };

namespace {
NERVE_DECL(PlayerBoomerang, Wait);
NERVE_DECL(PlayerBoomerang, MoveBack);
PLAYER_BOOMERANG_NERVE_DECL(MoveBackAbove, MoveBack);
NERVE_DECL(PlayerBoomerang, MoveGo);
NERVE_DECL(PlayerBoomerang, Burn);
NERVE_DECL(PlayerBoomerang, Interval);
NERVE_DECL(PlayerBoomerang, MoveBrake);
PLAYER_BOOMERANG_NERVE_DECL(MoveStayAbove, MoveStay);
NERVE_DECL(PlayerBoomerang, MoveStay);
PLAYER_BOOMERANG_NERVE_DECL(MoveLostAbove, MoveLost);
NERVE_DECL(PlayerBoomerang, MoveLost);
PLAYER_BOOMERANG_NERVE_DECL(MoveBrakeAbove, MoveBrake);
NERVES_MAKE_NOSTRUCT(PlayerBoomerang, Wait, MoveBack, MoveBackAbove, MoveGo, Burn, Interval,
                     MoveBrake, MoveStayAbove, MoveStay, MoveLostAbove, MoveLost, MoveBrakeAbove)

/// Archive of the boomerang model for each playable character.
const char* const cArchiveName[] = {
    "MarioBoomerangBoomerang",   "LuigiBoomerangBoomerang",   "PeachBoomerangBoomerang",
    "KinopioBoomerangBoomerang", "RosettaBoomerangBoomerang", "KinopioBoomerangBoomerang",
    "KinopioBoomerangBoomerang", "KinopioBoomerangBoomerang", "KinopioBoomerangBoomerang",
};
}  // namespace

/**
 * @brief Constructs the boomerang.
 * @param chara Character throwing the boomerang (selects the model).
 * @param pInput Input of the throwing player.
 */
PlayerBoomerang::PlayerBoomerang(EPlayerChara chara, const IUsePlayerInput* pInput)
    : al::LiveActor("プレイヤーブーメラン"), mChara(chara),
      mComboCounter(new al::ComboCounterWithSe(this)), mInput(pInput) {}

/**
 * @brief Initializes the model of the character, the parameters, the effect matrix and the nerve.
 * The boomerang starts dead.
 * @param rInfo Actor init info.
 */
void PlayerBoomerang::init(const al::ActorInitInfo& rInfo) {
    mIsEnableEcho = rInfo.getActorSceneInfo().isSingleMode;
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, sead::SafeString(cArchiveName[mChara]),
                                                nullptr);
    al::invalidateClipping(this);
    mParam = new PlayerBoomerangParam(this);
    al::setEffectFollowMtxPtr(this, "WallHit", &mEffectMtx);
    mColliderRadius = al::getColliderRadius(this);
    al::initNerve(this, &NrvPlayerBoomerangWait, 0);
    makeActorDead();
}

/**
 * @brief Looks up all boomerang parameters.
 * @param pActor Actor holding the parameters.
 */
PlayerBoomerangParam::PlayerBoomerangParam(al::LiveActor* pActor) {
    mThrowHeight = al::findActorParamF32(pActor, "投げる高さ");
    mBrakeStrength = al::findActorParamF32(pActor, "ブレーキ強さ");
    mReturnStrength = al::findActorParamF32(pActor, "戻り強さ");
    mEndStopTime = al::findActorParamS32(pActor, "端点停止時間");
    mSpeed = al::findActorParamF32(pActor, "速度");
    mRange = al::findActorParamF32(pActor, "到達距離");
    mGravity = al::findActorParamF32(pActor, "重力");
    mMaxFallSpeed = al::findActorParamF32(pActor, "落下最高速度");
    mAboveHeight = al::findActorParamF32(pActor, "プレイヤーより上とみなす高低差");
    mTurnLimitDegree = al::findActorParamF32(pActor, "ターン限界角度");
    mLostTime1 = al::findActorParamS32(pActor, "見失い時間[1回目]");
    mLostTime2 = al::findActorParamS32(pActor, "見失い時間[2回目]");
    mLostTime3 = al::findActorParamS32(pActor, "見失い時間[3回目]");
    mReflectRate = al::findActorParamF32(pActor, "反射率");
    mBurnTime = al::findActorParamS32(pActor, "燃える時間");
    mBurnAirResistance = al::findActorParamF32(pActor, "空気抵抗[燃え時]");
    mBurnHitBrake = al::findActorParamF32(pActor, "衝突減速[燃え時]");
    mBreakInterval = al::findActorParamS32(pActor, "壊れた後のインターバル");
}

/**
 * @brief Throws the boomerang forward from the player.
 * @param pPlayer Throwing player.
 * @param pHolder Unused.
 */
void PlayerBoomerang::appearBoomerang(const al::LiveActor* pPlayer,
                                      const al::LiveActor* pHolder) {
    mIsCatchTrigOn = false;
    mPlayer = pPlayer;
    al::setColliderRadius(this, mColliderRadius);

    sead::Vector3f front;
    al::calcFrontDir(&front, mPlayer);
    startThrow(sead::Quatf::unit, front * mParam->mSpeed->value);
    appear();
    al::updateHitSensorsAll(this);
}

/**
 * @brief Starts the throw: places the boomerang in front of the player (pulled back out of any
 * wall in between) and starts flying.
 * @param rQuat Initial rotation.
 * @param rVelocity Throw velocity.
 */
void PlayerBoomerang::startThrow(const sead::Quatf& rQuat, const sead::Vector3f& rVelocity) {
    al::updatePoseQuat(this, rQuat);
    al::setVelocity(this, rVelocity);
    mThrowDir.set(rVelocity);
    if (al::isNearZero(mThrowDir, 0.001f)) {
        al::calcFrontDir(&mThrowDir, mPlayer);
    }

    al::normalize(&mThrowDir);
    sead::Vector3f* pTrans = al::getTransPtr(this);
    pTrans->set(al::getTrans(mPlayer) + sead::Vector3f(0.0f, mParam->mThrowHeight->value, 0.0f) +
                mThrowDir * 80.0f);
    mThrowSpeed = rVelocity.length();
    mGoStep = mParam->mRange->value / mThrowSpeed;
    al::onCollide(this);
    getCollider()->onInvalidate();

    sead::Vector3f trans = al::getTrans(this);
    sead::Vector3f dir = mThrowDir;
    al::Triangle triangle;
    f32 radius = al::getColliderRadius(this);
    sead::Vector3f start = trans - dir * (radius + 80.0f);
    alCollisionUtil::SphereMoveHitInfo hitInfos[16];
    u32 hitNum = alCollisionUtil::checkStrikeSphereMove(this, hitInfos, 16, start, radius,
                                                        trans - start, nullptr, nullptr);
    if (hitNum != 0) {
        f32 minTime = 1.0f;
        for (u32 i = 0; i < hitNum; i++) {
            f32 time = hitInfos[i].time;
            if (time >= 0.0f) {
                minTime = sead::Mathf::min(time, minTime);
            }
        }

        al::setTrans(this, start + (trans - start) * minTime);
    }

    al::startAction(this, "Spin");
    al::setNerve(this, &NrvPlayerBoomerangMoveGo);
}

/**
 * @brief Breaks the boomerang if it is flying.
 * @return Whether the boomerang was broken.
 */
bool PlayerBoomerang::tryKill() {
    if (al::isDead(this)) {
        return false;
    }

    if (isMoving()) {
        doBreak("破壊");
        return true;
    }

    return false;
}

/**
 * @brief Whether the boomerang is in one of its flying states.
 * @return True while flying.
 */
bool PlayerBoomerang::isMoving() const {
    return al::isNerve(this, &NrvPlayerBoomerangMoveGo) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveBrake) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveStay) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveBack) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveLost) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveBrakeAbove) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveStayAbove) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveBackAbove) ||
           al::isNerve(this, &NrvPlayerBoomerangMoveLostAbove);
}

/**
 * @brief Stops and hides the boomerang, and waits for the interval after a break.
 * @param pReactionName Hit reaction to start.
 */
void PlayerBoomerang::doBreak(const char* pReactionName) {
    al::setVelocityZero(this);
    al::startHitReaction(this, pReactionName);
    al::hideModelIfShow(this);
    al::startAction(this, "Normal");
    al::setNerve(this, &NrvPlayerBoomerangInterval);
}

/**
 * @brief Breaks a flying boomerang and kills it at once.
 */
void PlayerBoomerang::forceEnd() {
    if (tryKill()) {
        al::setVelocityZero(this);
        kill();
    }
}

/**
 * @brief Appears and resets the combo, the reflection state and the lost count.
 */
void PlayerBoomerang::appear() {
    al::LiveActor::appear();
    mComboCounter->reset();
    mIsReflected = false;
    mLostCount = 0;
    mReflectInterval = -1;
    if (al::isHideModel(this)) {
        al::showModel(this);
    }

    al::validateHitSensors(this);
}

/**
 * @brief Handles the catch by the player, breaking, reflecting and attacking other sensors.
 * @param pSelf Own sensor.
 * @param pOther Other sensor.
 */
void PlayerBoomerang::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther) &&
        (al::isNerve(this, &NrvPlayerBoomerangMoveBack) ||
         al::isNerve(this, &NrvPlayerBoomerangMoveBackAbove)) &&
        al::isSensorName(pSelf, "PreCatch") && !mIsCatchTrigOn && mInput->isFireBallTrigOn()) {
        mIsCatchTrigOn = true;
    }

    if (!al::isSensorName(pSelf, "Attack")) {
        return;
    }

    if (!isMoving()) {
        return;
    }

    sead::Matrix34f invMtx;
    invMtx.setInverse(*getBaseMtx());
    sead::Vector3f localPos;
    localPos.setMul(invMtx, al::getSensorPos(pOther));
    f32 height = localPos.y > 0.0f ? localPos.y : -localPos.y;
    if (!(height < al::getSensorRadius(pOther) + 24.0f)) {
        return;
    }

    if (al::isSensorPlayer(pOther) && al::getSensorHost(pOther) == mPlayer && tryCatch()) {
        return;
    }

    if (al::sendMsgPlayerBoomerangBreak(pOther, pSelf)) {
        doBreak("破壊");
        return;
    }

    al::sendMsgKickKouraGetItem(pOther, pSelf);
    if (mReflectInterval <= 0 && al::sendMsgPlayerBoomerangReflect(pOther, pSelf)) {
        if (al::isNerve(this, &NrvPlayerBoomerangMoveGo)) {
            doReflectSensor(pSelf, pOther);
            return;
        }

        if (mIsReflected &&
            (al::isNerve(this, &NrvPlayerBoomerangMoveBackAbove) ||
             al::isNerve(this, &NrvPlayerBoomerangMoveBack)) &&
            al::isLessStep(this, 5)) {
            return;
        }

        doBreak("破壊");
        return;
    }

    al::sendMsgPlayerBoomerangAttack(pOther, pSelf, mComboCounter);
}

/**
 * @brief Gets caught by the player unless the boomerang was just thrown.
 * @return Whether the boomerang was caught.
 */
bool PlayerBoomerang::tryCatch() {
    if (!isMoving()) {
        return false;
    }

    if (al::isNerve(this, &NrvPlayerBoomerangMoveGo) && al::isLessStep(this, 20)) {
        return false;
    }

    caught();
    return true;
}

/**
 * @brief Bounces the boomerang back after hitting a reflecting sensor.
 * @param pSelf Own sensor.
 * @param pOther Reflecting sensor.
 */
void PlayerBoomerang::doReflectSensor(const al::HitSensor* pSelf, const al::HitSensor* pOther) {
    if (mReflectInterval > 0) {
        return;
    }

    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    pVelocity->negate();
    pVelocity = al::getVelocityPtr(this);
    *pVelocity *= mParam->mReflectRate->value;

    sead::Vector3f front = al::getVelocity(this);
    if (al::normalizeOrZero(&front)) {
        al::calcFrontDir(&front, this);
        front = -front;
    }

    sead::Vector3f pos;
    al::calcPosBetweenSensors(&pos, pSelf, pOther, al::getSensorRadius(pSelf));
    al::makeMtxFrontUpPos(&mEffectMtx, front, -al::getGravity(this), pos);
    al::startHitReaction(this, "反射[センサー当たり]");
    mThrowDir.set(al::getVelocity(this));
    if (al::normalizeOrZero(&mThrowDir)) {
        al::calcFrontDir(&mThrowDir, mPlayer);
    }

    mReflectInterval = 3;
    mIsReflected = true;
}

/**
 * @brief Breaks on goal demos, dies on demo starts and answers host player queries. A fire attack
 * burns the flying boomerang.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Receiver sensor.
 * @return Whether the message was handled.
 */
bool PlayerBoomerang::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (rc::isMsgStartGoalDemoPole(pMsg)) {
        tryKill();
        return true;
    }

    if (rc::isMsgStartGoalDemoHouse(pMsg) || rc::isMsgStartDemoBossStart(pMsg)) {
        makeActorDead();
        return true;
    }

    if (al::isHideModel(this)) {
        return false;
    }

    if (mPlayer != nullptr && rc::isMsgAskControlUserId(pMsg, rc::findControlUserId(mPlayer))) {
        return true;
    }

    if (rc::isMsgQueryHostPlayer(pMsg)) {
        return rc::isEqualHostPlayer(pMsg, mPlayer);
    }

    if (!al::isSensorName(pSelf, "Body")) {
        return false;
    }

    if (!isMoving()) {
        return false;
    }

    if (al::isMsgEnemyAttackFire(pMsg)) {
        al::setNerve(this, &NrvPlayerBoomerangBurn);
        return true;
    }

    return false;
}

/**
 * @brief Counts down the reflection interval, updates the water material and hides the model
 * near the camera.
 */
void PlayerBoomerang::control() {
    if (mReflectInterval > 0) {
        mReflectInterval--;
    }

    al::updateMaterialCodeWater(this);
    controlShowHideModel();
}

/**
 * @brief Hides the flying boomerang when it gets close to the camera.
 */
void PlayerBoomerang::controlShowHideModel() {
    if (isMoving()) {
        al::switchShowHideModelIfNearCamera(this, al::getSensorRadius(this, "Body") * 2.0f);
    }
}

/**
 * @brief Applies gravity in the states above the player.
 */
void PlayerBoomerang::updateGravity() {
    if (al::isNerve(this, &NrvPlayerBoomerangMoveBrakeAbove) ||
        al::isNerve(this, &NrvPlayerBoomerangMoveStayAbove) ||
        al::isNerve(this, &NrvPlayerBoomerangMoveBackAbove) ||
        al::isNerve(this, &NrvPlayerBoomerangMoveLostAbove)) {
        al::tryAddVelocityLimit(this, al::getGravity(this) * mParam->mGravity->value,
                                mParam->mMaxFallSpeed->value);
    }
}

/**
 * @brief Vanishes when flying into a wall.
 * @return Whether a wall was hit.
 */
bool PlayerBoomerang::tryHitWall() {
    if (!al::isCollidedWallFace(this)) {
        return false;
    }

    if (al::getCollidedWallNormal(this).dot(al::getVelocity(this)) > 0.0f) {
        return false;
    }

    trySendMsgToCollision();
    doBreak("消滅[壁ヒット]");
    if (mIsEnableEcho) {
        rc::emitEcho(this, al::getTrans(this), 200.0f, 60, false);
    }

    return true;
}

/**
 * @brief Attacks the sensor of the collision the boomerang hit.
 */
void PlayerBoomerang::trySendMsgToCollision() {
    al::HitSensor* pSensor = al::tryGetCollidedWallSensor(this);
    if (pSensor == nullptr) {
        pSensor = al::tryGetCollidedGroundSensor(this);
    }

    if (pSensor == nullptr) {
        pSensor = al::tryGetCollidedCeilingSensor(this);
    }

    if (pSensor != nullptr) {
        al::sendMsgPlayerBoomerangAttackCollide(pSensor, al::getHitSensor(this, "Attack"));
    }
}

/**
 * @brief Bounces the boomerang off a wall.
 * @return Whether the boomerang was reflected.
 */
bool PlayerBoomerang::tryReflectWall() {
    if (mReflectInterval > 0) {
        return false;
    }

    if (!al::isCollidedWallFace(this)) {
        return false;
    }

    sead::Vector3f normal = al::getCollidedWallNormal(this);
    al::verticalizeVec(&normal, al::getGravity(this), normal);
    if (al::normalizeOrZero(&normal)) {
        return false;
    }

    if (!al::calcReflectionVector(al::getVelocityPtr(this), normal, mParam->mReflectRate->value,
                                  0.0f)) {
        return false;
    }

    al::makeMtxFrontUpPos(&mEffectMtx, normal, -al::getGravity(this),
                          al::getCollidedWallPos(this));
    al::startHitReaction(this, "反射[壁ヒット]");
    mThrowDir.set(al::getVelocity(this));
    if (al::normalizeOrZero(&mThrowDir)) {
        al::calcFrontDir(&mThrowDir, mPlayer);
    }

    mReflectInterval = 3;
    trySendMsgToCollision();
    if (mIsEnableEcho) {
        rc::emitEcho(this, al::getTrans(this), 200.0f, 60, false);
    }

    return true;
}

/**
 * @brief Ends the throw after the player caught the boomerang.
 */
void PlayerBoomerang::caught() {
    al::offCollide(this);
    al::startAction(this, "Normal");
    al::startHitReaction(this, "キャッチ");
    mLostCount = 0;
    al::setNerve(this, &NrvPlayerBoomerangWait);
    kill();
}

/**
 * @brief Gets the distance the boomerang flies forward.
 * @return Range.
 */
f32 PlayerBoomerang::getRange() const {
    return mParam->mRange->value;
}

/**
 * @brief Gets how long the boomerang stays lost, which depends on how often it got lost.
 * @return Lost time in steps.
 */
s32 PlayerBoomerang::getLostTime() const {
    switch (mLostCount) {
    case 1:
        return mParam->mLostTime1->value;
    case 2:
        return mParam->mLostTime2->value;
    case 3:
        return mParam->mLostTime3->value;
    default:
        return 0;
    }
}

/**
 * @brief Whether the boomerang flies higher above the player than the parameter allows.
 * @return True if above the player.
 */
bool PlayerBoomerang::isAbovePlayer() const {
    return al::getTrans(mPlayer).y + mParam->mAboveHeight->value < al::getTrans(this).y;
}

/**
 * @brief Whether there is collision right in front of the boomerang.
 * @return True if the boomerang is about to hit a wall.
 */
bool PlayerBoomerang::checkCollideFront() const {
    sead::Vector3f dir = al::getVelocity(this);
    f32 length = dir.length();
    if (length > 0.0f) {
        dir *= 500.0f / length;
    }

    sead::Vector3f hitPos;
    if (!alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, nullptr, al::getTrans(this), dir,
                                              nullptr, nullptr)) {
        return false;
    }

    f32 distance =
        al::getSensorRadius(this, "Attack") + al::getVelocity(this).length() * 2.0f;
    return (hitPos - al::getTrans(this)).squaredLength() < distance * distance;
}

/**
 * @brief Waits while caught.
 */
void PlayerBoomerang::exeWait() {}

/**
 * @brief Flies forward until the range is reached, then brakes, or flies back if reflected.
 */
void PlayerBoomerang::exeMoveGo() {
    tryReflectWall();
    if (al::isGreaterStep(this, mGoStep)) {
        if (mIsReflected) {
            al::turnQuatFrontToPosDegreeH(this, al::getTrans(mPlayer), 180.0f);
            al::setNerve(this, &NrvPlayerBoomerangMoveBack);
            return;
        }

        al::setNerve(this, &NrvPlayerBoomerangMoveBrake);
    }
}

/**
 * @brief Brakes against the throw direction until the boomerang stops.
 */
void PlayerBoomerang::exeMoveBrake() {
    al::addVelocity(this, -(mParam->mBrakeStrength->value * mThrowDir));
    if (tryReflectWall()) {
        return;
    }

    sead::Vector3f velocity = al::getVelocity(this);
    velocity.y = 0.0f;
    if (checkCollideFront()) {
        velocity.set(0.0f, 0.0f, 0.0f);
    }

    if (mLostCount >= 1) {
        updateGravity();
    }

    if (al::isNearZero(velocity, 0.001f) || al::isReverseDirection(velocity, mThrowDir, 0.01f) ||
        !isLostPlayer()) {
        if (isAbovePlayer()) {
            al::setNerve(this, &NrvPlayerBoomerangMoveStayAbove);
        } else {
            al::setNerve(this, &NrvPlayerBoomerangMoveStay);
        }
    }
}

/**
 * @brief Whether the boomerang is horizontally close to the player.
 * @return True if close to the player.
 */
bool PlayerBoomerang::isLostPlayer() const {
    sead::Vector3f diff = al::getTrans(mPlayer) + sead::Vector3f::ey * 100.0f - al::getTrans(this);
    diff.y = 0.0f;
    return diff.length() < 80.0f;
}

/**
 * @brief Hovers at the end point, then turns around towards the player.
 */
void PlayerBoomerang::exeMoveStay() {
    if (al::isFirstStep(this)) {
        al::getVelocityPtr(this)->x = 0.0f;
        al::getVelocityPtr(this)->z = 0.0f;
        if (mLostCount >= 4) {
            doBreak("破壊");
            return;
        }
    }

    if (al::isGreaterStep(this, mParam->mEndStopTime->value)) {
        al::turnQuatFrontToPosDegreeH(this, al::getTrans(mPlayer), 180.0f);
        mBackSpeed = 0.0f;
        if (isAbovePlayer()) {
            al::setNerve(this, &NrvPlayerBoomerangMoveBackAbove);
        } else {
            al::setNerve(this, &NrvPlayerBoomerangMoveBack);
        }
    }
}

/**
 * @brief Flies back towards the player, speeding up.
 */
void PlayerBoomerang::exeMoveBack() {
    bool isLost = isLostPlayer();
    const sead::Vector3f& rPlayerTrans = al::getTrans(mPlayer);
    if (isLost) {
        if (rPlayerTrans.y + mParam->mAboveHeight->value < al::getTrans(this).y) {
            al::setNerve(this, &NrvPlayerBoomerangMoveLostAbove);
        } else {
            al::setNerve(this, &NrvPlayerBoomerangMoveLost);
        }

        return;
    }

    al::turnQuatFrontToPosDegreeH(this, rPlayerTrans, mParam->mTurnLimitDegree->value);
    mBackSpeed += mParam->mReturnStrength->value;
    mBackSpeed = sead::Mathf::clamp(mBackSpeed, 0.0f, mThrowSpeed);

    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    al::setVelocity(this, front * mBackSpeed);
    updateGravity();
    if (!tryHitWall() && al::isGreaterStep(this, 420)) {
        kill();
    }
}

/**
 * @brief Keeps flying while the player is lost, then brakes again.
 */
void PlayerBoomerang::exeMoveLost() {
    if (al::isFirstStep(this)) {
        mLostCount++;
        al::calcQuatFront(&mThrowDir, this);
    }

    if (tryHitWall()) {
        return;
    }

    if (mLostCount >= 1) {
        updateGravity();
    }

    if (al::isGreaterEqualStep(this, getLostTime())) {
        if (isAbovePlayer()) {
            al::setNerve(this, &NrvPlayerBoomerangMoveBrakeAbove);
        } else {
            al::setNerve(this, &NrvPlayerBoomerangMoveBrake);
        }
    }
}

/**
 * @brief Burns up after a fire attack.
 */
void PlayerBoomerang::exeBurn() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "消滅[炎ヒット]");
        al::startAction(this, "Normal");
        al::hideModelIfShow(this);
        al::invalidateHitSensors(this);

        sead::Vector3f dir = al::getVelocity(this);
        al::normalizeOrZero(&dir);
        al::addVelocity(this, dir * -mParam->mBurnHitBrake->value);
    }

    al::scaleVelocity(this, mParam->mBurnAirResistance->value);
    if (al::isGreaterEqualStep(this, mParam->mBurnTime->value)) {
        al::setVelocityZero(this);
        kill();
    }
}

/**
 * @brief Waits for the interval after a break, then dies.
 */
void PlayerBoomerang::exeInterval() {
    if (al::isGreaterEqualStep(this, mParam->mBreakInterval->value)) {
        al::setVelocityZero(this);
        kill();
    }
}

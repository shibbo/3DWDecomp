#include "Library/Player/PlayerUtil.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseKoopa.hpp"
#include "Boss/KoopaChaseBattleLv1.hpp"
#include "Boss/KoopaChaseBattleLv2.hpp"
#include "Boss/KoopaChaseMover.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <cmath>
namespace {
NERVE_DECL(KoopaChase, Battle);
NERVES_MAKE_NOSTRUCT(KoopaChase, Battle)
}
/** @brief Creates the chase car and its movement fields. @param pName Actor name. */
KoopaChase::KoopaChase(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the chase actor. */
KoopaChase::~KoopaChase() = default;
/** @brief Gets the distance for starting the chase. @return Starting distance. */
float KoopaChase::getRunStartDistance() { return 1700.0f; }
/** @brief Gets the distance for stopping the chase. @return Stopping distance. */
float KoopaChase::getRunStopDistance() { return 400.0f; }
/** @brief Initializes Bowser, tire controllers, battle state, and movement points. @param rInfo Initialization data. */
void KoopaChase::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    mKoopa = new KoopaChaseKoopa(this);
    mKoopa->init(rInfo);
    al::copyPose(mKoopa, this);
    al::setTrans(mKoopa, al::getTrans(this));
    al::initJointControllerKeeper(this, 4);
    al::initJointLocalZRotator(this, &mTireRotation, "TireFrontSxis");
    al::initJointLocalZRotator(this, &mTireRotation, "TireBackSxis");
    al::initJointLocalYRotator(this, &mRootRotation, "AllRoot");
    al::initJointLocalTransController(this, &mRootOffset, "AllRoot");
    al::initNerve(this, &NrvKoopaChaseBattle, 3);
    if (al::isObjectName(rInfo, "KoopaChaseLv2")) {
        mBattle = new KoopaChaseBattleLv2(this, rInfo);
        al::initNerveState(this, mBattle, &NrvKoopaChaseBattle, "バトルLv2");
    } else {
        mBattle = new KoopaChaseBattleLv1(this, rInfo);
        al::initNerveState(this, mBattle, &NrvKoopaChaseBattle, "バトルLv1");
    }
    al::setHitSensorPosPtr(this, "Core", al::getTransPtr(mKoopa));
    mMover = new KoopaChaseMover(this, rInfo);
    if (al::trySyncStageSwitchAppear(this)) {
        mKoopa->makeActorDead();
        al::hideModel(mKoopa);
        al::hideModel(this);
    }
}
/** @brief Shows the car and Bowser. */
void KoopaChase::appear() { al::LiveActor::appear(); mKoopa->appear(); }
/** @brief Removes the camera target and both actors. */
void KoopaChase::kill() {
    al::setAdditionalCameraLookAtPosPtr(this, this, nullptr);
    al::LiveActor::kill();
    mKoopa->kill();
}
/** @brief Removes effects from the car and Bowser. */
void KoopaChase::stopEffectAll() {
    al::tryDeleteEmitterAndParticleAll(this);
    al::tryDeleteEmitterAndParticleAll(mKoopa);
}
/** @brief Updates tire effects, targets, movement, and camera tracking. */
void KoopaChase::control() {
    if (mBattle->isStateTireMove()) {
        float speed = al::calcSpeedH(this);
        float rotation = al::lerpValue(speed, 0.0f, 10.0f, 1.0f, 7.0f);
        if (rotation > 3.0f) {
            al::tryEmitEffect(this, "TireFrontL", nullptr);
            al::tryEmitEffect(this, "TireFrontR", nullptr);
            al::tryEmitEffect(this, "TireBackL", nullptr);
            al::tryEmitEffect(this, "TireBackR", nullptr);
        } else if (rotation < 1.0f) {
            al::tryDeleteEffect(this, "TireFrontL");
            al::tryDeleteEffect(this, "TireFrontR");
            al::tryDeleteEffect(this, "TireBackL");
            al::tryDeleteEffect(this, "TireBackR");
        }
        if (KoopaChaseFunction::getDamageCount(this) >= 2) {
            al::tryDeleteEffect(this, "TireBackR");
            al::tryDeleteEffect(this, "TireFrontL");
        }
        mTireRotation = al::modf(rotation + mTireRotation + 360.0f, 360.0f) + 0.0f;
        if (speed > 4.0f) mSwayStep++;
        float angle = mSwayStep * 0.02617993950843811f;
        mRootRotation = cosf(angle) * 2.0f;
        mRootOffset.x = cosf(angle) * 20.0f;
    }
    if (rc::calcActivePlayerNum(this) >= 1) {
        mTargetPlayer = rc::tryFindNearestActivePlayerActorInSphere(this, 2000.0f);
        mLookAtTargetPlayer = rc::findNearestActivePlayerActor(this);
    }
    if (mBattle->isStateUpdateMoveCount()) mValue168++;
    if (mBattle->isStateUpdatePose()) KoopaChaseFunction::updateKoopaPose(this);
    if (mBattle->isStateTurnToTarget() && mLookAtTargetPlayer) mKoopa->updatePose(al::getTrans(mLookAtTargetPlayer));
    mMover->update();
    if (mBattle->isStateWarp()) {
        if (isAllPlayerBinded()) {
            if (mLookAtTargetPlayer) mCameraLookAt = al::getTrans(mLookAtTargetPlayer);
        } else {
            sead::Vector3f pos(mPreviousKoopaPos.x, mPreviousKoopaPos.y, mPreviousKoopaPos.z);
            sead::Vector3f target;
            if (mLookAtTargetPlayer) target = al::getTrans(mLookAtTargetPlayer);
            else target = pos + sead::Vector3f(0.0f, 0.0f, 100.0f);
            pos.y = target.y;
            pos.x = pos.x + (target.x - pos.x) * 0.67f;
            pos.y = pos.y + (target.y - pos.y) * 0.67f;
            pos.z = pos.z + (target.z - pos.z) * 0.67f;
            sead::Vector3f delta = pos - mCameraLookAt;
            sead::Vector3f dir = al::getPlayerRailDir(this);
            if (delta.dot(dir) > 0.0f) mCameraLookAt = pos;
            return;
        }
    } else if (mLookAtTargetPlayer) {
        const auto& koopaPos = al::getTrans(mKoopa);
        const auto& targetPos = al::getTrans(mLookAtTargetPlayer);
        mCameraLookAt.x = koopaPos.x + (targetPos.x - koopaPos.x) * 0.67f;
        mCameraLookAt.y = koopaPos.y + (targetPos.y - koopaPos.y) * 0.67f;
        mCameraLookAt.z = koopaPos.z + (targetPos.z - koopaPos.z) * 0.67f;
    }
    mPreviousKoopaPos = al::getTrans(mKoopa);
}
/** @brief Checks whether every active player is bound. @return True when no unbound active player remains. */
bool KoopaChase::isAllPlayerBinded() const {
    for (int i = 0; i < al::getPlayerNumMax(this); i++) {
        if (!rc::isPlayerDeadOrBubble(al::getPlayerActor(this, i)) && !rc::isPlayerBinded(al::getPlayerActor(this, i))) return false;
    }
    return true;
}
/** @brief Pushes players away from map-object sensors. @param pSelf Own sensor. @param pOther Target sensor. */
void KoopaChase::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pSelf) && al::isSensorPlayer(pOther)) al::sendMsgPushVeryStrong(pOther, pSelf);
}
/** @brief Forwards battle messages. @param pMsg Message. @param pSelf Own sensor. @param pOther Sender sensor. @return Whether handled. */
bool KoopaChase::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pOther)) return false;
    if (al::isNerve(this, &NrvKoopaChaseBattle)) return mBattle->receiveMsg(pMsg, pSelf, pOther);
    return false;
}
/** @brief Registers the camera target and updates the battle state. */
void KoopaChase::exeBattle() {
    if (al::isFirstStep(this)) al::setAdditionalCameraLookAtPosPtr(this, this, &mCameraLookAt);
    if (al::updateNerveState(this)) kill();
}

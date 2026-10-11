#include "Boss/InkBomb.hpp"
#include "Enemy/BombStateExplosion.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "MapObj/ExplosionComboCounterHolder.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

// Nerve whose host also has an end callback.
#define NERVE_END_DECL(Class, Action)                                                              \
    class Class##Nrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Class>())->exe##Action();                                          \
        }                                                                                          \
                                                                                                   \
        void executeOnEnd(al::NerveKeeper* pKeeper) const override {                               \
            (pKeeper->getParent<Class>())->end##Action();                                          \
        }                                                                                          \
    };

namespace {
NERVE_END_DECL(InkBomb, Placed)
NERVE_DECL(InkBomb, Explosion)
NERVE_DECL(InkBomb, Break)

/** @brief Explosion that leaves Fury Bowser unharmed; shares the regular explosion behavior. */
class InkBombNrvExplosionNoDamage : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<InkBomb>()->exeExplosion();
    }
};

NERVE_DECL(InkBomb, Kicked)
NERVE_DECL(InkBomb, Wait)
NERVE_END_DECL(InkBomb, Roll)
NERVE_DECL(InkBomb, Bound)
NERVE_DECL(InkBomb, Stop)

NERVES_MAKE_STRUCT(InkBomb, Placed, Explosion, Break, ExplosionNoDamage, Kicked, Wait, Roll, Bound,
                   Stop)

/**
 * @brief Switches both the effect and the sound material of the bomb.
 * @param rActor Bomb to update.
 * @param pMaterial Material code to use.
 */
void updateMaterialCode(InkBomb& rActor, const char* pMaterial) {
    al::tryUpdateEffectMaterialCode(&rActor, pMaterial);
    al::tryUpdateSeMaterialCode(&rActor, pMaterial);
}

const BombStateExplosionParam cExplosionParam = {
    4500.0f, 2250.0f, 500.0f, {4.0f, 2.0f, 1.0f}, 1.0f, 8,
};
}  // namespace

/**
 * @brief Constructs an ink bomb with its fuse idle.
 * @param pName Actor name.
 * @param pHost Actor that throws the bomb.
 */
InkBomb::InkBomb(const char* pName, const al::LiveActor* pHost)
    : al::LiveActor(pName), mHost(pHost) {}

/**
 * @brief Initializes the model, the explosion states and the rolling smoke effect, then waits dead
 * until thrown.
 * @param rInfo Actor placement and scene information.
 */
void InkBomb::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "InkBomb", nullptr);
    al::initNerve(this, &NrvInkBomb.Placed, 3);
    mStateExplosion = new BombStateExplosion(this, true, &cExplosionParam);
    mStateExplosionNoDamage = new BombStateExplosion(this, true, &cExplosionParam);
    mStateBreak = new BombStateExplosion(this, true, nullptr);
    al::initNerveState(this, mStateExplosion, &NrvInkBomb.Explosion, "爆発");
    al::initNerveState(this, mStateBreak, &NrvInkBomb.Break, "破壊");
    al::initNerveState(this, mStateExplosionNoDamage, &NrvInkBomb.ExplosionNoDamage, "NoDamage");
    rc::createExplosionComboCounter(this);
    al::setScaleAll(this, 15.0f);
    al::setTrans(this, al::getTrans(this) - al::getGravity(this) * 75.0f);
    al::setEffectFollowPosPtr(this, "RollingSmoke", &mSmokePos);
    makeActorDead();
}

/** @brief Appears without collision until the bomb starts moving. */
void InkBomb::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::offCollide(this);
}

/**
 * @brief Damages what the explosion touches and deflects enemies off a moving bomb.
 * @param pSelf Sensor of the bomb.
 * @param pOther Sensor that was touched.
 */
void InkBomb::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvInkBomb.Explosion)) {
        mStateExplosion->attackSensorGiga(pSelf, pOther, nullptr);
        return;
    }

    if (al::isNerve(this, &NrvInkBomb.ExplosionNoDamage)) {
        if (!al::isSensorHostName(pOther, "DarkBowser")) {
            mStateExplosion->attackSensorGiga(pSelf, pOther, nullptr);
        }

        return;
    }

    if (al::isSensorPlayer(pOther) && !al::isNerve(this, &NrvInkBomb.Kicked) &&
        al::isActionPlaying(this, "BombSign")) {
        mFuseTimer = 401;
        al::setNerve(this, &NrvInkBomb.ExplosionNoDamage);
        return;
    }

    if (!al::isSensorEnemyBody(pOther) && !al::isSensorEnemyType(pOther)) {
        return;
    }

    if (al::isNerve(this, &NrvInkBomb.Kicked)) {
        if (!rc::sendMsgBombBoundKickedAttack(pOther, pSelf)) {
            al::setNerve(this, &NrvInkBomb.Explosion);
        }

        return;
    }

    if (!al::isNerve(this, &NrvInkBomb.Roll) && !al::isNerve(this, &NrvInkBomb.Bound)) {
        return;
    }

    sead::Vector3f velocity = al::getVelocity(al::getSensorHost(pSelf));
    sead::Vector3f normal;
    al::calcVecBetweenSensors(&normal, pOther, pSelf);
    f32 lengthSqH = normal.x * normal.x + normal.z * normal.z;
    f32 radius = al::getSensorRadius(pOther) * al::getSensorRadius(pOther) * 0.5f;
    if (lengthSqH > radius) {
        normal.y = 0.0f;
    }

    al::normalizeOrDirZ(&normal);
    al::calcReflectionVector(&velocity, normal, 0.9f, 0.0f);
    al::setVelocity(al::getSensorHost(pSelf), velocity);
}

/**
 * @brief Advances the fuse warnings and explodes once the fuse has burned down.
 * @return Whether the bomb started exploding.
 */
bool InkBomb::goExplosion() {
    if (mFuseTimer == 600) {
        al::startHitReaction(this, "爆発予兆開始");
    }

    if (mFuseTimer == 300) {
        al::startAction(this, "BombSign");
    }

    if (mFuseTimer > 400) {
        al::setNerve(this, &NrvInkBomb.ExplosionNoDamage);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether a player attack message can kick the bomb.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bomb.
 * @return Whether the bomb can be kicked.
 */
bool InkBomb::canKicked(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                        al::HitSensor* pSelf) const {
    if (mStrongKickGuardTimer != 0) {
        if (al::isMsgPlayerClimbAttack(pMsg)) {
            return false;
        }

        if (al::isMsgPlayerTailAttack(pMsg)) {
            return false;
        }

        if (al::isMsgPlayerObjRollingAttack(pMsg)) {
            return false;
        }
    }

    if (al::isNerve(this, &NrvInkBomb.Wait)) {
        return false;
    }

    if (al::isNerve(this, &NrvInkBomb.Explosion)) {
        return false;
    }

    if (al::isNerve(this, &NrvInkBomb.ExplosionNoDamage)) {
        return false;
    }

    if (mFuseTimer >= 300) {
        return false;
    }

    if (al::isNerve(this, &NrvInkBomb.Kicked) && al::isLessStep(this, 5)) {
        return false;
    }

    if ((al::isNerve(this, &NrvInkBomb.Roll) || al::isNerve(this, &NrvInkBomb.Bound)) &&
        al::isLessStep(this, 5)) {
        return false;
    }

    if (al::isMsgPlayerKick(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
        al::isMsgPlayerRollingReflect(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerClimbRollingAttack(pMsg) ||
        al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
        al::isMsgPlayerTrample(pMsg)) {
        if (!al::isSensorPlayer(pOther)) {
            return false;
        }

        if (mKickerSensor == pOther) {
            return false;
        }

        return rc::tryFindRelativeControlUserId(pOther) != mControlUserId;
    }

    return false;
}

/**
 * @brief Checks whether a boomerang can hit the bomb.
 * @return Whether the bomb is in a state that reacts to boomerangs.
 */
bool InkBomb::canBoomerangHit() const {
    if (al::isNerve(this, &NrvInkBomb.Wait)) {
        return false;
    }

    if (al::isNerve(this, &NrvInkBomb.Explosion)) {
        return false;
    }

    if (al::isNerve(this, &NrvInkBomb.ExplosionNoDamage)) {
        return false;
    }

    if ((al::isNerve(this, &NrvInkBomb.Roll) || al::isNerve(this, &NrvInkBomb.Bound)) &&
        al::isLessStep(this, 5)) {
        return false;
    }

    return true;
}

/** @brief Switches the effect and sound material between ink, water and the default. */
void InkBomb::setupEffectMaterial() {
    sead::Vector3f trans = al::getTrans(this);
    f32 radius = al::getColliderRadius(this);
    if (rc::isInWaterArea(this, trans - sead::Vector3f::ey * radius)) {
        mIsInWater = true;
        if (InkUtil::isInInkLimitArrow(this, trans, radius + radius)) {
            updateMaterialCode(*this, "Ink");
        } else {
            updateMaterialCode(*this, "Water");
        }
    } else {
        updateMaterialCode(*this, "NoCode");
        mIsInWater = false;
    }
}

/** @brief Plays the landing reaction on the water surface below the bomb. */
void InkBomb::startLandHitReaction() {
    mLandReactionTimer = 10;
    f32 radius = al::getColliderRadius(this);
    sead::Vector3f surfacePos;
    WaterUtil::calcWaterSurfacePos(this, al::getTrans(this) - sead::Vector3f::ey * radius,
                                   radius + radius, &surfacePos);
    al::startHitReactionHitEffect(this, "Land", surfacePos);
}

/**
 * @brief Handles kicks, attacks and explosions sent to the bomb.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bomb.
 * @return Whether the message was handled.
 */
bool InkBomb::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mControlUserId)) {
        return true;
    }

    if (rc::isMsgBombBoundKickedAttack(pMsg)) {
        al::setNerve(this, &NrvInkBomb.Break);
        return true;
    }

    if ((al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
         al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
         al::isMsgLaserAttack(pMsg)) &&
        !al::isNerve(this, &NrvInkBomb.Bound) && !al::isNerve(this, &NrvInkBomb.Explosion) &&
        !al::isNerve(this, &NrvInkBomb.ExplosionNoDamage) && !al::isNerve(this, &NrvInkBomb.Break)) {
        al::setNerve(this, &NrvInkBomb.Explosion);
        return true;
    }

    if (al::isMsgPlayerBoomerangAttack(pMsg) && canBoomerangHit()) {
        if (!al::isNerve(this, &NrvInkBomb.Kicked) && mFuseTimer < 300 && mKickerSensor != pOther) {
            mKickerSensor = pOther;
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::startHitReaction(this, "キック");
            sead::Vector3f dir = al::getVelocity(al::getSensorHost(pOther));
            if (al::normalizeOrZero(&dir)) {
                al::calcFrontDir(&dir, al::getSensorHost(pOther));
            }

            dir.y = 0.0f;
            al::normalize(&dir);
            al::setVelocitySeparateHV(this, dir, 220.0f, 75.0f);
            mSpeedH = 220.0f;
            al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
            al::setNerve(this, &NrvInkBomb.Kicked);
            return true;
        }

        if (!al::isNerve(this, &NrvInkBomb.Explosion) &&
            !(al::isNerve(this, &NrvInkBomb.Kicked) && al::isLessStep(this, 30)) &&
            mKickerSensor != pOther) {
            al::setNerve(this, &NrvInkBomb.Explosion);
            return true;
        }
    }

    if (canKicked(pMsg, pOther, pSelf)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReaction(this, "キック");
        al::LiveActor* pKicker = al::getSensorHost(pOther);
        const al::LiveActor* pHost = mHost;
        sead::Vector3f trans = al::getTrans(this);
        Type type = mType;
        bool isStrong = al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
                        al::isMsgPlayerObjRollingAttack(pMsg);

        sead::Vector3f awayDir = trans - al::getTrans(pHost);
        sead::Vector3f front = rc::getPlayerFront(pKicker);
        awayDir.y = 0.0f;
        al::normalizeOrDirZ(&awayDir);
        sead::Vector3f dir = front;
        if (front.dot(awayDir) < -0.965f) {
            dir = -awayDir;
        }

        al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
        if (al::normalizeOrZero(&dir)) {
            dir = sead::Vector3f::ez;
        }

        f32 speedH;
        f32 speedV;
        if (isStrong) {
            speedH = type == Type_Roll ? 1200.0f : 600.0f;
            speedV = type == Type_Roll ? 120.0f : 125.0f;
        } else if (rc::isPlayerOnGround(pKicker)) {
            if (rc::isPlayerDashFast(pKicker)) {
                speedH = 450.0f;
                speedV = 100.0f;
            } else if (rc::isPlayerDash(pKicker)) {
                speedH = 400.0f;
                speedV = type == Type_Roll ? 100.0f : 10.0f;
            } else {
                speedH = 300.0f;
                speedV = 100.0f;
            }
        } else {
            speedH = 425.0f;
            speedV = 125.0f;
        }

        al::setVelocitySeparateHV(this, dir, speedH, speedV);
        mSpeedH = speedH;
        al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
        mKickerSensor = pOther;
        mControlUserId = rc::tryFindRelativeControlUserId(pOther);
        al::setNerve(this, &NrvInkBomb.Kicked);
        return true;
    }

    if (al::isMsgExplosion(pMsg) && !al::isNerve(this, &NrvInkBomb.Bound) &&
        !al::isNerve(this, &NrvInkBomb.Explosion) &&
        !al::isNerve(this, &NrvInkBomb.ExplosionNoDamage) && !al::isNerve(this, &NrvInkBomb.Break)) {
        al::setNerve(this, &NrvInkBomb.Explosion);
        return true;
    }

    return false;
}

/**
 * @brief Lets a touch-screen slide kick the bomb.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched the bomb.
 * @param pTarget Screen point target of the bomb.
 * @return Whether the touch kicked the bomb.
 */
bool InkBomb::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssist(pMsg) || al::isNerve(this, &NrvInkBomb.Wait) ||
        al::isNerve(this, &NrvInkBomb.Explosion) ||
        al::isNerve(this, &NrvInkBomb.ExplosionNoDamage) || al::isNerve(this, &NrvInkBomb.Kicked) ||
        al::isNerve(this, &NrvInkBomb.Break)) {
        return false;
    }

    al::HitSensor* pSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
    mControlUserId = rc::tryFindRelativeControlUserId(pSensor);
    mKickerSensor = pSensor;

    sead::Vector3f dir = sead::Vector3f::zero;
    al::LiveActor* pTouchActor = DrcFunction::tryFindDrcTouchActor(this, pPointer);
    if (pTouchActor != nullptr &&
        !rc::tryCalcTouchPointerSlideDirOnWorldByPointer(&dir, pTouchActor)) {
        dir = al::getScreenPointTargetPos(pTarget) - al::getHitScreenPointTargetPos(pPointer);
    }

    if (al::normalizeOrZero(&dir)) {
        al::calcFrontDir(&dir, this);
    }

    al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
    al::setVelocity(this, dir * 220.0f - al::getGravity(this) * 80.0f);
    mSpeedH = al::calcSpeedH(this);
    al::setNerve(this, &NrvInkBomb.Kicked);
    return true;
}

/** @brief Burns the fuse and spins the bomb along its movement direction. */
void InkBomb::control() {
    if (!al::isNerve(this, &NrvInkBomb.Wait) && !al::isNerve(this, &NrvInkBomb.Placed)) {
        mFuseTimer++;
    }

    if (mStrongKickGuardTimer > 0) {
        mStrongKickGuardTimer--;
    }

    if (al::isNerve(this, &NrvInkBomb.Wait) || al::isNerve(this, &NrvInkBomb.Placed) ||
        al::isNerve(this, &NrvInkBomb.Stop)) {
        return;
    }

    sead::Vector3f moment;
    sead::Vector3f dir;
    al::verticalizeVec(&dir, al::getGravity(this), al::getVelocity(this));
    if (al::normalizeOrZero(&dir)) {
        return;
    }

    f32 rate = al::lerpValue(al::calcSpeedH(this), 0.0f, mSpeedH, 0.0f, 0.05f);
    const sead::Vector3f& rGravity = al::getGravity(this);
    moment.setCross(dir * rate, rGravity);
    sead::Quatf* pQuat = al::getQuatPtr(this);
    al::rotateQuatMoment(pQuat, *pQuat, moment);
}

/** @brief Updates the ground material and plays the landing reaction when touching down. */
void InkBomb::updateCollider() {
    al::LiveActor::updateCollider();
    bool isInWaterPrev = mIsInWater;
    setupEffectMaterial();
    if (mLandReactionTimer > 0) {
        mLandReactionTimer--;
        if (mLandReactionTimer != 0) {
            return;
        }
    }

    if ((!isInWaterPrev && mIsInWater) || al::isCollidedGround(this)) {
        startLandHitReaction();
    }
}

/**
 * @brief Resets the bomb and puts it back into its generator.
 * @param isBlink Whether to blink while reloading.
 */
void InkBomb::reload(bool isBlink) {
    makeActorAppeared();
    resetInner();
    al::validateClipping(this);
    al::startAction(this, "Reload");
    if (isBlink) {
        al::startMclAnim(this, "Blink");
    }

    al::setNerve(this, &NrvInkBomb.Wait);
}

/** @brief Clears the kicker, the fuse and the movement. */
void InkBomb::resetInner() {
    mKickerSensor = nullptr;
    mControlUserId = -1;
    mFuseTimer = 0;
    mStateExplosion->reset();
    al::startAction(this, "Default");
    al::setVelocityZero(this);
    al::offCollide(this);
}

/**
 * @brief Launches a reloaded bomb.
 * @param type Movement type of the bomb.
 * @param rVelocity Launch velocity.
 * @param isStartFuse Whether the fuse starts already lit.
 * @param isUseGravity Whether gravity pulls the bomb while it bounces.
 */
void InkBomb::launch(Type type, const sead::Vector3f& rVelocity, bool isStartFuse,
                     bool isUseGravity) {
    if (!al::isNerve(this, &NrvInkBomb.Wait)) {
        return;
    }

    al::invalidateClipping(this);
    if (isStartFuse) {
        al::startHitReaction(this, "爆発予兆開始");
        mFuseTimer = 300;
    } else {
        mFuseTimer = 0;
    }

    mCollideStartStep = 5;
    al::setVelocity(this, rVelocity);
    mType = type;
    mSpeedH = al::calcSpeedH(this);
    al::setNerve(this, mType == Type_Bound ? static_cast<const al::Nerve*>(&NrvInkBomb.Bound) :
                                             &NrvInkBomb.Roll);
    mGravityRate = isUseGravity;
}

/**
 * @brief Throws the bomb from its thrower.
 * @param type Movement type of the bomb.
 * @param rVelocity Throw velocity.
 * @param gravity Gravity applied while bouncing.
 * @param isStartFuse Whether the fuse starts already lit.
 * @param isKeepVelocity Whether to use the velocity as is instead of the default throw.
 */
void InkBomb::thrown(Type type, const sead::Vector3f& rVelocity, f32 gravity, bool isStartFuse,
                     bool isKeepVelocity) {
    makeActorAppeared();
    resetInner();
    mGravity = gravity;
    mGravityRate = isKeepVelocity;
    if (isStartFuse) {
        mFuseTimer = 300;
        al::startHitReaction(this, "爆発予兆開始");
    }

    mCollideStartStep = 10;
    if (isKeepVelocity) {
        al::setVelocity(this, rVelocity);
    } else {
        mGravity = 2.5f;
        al::setVelocity(this, rVelocity * 275.0f);
    }

    al::limitVelocitySeparateHV(this, al::getGravity(this), 300.0f, 300.0f);
    mType = type;
    mSpeedH = al::calcSpeedH(this);
    al::setNerve(this, mType == Type_Bound ? static_cast<const al::Nerve*>(&NrvInkBomb.Bound) :
                                             &NrvInkBomb.Roll);
}

/** @brief Makes the bomb disappear without exploding. */
void InkBomb::vanish() {
    al::startHitReaction(this, "消滅");
    kill();
}

/**
 * @brief Checks whether the bomb is exploding or flying after a kick.
 * @return Whether the bomb is exploding or kicked.
 */
bool InkBomb::isExplodingOrKicked() const {
    return al::isNerve(this, &NrvInkBomb.Explosion) || al::isNerve(this, &NrvInkBomb.Kicked);
}

/**
 * @brief Gets the horizontal speed the bomb is thrown with.
 * @return Horizontal throw speed.
 */
f32 InkBomb::getSpeedH() const {
    return 200.0f;
}

/** @brief Bounces the bomb off collision until it explodes. */
void InkBomb::exeBound() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
    }

    if (al::isGreaterEqualStep(this, mCollideStartStep) && al::isNoCollide(this)) {
        al::onCollide(this);
    }

    if (rc::isCollidedDamageFire(this)) {
        al::setNerve(this, &NrvInkBomb.Explosion);
        return;
    }

    if (mGravityRate != 0.0f) {
        al::addVelocityToGravity(this, mGravity);
    }

    al::limitVelocitySeparateHV(this, al::getGravity(this), 300.0f, 300.0f);
    sead::Vector3f prevVelocity = al::getVelocity(this);
    if (al::reboundVelocityFromCollision(this, 0.8f, 0.0f, 1.0f)) {
        sead::Vector3f impact = prevVelocity - al::getVelocity(this);
        al::startSeWithParam(this, "Bound", impact.length(), nullptr);
        mGravityRate = 1.0f;
    }

    goExplosion();
}

/** @brief Rolls the bomb along the ground until it explodes. */
void InkBomb::exeRoll() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
    }

    if (al::isGreaterEqualStep(this, mCollideStartStep) && al::isNoCollide(this)) {
        al::onCollide(this);
    }

    if (rc::isCollidedDamageFire(this)) {
        al::setNerve(this, &NrvInkBomb.Explosion);
        return;
    }

    al::addVelocityToGravity(this, 2.5f);
    al::limitVelocitySeparateHV(this, al::getGravity(this), 300.0f, 300.0f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::holdSeWithParam(this, "Roll", al::getVelocity(this).length(), nullptr);
        mSmokePos = al::getCollidedGroundPos(this);
        al::emitEffect(this, "RollingSmoke", nullptr);
    } else {
        al::tryDeleteEffect(this, "RollingSmoke");
    }

    sead::Vector3f prevVelocity = al::getVelocity(this);
    if (al::reboundVelocityFromEachCollision(this, 0.8f, 0.97f, 1.0f, 4.0f)) {
        sead::Vector3f impact = prevVelocity - al::getVelocity(this);
        al::startSeWithParam(this, "Bound", impact.length(), nullptr);
    }

    goExplosion();
}

/** @brief Stops the rolling smoke. */
void InkBomb::endRoll() {
    al::tryDeleteEffect(this, "RollingSmoke");
}

/** @brief Flies after a kick and explodes on the first thing it hits. */
void InkBomb::exeKicked() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::startHitReaction(this, "爆発予兆開始");
        al::startAction(this, "BombSign");
        al::startHitReaction(this, "キック");
        mGravityRate = 1.0f;
    }

    al::addVelocityToGravity(this, 3.0f);
    if (rc::isCollidedDamageFire(this)) {
        al::setNerve(this, &NrvInkBomb.Explosion);
        return;
    }

    al::HitSensor* pWallSensor = al::tryGetCollidedWallSensor(this);
    al::HitSensor* pGroundSensor = al::tryGetCollidedGroundSensor(this);
    sead::Vector3f prevVelocity = al::getVelocity(this);
    if (al::reboundVelocityFromCollision(this, 0.75f, 0.0f, 1.0f)) {
        sead::Vector3f impact = prevVelocity - al::getVelocity(this);
        al::startSeWithParam(this, "Bound", impact.length(), nullptr);
        if (pWallSensor != nullptr) {
            al::sendMsgExplosionCollide(pWallSensor, al::getHitSensor(this, "Body"),
                                        rc::getExplosionComboCounter(this));
            al::setNerve(this, &NrvInkBomb.Explosion);
            return;
        }

        if (pGroundSensor != nullptr &&
            al::sendMsgExplosionCollide(pGroundSensor, al::getHitSensor(this, "Body"),
                                        rc::getExplosionComboCounter(this))) {
            al::setNerve(this, &NrvInkBomb.Explosion);
            return;
        }
    }

    if (al::isGreaterEqualStep(this, 180) || rc::isInDeathArea(this)) {
        al::setNerve(this, &NrvInkBomb.Explosion);
    }
}

/** @brief Waits in place while the fuse burns. */
void InkBomb::exeStop() {
    goExplosion();
}

/** @brief Explodes and disappears once the explosion finishes. */
void InkBomb::exeExplosion() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Explosion");
    }

    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Breaks apart and disappears once the break finishes. */
void InkBomb::exeBreak() {
    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Enables clipping while the bomb sits at its placement. */
void InkBomb::exePlaced() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
    }
}

/** @brief Disables clipping once the bomb leaves its placement. */
void InkBomb::endPlaced() {
    al::invalidateClipping(this);
}

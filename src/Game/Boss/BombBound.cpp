#include "Boss/BombBound.hpp"
#include <attributes.h>
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
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DrcUtil.hpp"
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
NERVE_END_DECL(BombBound, Placed)
NERVE_DECL(BombBound, Explosion)
NERVE_DECL(BombBound, Break)
NERVE_DECL(BombBound, Stop)
NERVE_DECL(BombBound, Kicked)
NERVE_DECL(BombBound, Wait)
NERVE_END_DECL(BombBound, Roll)
NERVE_DECL(BombBound, Bound)
NERVES_MAKE_STRUCT(BombBound, Placed, Explosion, Break, Stop, Kicked, Wait, Roll, Bound)

/**
 * @brief Computes the kick direction and speeds used when a bomb is kicked in single mode.
 * @param pDir Receives the horizontal kick direction.
 * @param pSpeedH Receives the horizontal kick speed.
 * @param pSpeedV Receives the vertical kick speed.
 * @param pKicker Actor that kicked the bomb.
 * @param type Movement type of the bomb.
 * @param isStrong Whether the kick came from a strong attack.
 * @param trans Current position of the bomb.
 */
NOINLINE void calcKickParamSingleMode(sead::Vector3f* pDir, f32* pSpeedH, f32* pSpeedV,
                                      const al::LiveActor* pKicker, BombBound::Type type,
                                      bool isStrong, sead::Vector3f trans) {
    *pDir = trans - al::getTrans(pKicker);
    pDir->normalize();
    al::verticalizeVec(pDir, sead::Vector3f::ey, *pDir);
    if (al::normalizeOrZero(pDir)) {
        *pDir = sead::Vector3f::ez;
    }

    if (type != BombBound::Type_Roll && !isStrong) {
        *pSpeedH = 25.0f;
        *pSpeedV = 16.0f;
    } else {
        *pSpeedH = isStrong ? 30.0f : 22.0f;
        *pSpeedV = isStrong ? 25.0f : 16.0f;
    }
}
}  // namespace

/**
 * @brief Constructs a bouncing bomb with its fuse idle.
 * @param pName Actor name.
 */
BombBound::BombBound(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, the explosion and break states and the rolling smoke effect.
 * @param rInfo Actor placement and scene information.
 */
void BombBound::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BombBound", nullptr);
    al::initNerve(this, &NrvBombBound.Placed, 2);
    mStateExplosion = new BombStateExplosion(this, true, nullptr);
    mStateBreak = new BombStateExplosion(this, true, nullptr);
    al::initNerveState(this, mStateExplosion, &NrvBombBound.Explosion, "爆発");
    al::initNerveState(this, mStateBreak, &NrvBombBound.Break, "破壊");
    rc::createExplosionComboCounter(this);
    al::setTrans(this, al::getTrans(this) - al::getGravity(this) * 75.0f);
    al::setEffectFollowPosPtr(this, "RollingSmoke", &mSmokePos);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    makeActorAppeared();
}

/** @brief Appears without collision until the bomb starts moving. */
void BombBound::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::offCollide(this);
}

/**
 * @brief Explodes on contact while moving and pushes or damages what it touches.
 * @param pSelf Sensor of the bomb.
 * @param pOther Sensor that was touched.
 */
void BombBound::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf)) {
        if (al::isNerve(this, &NrvBombBound.Roll) || al::isNerve(this, &NrvBombBound.Bound) ||
            al::isNerve(this, &NrvBombBound.Stop)) {
            if ((al::isSensorPlayerOrPlayerWeapon(pOther) && mFuseTimer >= 121) ||
                al::isSensorEnemyBody(pOther)) {
                al::setNerve(this, &NrvBombBound.Explosion);
                return;
            }
        }

        if (al::isNerve(this, &NrvBombBound.Kicked)) {
            if (rc::sendMsgBombBoundKickedAttack(pOther, pSelf)) {
                return;
            }

            if (al::isSensorEnemyBody(pOther)) {
                al::setNerve(this, &NrvBombBound.Explosion);
                return;
            }

            if (al::isGreaterStep(this, 10)) {
                al::ComboCounter* pCounter = rc::getExplosionComboCounter(this);
                if (rc::tryFindRelativeControlUserId(pOther) != mControlUserId &&
                    al::sendMsgExplosion(pOther, pSelf, pCounter)) {
                    al::setNerve(this, &NrvBombBound.Explosion);
                    return;
                }
            }
        }

        if (!mIsSingleMode || !al::isSensorPlessie(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if (al::isNerve(this, &NrvBombBound.Explosion)) {
        mStateExplosion->attackSensor(pSelf, pOther, nullptr);
    }
}

/**
 * @brief Checks whether a player attack message can kick the bomb.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bomb.
 * @return Whether the bomb can be kicked.
 */
bool BombBound::canKicked(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) const {
    if (al::isNerve(this, &NrvBombBound.Wait)) {
        return false;
    }

    if (al::isNerve(this, &NrvBombBound.Explosion)) {
        return false;
    }

    if (mFuseTimer >= 120 && mIsFuseEnabled) {
        return false;
    }

    if (al::isNerve(this, &NrvBombBound.Kicked) && al::isLessStep(this, 5)) {
        return false;
    }

    if ((al::isNerve(this, &NrvBombBound.Roll) || al::isNerve(this, &NrvBombBound.Bound)) &&
        al::isLessStep(this, 5)) {
        return false;
    }

    if (al::isMsgPlayerKick(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
        al::isMsgPlayerRollingReflect(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerClimbRollingAttack(pMsg) ||
        al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
        al::isMsgPlayerTrample(pMsg) || al::isMsgPush(pMsg)) {
        if (mIsSingleMode && al::isSensorRide(pOther)) {
            if (mKickerSensor == pOther) {
                return false;
            }
        } else {
            if (!al::isSensorPlayer(pOther)) {
                return false;
            }

            if (mKickerSensor == pOther) {
                return false;
            }

            if (rc::tryFindRelativeControlUserId(pOther) == mControlUserId) {
                return false;
            }
        }

        return true;
    }

    return false;
}

/**
 * @brief Checks whether a single-mode attack message can kick the bomb.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bomb.
 * @return Whether the bomb can be kicked.
 */
bool BombBound::canKickedSM(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) const {
    if (al::isNerve(this, &NrvBombBound.Wait)) {
        return false;
    }

    if (al::isNerve(this, &NrvBombBound.Explosion)) {
        return false;
    }

    if (mFuseTimer >= 120 && mIsFuseEnabled) {
        return false;
    }

    if (al::isNerve(this, &NrvBombBound.Kicked) && al::isLessStep(this, 5)) {
        return false;
    }

    if ((al::isNerve(this, &NrvBombBound.Roll) || al::isNerve(this, &NrvBombBound.Bound)) &&
        al::isLessStep(this, 5)) {
        return false;
    }

    if (al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) || al::isMsgKeyThrow(pMsg) ||
        al::isMsgNekoAttack(pMsg)) {
        return mKickerSensor != pOther;
    }

    return false;
}

/**
 * @brief Checks whether a boomerang can hit the bomb.
 * @return Whether the bomb is in a state that reacts to boomerangs.
 */
bool BombBound::canBoomerangHit() const {
    if (al::isNerve(this, &NrvBombBound.Wait)) {
        return false;
    }

    if (al::isNerve(this, &NrvBombBound.Explosion)) {
        return false;
    }

    if ((al::isNerve(this, &NrvBombBound.Roll) || al::isNerve(this, &NrvBombBound.Bound)) &&
        al::isLessStep(this, 5)) {
        return false;
    }

    return true;
}

/**
 * @brief Advances the fuse warnings and explodes once the fuse has burned down.
 * @return Whether the bomb started exploding.
 */
bool BombBound::goExplosion() {
    if (!mIsFuseEnabled) {
        return false;
    }

    if (mFuseTimer == 90) {
        al::startHitReaction(this, "爆発予兆開始");
    }

    if (mFuseTimer == 120) {
        al::startAction(this, "BombSign");
    }

    if (mFuseTimer > 220) {
        al::setNerve(this, &NrvBombBound.Explosion);
        return true;
    }

    return false;
}

/**
 * @brief Handles kicks, attacks and explosions sent to the bomb.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bomb.
 * @return Whether the message was handled.
 */
bool BombBound::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mControlUserId)) {
        return true;
    }

    if (rc::isMsgBombBoundKickedAttack(pMsg) && !al::isNerve(this, &NrvBombBound.Explosion) &&
        !al::isNerve(this, &NrvBombBound.Break)) {
        al::setNerve(this, &NrvBombBound.Break);
        return true;
    }

    if ((al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
         al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
         al::isMsgLaserAttack(pMsg)) &&
        !al::isNerve(this, &NrvBombBound.Explosion) && !al::isNerve(this, &NrvBombBound.Break)) {
        al::setNerve(this, &NrvBombBound.Explosion);
        return true;
    }

    if (al::isMsgPlayerBoomerangAttack(pMsg) && canBoomerangHit()) {
        if (!al::isNerve(this, &NrvBombBound.Kicked) && mFuseTimer < 120 &&
            mKickerSensor != pOther) {
            mKickerSensor = pOther;
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::startHitReaction(this, "キック");
            sead::Vector3f dir = al::getVelocity(al::getSensorHost(pOther));
            if (al::normalizeOrZero(&dir)) {
                al::calcFrontDir(&dir, al::getSensorHost(pOther));
            }

            dir.y = 0.0f;
            al::normalize(&dir);
            al::setVelocitySeparateHV(this, dir, 22.0f, 15.0f);
            mSpeedH = 22.0f;
            al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
            al::setNerve(this, &NrvBombBound.Kicked);
            return true;
        }

        if (!al::isNerve(this, &NrvBombBound.Explosion) &&
            !(al::isNerve(this, &NrvBombBound.Kicked) && al::isLessStep(this, 30)) &&
            mKickerSensor != pOther) {
            al::setNerve(this, &NrvBombBound.Explosion);
            return true;
        }
    }

    if (canKicked(pMsg, pOther, pSelf) && !al::isSensorRide(pOther)) {
        mKickerSensor = pOther;
        mControlUserId = rc::tryFindRelativeControlUserId(pOther);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReaction(this, "キック");
        al::LiveActor* pKicker = al::getSensorHost(pOther);
        Type type = mType;
        bool isStrong = al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
                        al::isMsgPlayerObjRollingAttack(pMsg);
        sead::Vector3f dir = rc::getPlayerFront(pKicker);
        al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
        if (al::normalizeOrZero(&dir)) {
            dir = sead::Vector3f::ez;
        }

        f32 speedH;
        f32 speedV;
        if (isStrong) {
            speedH = 30.0f;
            speedV = 25.0f;
        } else if (rc::isPlayerOnGround(pKicker)) {
            if (rc::isPlayerDashFast(pKicker)) {
                speedH = type == Type_Roll ? 25.0f : 30.0f;
                speedV = 16.0f;
            } else if (rc::isPlayerDash(pKicker)) {
                speedH = type == Type_Roll ? 23.0f : 30.0f;
                speedV = 16.0f;
            } else {
                speedH = type == Type_Roll ? 22.0f : 25.0f;
                speedV = 16.0f;
            }
        } else {
            speedH = type == Type_Roll ? 23.0f : 25.0f;
            speedV = type == Type_Roll ? 18.0f : 30.0f;
        }

        al::setVelocitySeparateHV(this, dir, speedH, speedV);
        mSpeedH = speedH;
        al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
        al::setNerve(this, &NrvBombBound.Kicked);
        return true;
    }

    if ((mIsSingleMode && al::isSensorRide(pOther) && al::isMsgPush(pMsg) &&
         canKicked(pMsg, pOther, pSelf)) ||
        (mIsSingleMode && canKickedSM(pMsg, pOther, pSelf))) {
        mKickerSensor = pOther;
        mControlUserId = rc::tryFindRelativeControlUserId(pOther);
        al::startHitReaction(this, "キック");
        al::LiveActor* pKicker = al::getSensorHost(pOther);
        sead::Vector3f trans = al::getTrans(this);
        Type type = mType;
        bool isStrong = al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
                        al::isMsgPlayerObjRollingAttack(pMsg);
        sead::Vector3f dir;
        f32 speedH;
        f32 speedV;
        calcKickParamSingleMode(&dir, &speedH, &speedV, pKicker, type, isStrong, trans);
        al::setVelocitySeparateHV(this, dir, speedH, speedV);
        mSpeedH = speedH;
        al::makeQuatFrontNoSupport(al::getQuatPtr(this), dir);
        al::setNerve(this, &NrvBombBound.Kicked);
        return true;
    }

    if (mIsSingleMode && al::isMsgGigaEnemyAttack(pMsg) &&
        !al::isNerve(this, &NrvBombBound.Explosion) && !al::isNerve(this, &NrvBombBound.Break)) {
        al::setNerve(this, &NrvBombBound.Explosion);
        return true;
    }

    if (al::isMsgPlayerSpinAttack(pMsg) && al::isSensorKoopaJr(pOther)) {
        al::setVelocityBlowAttack(this, al::getSensorPos(pOther), 20.0f, 5.0f);
        return true;
    }

    if (al::isMsgExplosion(pMsg) && !al::isNerve(this, &NrvBombBound.Explosion) &&
        !al::isNerve(this, &NrvBombBound.Break)) {
        al::setNerve(this, &NrvBombBound.Explosion);
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
bool BombBound::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssist(pMsg) || al::isNerve(this, &NrvBombBound.Wait) ||
        al::isNerve(this, &NrvBombBound.Explosion) || al::isNerve(this, &NrvBombBound.Kicked) ||
        al::isNerve(this, &NrvBombBound.Break)) {
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
    al::setVelocity(this, dir * 22.0f - al::getGravity(this) * 16.0f);
    mSpeedH = al::calcSpeedH(this);
    al::setNerve(this, &NrvBombBound.Kicked);
    return true;
}

/** @brief Burns the fuse and spins the bomb along its movement direction. */
void BombBound::control() {
    if (!al::isNerve(this, &NrvBombBound.Wait) && !al::isNerve(this, &NrvBombBound.Placed)) {
        mFuseTimer++;
    }

    if (al::isNerve(this, &NrvBombBound.Wait) || al::isNerve(this, &NrvBombBound.Placed) ||
        al::isNerve(this, &NrvBombBound.Stop)) {
        return;
    }

    sead::Vector3f dir;
    al::verticalizeVec(&dir, al::getGravity(this), al::getVelocity(this));
    if (al::normalizeOrZero(&dir)) {
        return;
    }

    f32 rate = al::lerpValue(al::calcSpeedH(this), 0.0f, mSpeedH, 0.0f, 0.2f);
    const sead::Vector3f& rGravity = al::getGravity(this);
    sead::Vector3f moment;
    moment.setCross(dir * rate, rGravity);
    sead::Quatf* pQuat = al::getQuatPtr(this);
    al::rotateQuatMoment(pQuat, *pQuat, moment);
}

/**
 * @brief Resets the bomb and puts it back into its generator.
 * @param isBlink Whether to blink while reloading.
 */
void BombBound::reload(bool isBlink) {
    makeActorAppeared();
    resetInner();
    al::validateClipping(this);
    al::startAction(this, "Reload");
    if (isBlink) {
        al::startMclAnim(this, "Blink");
    }

    al::setNerve(this, &NrvBombBound.Wait);
}

/** @brief Clears the kicker, the fuse and the movement. */
void BombBound::resetInner() {
    mKickerSensor = nullptr;
    mControlUserId = -1;
    mFuseTimer = 0;
    mStateExplosion->reset();
    al::startAction(this, "Default");
    al::setVelocityZero(this);
    al::offCollide(this);
}

/**
 * @brief Launches a reloaded bomb out of its generator.
 * @param type Movement type of the bomb.
 * @param rVelocity Launch velocity.
 * @param isStartFuse Whether the fuse starts already lit.
 */
void BombBound::launch(Type type, const sead::Vector3f& rVelocity, bool isStartFuse) {
    if (!al::isNerve(this, &NrvBombBound.Wait)) {
        return;
    }

    al::invalidateClipping(this);
    if (isStartFuse) {
        al::startHitReaction(this, "爆発予兆開始");
        mFuseTimer = 120;
    } else {
        mFuseTimer = 0;
    }

    mCollideStartStep = 10;
    al::setVelocity(this, rVelocity);
    mType = type;
    mSpeedH = al::calcSpeedH(this);
    al::setNerve(this, mType == Type_Bound ? static_cast<const al::Nerve*>(&NrvBombBound.Bound) :
                                             &NrvBombBound.Roll);
}

/**
 * @brief Throws the bomb from an arbitrary thrower.
 * @param type Movement type of the bomb.
 * @param rVelocity Throw velocity.
 * @param isStartFuse Whether the fuse starts already lit.
 */
void BombBound::thrown(Type type, const sead::Vector3f& rVelocity, bool isStartFuse) {
    makeActorAppeared();
    resetInner();
    if (isStartFuse) {
        mFuseTimer = 120;
        al::startHitReaction(this, "爆発予兆開始");
    }

    mCollideStartStep = 35;
    al::setVelocity(this, rVelocity);
    mType = type;
    mSpeedH = al::calcSpeedH(this);
    al::setNerve(this, mType == Type_Bound ? static_cast<const al::Nerve*>(&NrvBombBound.Bound) :
                                             &NrvBombBound.Roll);
}

/** @brief Makes the bomb disappear without exploding. */
void BombBound::vanish() {
    al::startHitReaction(this, "消滅");
    kill();
}

/**
 * @brief Checks whether the bomb is exploding or flying after a kick.
 * @return Whether the bomb is exploding or kicked.
 */
bool BombBound::isExplodingOrKicked() const {
    return al::isNerve(this, &NrvBombBound.Explosion) || al::isNerve(this, &NrvBombBound.Kicked);
}

/** @brief Bounces the bomb off collision until it explodes. */
void BombBound::exeBound() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
    }

    if (al::isGreaterEqualStep(this, mCollideStartStep) && al::isNoCollide(this)) {
        al::onCollide(this);
    }

    if (rc::isCollidedDamageFire(this) || (mIsSingleMode && rc::isInWaterArea(this))) {
        al::setNerve(this, &NrvBombBound.Explosion);
        return;
    }

    al::addVelocityToGravity(this, 0.5f);
    al::limitVelocitySeparateHV(this, al::getGravity(this), 15.0f, 20.0f);
    sead::Vector3f prevVelocity = al::getVelocity(this);
    if (al::reboundVelocityFromCollision(this, 0.5f, 0.0f, 1.0f)) {
        sead::Vector3f impact = prevVelocity - al::getVelocity(this);
        al::startSeWithParam(this, "Bound", impact.length(), nullptr);
    }

    goExplosion();
}

/** @brief Rolls the bomb along the ground until it explodes. */
void BombBound::exeRoll() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
    }

    if (al::isGreaterEqualStep(this, mCollideStartStep) && al::isNoCollide(this)) {
        al::onCollide(this);
    }

    if (rc::isCollidedDamageFire(this) || (mIsSingleMode && rc::isInWaterArea(this))) {
        al::setNerve(this, &NrvBombBound.Explosion);
        return;
    }

    if (!mIsFuseEnabled && rc::isInAreaObj(this, rc::AreaObjType::BombBoundBreakArea)) {
        if (!al::isNerve(this, &NrvBombBound.Explosion)) {
            al::setNerve(this, &NrvBombBound.Explosion);
        }

        return;
    }

    al::addVelocityToGravity(this, 0.5f);
    al::limitVelocitySeparateHV(this, al::getGravity(this), 15.0f, 20.0f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::holdSeWithParam(this, "Roll", al::getVelocity(this).length(), nullptr);
        sead::Vector3f groundPos = al::getCollidedGroundPos(this);
        mSmokePos = groundPos;
        al::emitEffect(this, "RollingSmoke", nullptr);
    } else {
        al::tryDeleteEffect(this, "RollingSmoke");
    }

    sead::Vector3f prevVelocity = al::getVelocity(this);
    if (al::reboundVelocityFromEachCollision(this, 0.45f, 0.97f, 1.0f, 4.0f)) {
        sead::Vector3f impact = prevVelocity - al::getVelocity(this);
        al::startSeWithParam(this, "Bound", impact.length(), nullptr);
    }

    goExplosion();
}

/** @brief Stops the rolling smoke. */
void BombBound::endRoll() {
    al::tryDeleteEffect(this, "RollingSmoke");
}

/** @brief Flies after a kick and explodes on the first thing it hits. */
void BombBound::exeKicked() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::startHitReaction(this, "爆発予兆開始");
        al::startAction(this, "BombSign");
        al::startHitReaction(this, "キック");
    }

    al::addVelocityToGravity(this, 0.67f);
    if (rc::isCollidedDamageFire(this) || (mIsSingleMode && rc::isInWaterArea(this))) {
        al::setNerve(this, &NrvBombBound.Explosion);
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
            al::setNerve(this, &NrvBombBound.Explosion);
            return;
        }

        if (pGroundSensor != nullptr &&
            al::sendMsgExplosionCollide(pGroundSensor, al::getHitSensor(this, "Body"),
                                        rc::getExplosionComboCounter(this))) {
            al::setNerve(this, &NrvBombBound.Explosion);
            return;
        }
    }

    if (al::isGreaterEqualStep(this, 180) || rc::isInDeathArea(this)) {
        al::setNerve(this, &NrvBombBound.Explosion);
    }
}

/** @brief Waits in place while the fuse burns. */
void BombBound::exeStop() {
    goExplosion();
}

/** @brief Explodes and disappears once the explosion finishes. */
void BombBound::exeExplosion() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Explosion");
    }

    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Breaks apart and disappears once the break finishes. */
void BombBound::exeBreak() {
    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Enables clipping while the bomb sits at its placement. */
void BombBound::exePlaced() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
    }
}

/** @brief Disables clipping once the bomb leaves its placement. */
void BombBound::endPlaced() {
    al::invalidateClipping(this);
}

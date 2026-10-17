#include "Enemy/FireBrosFireBall.hpp"
#include "Enemy/BrosFunction.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(FireBrosFireBall, Move)
NERVE_DECL(FireBrosFireBall, RouteDokan)
NERVES_MAKE_STRUCT(FireBrosFireBall, Move, RouteDokan)
}

/** @brief Creates a reusable fireball. @param pName Actor name. */
FireBrosFireBall::FireBrosFireBall(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the model and pipe travel state. @param rInfo Scene information. */
void FireBrosFireBall::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "FireBrosFireBall", nullptr);
    al::initNerve(this, &NrvFireBrosFireBall.Move, 1);
    al::invalidateClipping(this);
    mRouteDokan = new ActorStateRouteDokanMove(this, rInfo);
    al::initNerveState(this, mRouteDokan, &NrvFireBrosFireBall.RouteDokan,
                       "[state]\u30eb\u30fc\u30c8\u571f\u7ba1\u79fb\u52d5");
    makeActorDead();
}

/** @brief Updates collision and optionally extends the shadow to the floor. */
void FireBrosFireBall::updateCollider() {
    al::LiveActor::updateCollider();
    if (mUpdateShadow) {
        const sead::Vector3f& rTrans = al::getTrans(this);
        sead::Vector3f start = sead::Vector3f::ey * 100.0f + rTrans;
        sead::Vector3f direction = sead::Vector3f::ey * -1000.0f;
        sead::Vector3f hit;
        if (alCollisionUtil::getFirstPolyOnArrow(this, &hit, nullptr, start, direction,
                                               nullptr, nullptr)) {
            f32 length = rTrans.y - hit.y + 40.0f;
            al::setShadowDropLength(this, length < 50.0f ? 50.0f : length);
        }
    }
}

/** @brief Plays the appropriate hit reaction and removes the fireball. */
inline void FireBrosFireBall::hitDisappear() {
    if (al::isNerve(this, &NrvFireBrosFireBall.RouteDokan)) {
        al::startHitReaction(this, "\u6d88\u6ec5\u3010\u30eb\u30fc\u30c8\u571f\u7ba1\u5185\u3011");
    } else {
        al::startHitReactionHit(this);
    }
    kill();
}

/** @brief Attacks contacted actors or starts pipe travel.
 * @param pSelf Fireball sensor. @param pOther Contacted sensor. */
void FireBrosFireBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf)) {
        if (al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) {
            if (al::sendMsgEnemyAttackFire(pOther, pSelf)) {
                hitDisappear();
            }
        } else if (al::isSensorEnemy(pOther)) {
            if (al::sendMsgVanish(pOther, pSelf) ||
                (!al::isMySensor(pOther, mHost) && al::sendMsgEnemyAttackFire(pOther, pSelf))) {
                hitDisappear();
            }
        } else if (al::isSensorMapObj(pOther)) {
            if (al::sendMsgEnemyAttackFire(pOther, pSelf)) {
                hitDisappear();
            }
        } else if (GameDataFunction::isSingleMode(this) && al::isSensorKickKoura(pOther) &&
                   al::sendMsgEnemyAttackFire(pOther, pSelf)) {
            hitDisappear();
        }
    } else if (al::isNerve(this, &NrvFireBrosFireBall.Move) && mRouteDokan->tryStart(pSelf, pOther)) {
        al::offCollide(this);
        al::setNerve(this, &NrvFireBrosFireBall.RouteDokan);
    }
}

/** @brief Handles attacks that extinguish the fireball.
 * @param pMsg Incoming message. @param pOther Sender. @param pSelf Receiver.
 * @return Whether the message is acknowledged. */
bool FireBrosFireBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (al::isMsgVanish(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
        (al::isNerve(this, &NrvFireBrosFireBall.RouteDokan) && al::isMsgBallAttack(pMsg)) ||
        al::isMsgNekoAttack(pMsg)) {
        al::startHitReactionDisappear(this);
        kill();
        return true;
    }
    if (GameDataFunction::isSingleMode(this)) {
        if (rc::isMsgPackunEatStart(pMsg)) {
            return true;
        }
        if (rc::isMsgPackunEat(pMsg)) {
            al::startHitReactionDisappear(this);
            kill();
        }
    }
    return false;
}

/** @brief Launches a fireball from a hand transform.
 * @param pHost Throwing actor. @param pMtx Hand transform. @param rOffset Position offset.
 * @param rate Interpolation between the two throw trajectories. */
void FireBrosFireBall::shoot(const al::LiveActor* pHost, const sead::Matrix34f* pMtx,
                           const sead::Vector3f& rOffset, f32 rate) {
    mHost = pHost;
    mShotRate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
    sead::Vector3f front;
    al::calcFrontDir(&front, pHost);
    sead::Vector3f up;
    al::calcUpDir(&up, pHost);
    mLifeStep = 0;
    mHasBounced = false;
    mIsAppearing = true;
    rc::updateWeaponPose(this, pMtx, rOffset, sead::Vector3f::zero);
    sead::Vector3f velocity = al::lerpValue(mShotRate, 17.0f, 16.0f) * front;
    al::setVelocity(this, velocity + al::lerpValue(mShotRate, -10.0f, 0.0f) * up);
    al::setNerve(this, &NrvFireBrosFireBall.Move);
    al::onCollide(this);
    al::startAction(this, "Appear");
    makeActorAppeared();
    al::startSe(this, "Shoot", nullptr);
    al::startSe(this, "Moving", nullptr);
}

/** @brief Changes the appearance animation to its flight loop once it ends. */
inline void FireBrosFireBall::updateAppearAction() {
    bool isAppearing = mIsAppearing;
    if (isAppearing && al::isActionEnd(this)) {
        al::startAction(this, "Shot");
        isAppearing = false;
    }
    mIsAppearing = isAppearing;
}

/** @brief Moves, bounces and turns the fireball along its flight path. */
void FireBrosFireBall::exeMove() {
    updateAppearAction();
    sead::Vector3f horizontal = al::getVelocity(this);
    horizontal.y = 0.0f;
    f32 minSpeed = mHasBounced ? 8.0f : al::lerpValue(mShotRate, 8.0f, 4.0f);
    if (horizontal.length() > minSpeed) {
        horizontal = (mHasBounced ? 0.997f : al::lerpValue(mShotRate, 0.997f, 0.997f)) * horizontal;
    }
    if (horizontal.length() < minSpeed) {
        f32 length = horizontal.length();
        if (length > 0.0f) {
            horizontal = (minSpeed / length) * horizontal;
        }
    }
    al::getVelocityPtr(this)->x = horizontal.x;
    al::getVelocityPtr(this)->z = horizontal.z;
    if (al::isStep(this, 1)) {
        al::onCollide(this);
    }
    if (al::isOnGround(this, 0, 0.0f)) {
        mHasBounced = true;
        if (rc::isCollidedDamageFire(this)) {
            al::startHitReactionDisappear(this);
            al::startSe(this, "Vanish", nullptr);
            kill();
            return;
        }
        al::getVelocityPtr(this)->y = 15.0f;
        al::startSe(this, "Bound", nullptr);
        al::startHitReaction(this, "\u5e8a\u63a5\u89e6");
    } else {
        if (al::isCollidedWall(this)) {
            al::HitSensor* pWall = al::tryGetCollidedWallSensor(this);
            if (pWall) {
                al::sendMsgEnemyAttackFire(pWall, al::getHitSensor(this, "Attack"));
            }
            al::startHitReactionDisappear(this);
            al::startSe(this, "Vanish", nullptr);
            kill();
            return;
        }
        if (al::isCollidedCeiling(this) && al::getVelocity(this).z > 0.0f) {
            al::getVelocityPtr(this)->y = 0.0f;
            al::startSe(this, "Bound", nullptr);
        }
        sead::Vector3f* pVelocity = al::getVelocityPtr(this);
        *pVelocity += sead::Vector3f(0.0f,
            -(mHasBounced ? 1.6f : al::lerpValue(mShotRate, 1.0f, 0.6f)), 0.0f);
    }
    if (++mLifeStep >= 180) {
        al::startHitReactionDisappear(this);
        al::startSe(this, "Vanish", nullptr);
        kill();
        return;
    }
    sead::Vector3f front = al::getVelocity(this);
    if (!al::normalizeOrZero(&front) && !al::isParallelDirection(front, al::getGravity(this), 0.01f)) {
        sead::Quatf quat;
        al::makeQuatUpFront(&quat, -al::getGravity(this), front);
        al::updatePoseQuat(this, quat);
    }
}

/** @brief Advances pipe travel and resumes free flight at the exit. */
void FireBrosFireBall::exeRouteDokan() {
    updateAppearAction();
    if (al::isIntervalStep(this, 3, 0) && ++mLifeStep >= 180) {
        al::startHitReactionDisappear(this);
        kill();
        return;
    }
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvFireBrosFireBall.Move);
    }
}

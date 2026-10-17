#include "Player/Normal/PlayerFireBall.hpp"

#include <math/seadMatrix.h>
#include <math/seadQuat.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Player/Normal/PlayerFireBallAppearWatchFunction.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_ACTION_IMPL(PlayerFireBall, Move)
NERVE_ACTION_IMPL_(PlayerFireBall, MoveWithLowEffect, Move)
NERVE_ACTION_IMPL(PlayerFireBall, RouteDokan)
NERVE_ACTION_IMPL(PlayerFireBall, Dead)
NERVE_ACTION_IMPL(PlayerFireBall, DeadDeathCode)
NERVE_ACTION_IMPL(PlayerFireBall, DeadNoEffect)
NERVE_ACTION_IMPL(PlayerFireBall, DeadInRouteDokan)

NERVE_ACTIONS_MAKE_STRUCT(PlayerFireBall, Move, MoveWithLowEffect, RouteDokan, Dead, DeadDeathCode,
                          DeadNoEffect, DeadInRouteDokan)

/// World up direction, used to tell floors and ceilings from walls when reflecting.
const sead::Vector3f sUpDir(0.0f, 1.0f, 0.0f);
}  // namespace

/**
 * @brief Creates the fire ball actor.
 * @param rName Name of the actor.
 * @param pProperty Physical state of the player throwing the ball.
 * @param pPlayer The player actor throwing the ball.
 * @param rInfo Actor init info.
 * @param pPlayerSensor Sensor of the player, used to answer control-user and host queries.
 */
PlayerFireBall::PlayerFireBall(const sead::SafeString& rName, const PlayerProperty* pProperty,
                               al::LiveActor* pPlayer, const al::ActorInitInfo& rInfo,
                               al::HitSensor* pPlayerSensor)
    : al::LiveActor(rName.cstr()), mProperty(pProperty), mPlayer(pPlayer),
      mPlayerSensor(pPlayerSensor) {
    al::initCreateActorNoPlacementInfo(this, rInfo);
}

/**
 * @brief Initializes the actions, the route pipe state and the model, and leaves the ball dead.
 * @param rInfo Actor init info.
 */
void PlayerFireBall::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    al::initNerveAction(this, "Move", &NrvPlayerFireBall.collector, 1);
    mStateRouteDokan = new ActorStateRouteDokanMove(this, rInfo);
    al::initNerveState(this, mStateRouteDokan, NrvPlayerFireBall.RouteDokan.data(),
                       "[state]ルート土管移動");

    if (mIsSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "PlayerFireBall", "SM");
    } else {
        al::initActorWithArchiveName(this, rInfo, "PlayerFireBall", nullptr);
    }

    al::killPrePassLight(this, "炎", -1);
    al::killPrePassLight(this, "炎(軽量版)", -1);
    makeActorDead();
}

/**
 * @brief Kills the ball, unregisters it from the appear watcher and turns its lights off.
 */
void PlayerFireBall::kill() {
    al::LiveActor::kill();
    PlayerFireBallAppearWatchFunction::registKillFireBall(this);
    al::killPrePassLight(this, "炎", -1);
    al::killPrePassLight(this, "炎(軽量版)", -1);
}

/**
 * @brief Does nothing; the ball is driven by its nerves.
 */
void PlayerFireBall::control() {}

/**
 * @brief Enters route pipes and attacks the touched sensor while moving.
 * @param pSelf The ball's sensor.
 * @param pOther The touched sensor.
 */
void PlayerFireBall::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isMove() && mStateRouteDokan->tryStart(pSelf, pOther)) {
        al::offCollide(this);
        al::startNerveAction(this, "RouteDokan");
        return;
    }

    if (isMove() && al::isGreaterEqualStep(this, 1) &&
        al::sendMsgPlayerFireBallAttack(pOther, pSelf)) {
        al::startNerveAction(this, "DeadNoEffect");
        return;
    }

    if (al::isNerve(this, NrvPlayerFireBall.RouteDokan.data()) && al::isGreaterEqualStep(this, 1) &&
        al::sendMsgPlayerRouteDokanFireBallAttack(pOther, pSelf)) {
        al::startNerveAction(this, "DeadNoEffect");
    }
}

/**
 * @brief Checks whether the ball is flying around (normal or low-effect move).
 * @return Whether the ball is moving.
 */
bool PlayerFireBall::isMove() const {
    return al::isNerve(this, NrvPlayerFireBall.Move.data()) ||
           al::isNerve(this, NrvPlayerFireBall.MoveWithLowEffect.data());
}

/**
 * @brief Handles demo starts, player queries, vanish requests and wind.
 * @param pMsg The received message.
 * @param pOther The sending sensor.
 * @param pSelf The ball's sensor.
 * @return Whether the message was handled.
 */
bool PlayerFireBall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                al::HitSensor* pSelf) {
    if (rc::isMsgStartGoalDemoPole(pMsg)) {
        kill();
        return true;
    }

    if (rc::isMsgStartGoalDemoHouse(pMsg) || rc::isMsgStartDemoBossStart(pMsg)) {
        makeActorDead();
        return true;
    }

    if (rc::isMsgAskControlUserId(pMsg, mPlayerSensor)) {
        return true;
    }

    if (rc::isMsgQueryHostPlayer(pMsg)) {
        return rc::isEqualHostPlayer(pMsg, al::getSensorHost(mPlayerSensor));
    }

    if (al::isMsgVanish(pMsg)) {
        if (al::isNerve(this, NrvPlayerFireBall.RouteDokan.data())) {
            al::startNerveAction(this, "DeadInRouteDokan");
        } else {
            al::startNerveAction(this, "Dead");
        }

        return true;
    }

    if (al::isNerve(this, NrvPlayerFireBall.Move.data())) {
        sead::Vector3f windPower = sead::Vector3f::zero;

        if (rc::tryGetWindPower(&windPower, pMsg)) {
            al::addVelocity(this, windPower * 0.3f);
            return true;
        }
    }

    return false;
}

/**
 * @brief Throws the ball from the player's right hand in the player's front direction.
 */
void PlayerFireBall::shoot() {
    sead::Vector3f pos;
    al::calcJointPos(&pos, mPlayer, "HandR");

    sead::Vector3f front = mProperty->getFront();
    sead::Vector3f up = mProperty->getUpDir();

    if (al::isParallelDirection(front, up, 0.01f)) {
        up.set(mProperty->getGroundUp());
    }

    sead::Vector3f hitPos;
    al::Triangle triangle;

    if (alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle, pos - front * 80.0f,
                                             front * 80.0f, nullptr, nullptr)) {
        const sead::Vector3f& normal = *triangle.getNormal(0);
        f32 radius = al::getSensorRadius(this);
        pos = hitPos + normal * radius;
    }

    sead::Vector3f side;
    side.setCross(up, front);
    al::normalize(&side);
    front.setCross(side, up);

    sead::Matrix34f mtx;
    mtx.setBase(0, side);
    mtx.setBase(1, up);
    mtx.setBase(2, front);
    mtx.setTranslation(pos);
    al::updatePoseMtx(this, &mtx);

    bool isInWater = rc::isInWaterArea(this, pos);
    f32 frontSpeed = 32.5f;
    f32 downSpeed = 15.0f;

    if (isInWater) {
        frontSpeed = 26.5f;
        downSpeed = 10.0f;
    }

    sead::Vector3f velocity = front * frontSpeed - up * downSpeed;
    mIsInWater = isInWater;
    al::setVelocity(this, velocity);

    mAppearType = PlayerFireBallAppearWatchFunction::registAppearFireBall(this);

    switch (mAppearType) {
    case PlayerFireBallAppearType::Normal:
        al::appearPrePassLight(this, "炎", -1);
        break;
    case PlayerFireBallAppearType::LowEffect:
        al::appearPrePassLight(this, "炎(軽量版)", -1);
        break;
    }

    startMoveAction();
    mStep = 0;
    makeActorAppeared();
    al::startAction(this, "Shot");
    al::startSe(this, "PgShoot");
    mIsEnableGravity = false;
}

/**
 * @brief Starts the move action matching the appear type (low effect when many balls are out).
 */
void PlayerFireBall::startMoveAction() {
    if (mAppearType != PlayerFireBallAppearType::Normal) {
        al::startNerveAction(this, "MoveWithLowEffect");
    } else {
        al::startNerveAction(this, "Move");
    }
}

/**
 * @brief Checks whether the ball is gone.
 * @return Whether the ball is dead.
 */
bool PlayerFireBall::isVanished() const {
    return al::isDead(this);
}

/**
 * @brief Makes the ball vanish immediately.
 */
void PlayerFireBall::forceEnd() {
    al::startSe(this, "PgVanish");
    al::emitEffect(this, "Disappear", nullptr);
    kill();
}

/**
 * @brief Moves the ball: notifies touched sensors, handles water, limits the horizontal speed and
 * bounces it off the collision.
 */
void PlayerFireBall::exeMove() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
    }

    if (al::HitSensor* wallSensor = al::tryGetCollidedWallSensor(this)) {
        al::sendMsgFireBalCollide(wallSensor, al::getHitSensor(this, "Body"));

        if (mIsSingleMode) {
            rc::emitEcho(this, al::getTrans(this), 200.0f, 30, false);
        }
    }

    if (al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this)) {
        al::sendMsgFireBallFloorTouch(groundSensor, al::getHitSensor(this, "Body"));

        if (mIsSingleMode) {
            rc::emitEcho(this, al::getTrans(this), 200.0f, 30, false);
        }
    }

    al::AreaObj* waterArea =
        rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea, al::getTrans(this));
    bool isInWater = waterArea != nullptr;
    al::updateEffectMaterialWater(this, isInWater);
    al::updateSeMaterialWater(this, isInWater);
    rc::startHitReactionIfThroughWater(this);

    if (waterArea != nullptr) {
        if (!mIsInWater) {
            al::startSe(this, "PgWaterIn");
        }
    } else if (mIsInWater) {
        al::startSe(this, "PgWaterOut");
    }

    mIsInWater = isInWater;
    mStep++;

    if (mStep >= (waterArea != nullptr ? 120u : 90u)) {
        al::startNerveAction(this, "Dead");
        return;
    }

    if ((al::isCollided(this) && (rc::isCollidedDamageFire(this) || rc::isCollidedPoison(this))) ||
        (mIsSingleMode && InkUtil::isInInkLimitSphere(this))) {
        al::startNerveAction(this, "DeadDeathCode");
        return;
    }

    if (al::isCollided(this)) {
        mIsEnableGravity = true;
    }

    f32 maxSpeed = waterArea != nullptr ? 12.5f : 14.0f;
    sead::Vector3f hVelocity = al::getVelocity(this);
    hVelocity.y = 0.0f;

    if (hVelocity.length() > maxSpeed) {
        hVelocity *= 0.997f;

        if (hVelocity.length() < maxSpeed) {
            f32 length = hVelocity.length();

            if (length > 0.0f) {
                hVelocity *= maxSpeed / length;
            }
        }
    }

    al::getVelocityPtr(this)->x = hVelocity.x;
    al::getVelocityPtr(this)->z = hVelocity.z;

    if (al::isCollidedGround(this) && al::getVelocity(this).y < 0.0f) {
        if (al::getOnGroundNormal(this, 0).dot(sead::Vector3f::ey) > 0.5f) {
            boundGround(al::getCollidedGroundPos(this));
            turn(al::getVelocity(this));
            return;
        }

        boundWall(al::getOnGroundNormal(this, 0), al::getCollidedGroundPos(this));
    } else if (al::isCollidedWall(this)) {
        boundWall(al::getCollidedWallNormal(this), al::getCollidedWallPos(this));
    } else if (al::isCollidedCeiling(this) && al::getVelocity(this).y > 0.0f) {
        boundRoof();
    }

    applyGravity();
    turn(al::getVelocity(this));
}

/**
 * @brief Bounces the ball up off the floor.
 * @param rPos Position of the bounce.
 */
void PlayerFireBall::boundGround(const sead::Vector3f& rPos) {
    al::startSe(this, "PgBound");
    al::getVelocityPtr(this)->y = 15.0f;

    if (PlayerFireBallAppearWatchFunction::isEmittableBoundEffect(this)) {
        al::startHitReactionHitEffect(this, "床バウンド", rPos);
    }
}

/**
 * @brief Reflects the ball's horizontal velocity off a wall.
 * @param rNormal Normal of the wall.
 * @param rPos Position of the bounce.
 */
void PlayerFireBall::boundWall(const sead::Vector3f& rNormal, const sead::Vector3f& rPos) {
    sead::Vector3f normal = rNormal;
    al::verticalizeVec(&normal, sead::Vector3f(0.0f, 1.0f, 0.0f), normal);

    if (al::normalizeOrZero(&normal)) {
        return;
    }

    f32 dot = normal.dot(al::getVelocity(this)) * 2.0f;

    if (dot < 0.0f) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(this);
        *pVelocity -= normal * dot;

        if (PlayerFireBallAppearWatchFunction::isEmittableBoundEffect(this)) {
            al::startHitReactionHitEffect(this, "壁バウンド", rPos);
        }

        al::startSe(this, "PgBound");
    }
}

/**
 * @brief Applies the gravity once the ball has touched the collision.
 */
void PlayerFireBall::applyGravity() {
    if (mIsEnableGravity) {
        *al::getVelocityPtr(this) += sead::Vector3f(0.0f, -2.0f, 0.0f);
    }
}

/**
 * @brief Stops the ball's upward movement against a ceiling.
 */
void PlayerFireBall::boundRoof() {
    al::getVelocityPtr(this)->y = 0.0f;
}

/**
 * @brief Turns the ball halfway towards a direction.
 * @param rDir The direction to face.
 */
void PlayerFireBall::turn(const sead::Vector3f& rDir) {
    sead::Vector3f front = rDir;

    if (al::normalizeOrZero(&front)) {
        return;
    }

    sead::Vector3f up = -al::getGravity(this);
    sead::Quatf quat;
    al::calcQuat(&quat, this);

    sead::Quatf nextQuat;

    if (al::isParallelDirection(front, up, 0.01f)) {
        sead::Vector3f currentFront;
        al::calcFrontDir(&currentFront, this);
        al::makeQuatRotationRate(&nextQuat, currentFront, front, 0.5f);
        nextQuat = nextQuat * quat;
    } else {
        al::makeQuatFrontUp(&nextQuat, front, up);
        al::slerpQuat(&nextQuat, quat, nextQuat, 0.5f);
    }

    al::updatePoseQuat(this, nextQuat);
}

/**
 * @brief Moves the ball through a route pipe and starts moving again when it comes out.
 */
void PlayerFireBall::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgRouteDokanIn");
    }

    if (al::isIntervalStep(this, 3, 0)) {
        mStep++;
    }

    if (mStep >= 90) {
        al::startNerveAction(this, "DeadInRouteDokan");
        return;
    }

    turn(mStateRouteDokan->getMoveDirection());

    if (al::updateNerveState(this)) {
        al::addVelocity(this, mStateRouteDokan->getFrontDirection());
        mIsEnableGravity = true;
        al::onCollide(this);
        startMoveAction();
        al::startSe(this, "PgRouteDokanOut");
    }
}

/**
 * @brief Vanishes with the disappear effect.
 */
void PlayerFireBall::exeDead() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgVanish");
        al::emitEffect(this, "Disappear", nullptr);
        kill();
    }
}

/**
 * @brief Vanishes with the disappear effect after touching a damaging floor.
 */
void PlayerFireBall::exeDeadDeathCode() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgVanish");
        al::emitEffect(this, "Disappear", nullptr);
        kill();
    }
}

/**
 * @brief Vanishes without any effect.
 */
void PlayerFireBall::exeDeadNoEffect() {
    if (al::isFirstStep(this)) {
        kill();
    }
}

/**
 * @brief Vanishes inside a route pipe.
 */
void PlayerFireBall::exeDeadInRouteDokan() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgVanishInRouteDokan");
        al::emitEffect(this, "DisappearInRouteDokan", nullptr);
        kill();
    }
}

/**
 * @brief Bounces the ball off a surface, depending on whether it is a floor, a ceiling or a wall.
 * @param rPos Position of the bounce.
 * @param rNormal Normal of the surface.
 */
void PlayerFireBall::reflect(const sead::Vector3f& rPos, const sead::Vector3f& rNormal) {
    mIsEnableGravity = true;

    f32 dot = rNormal.dot(sUpDir);

    if (dot > 0.70710677f) {
        boundGround(rPos);
    } else if (dot < -0.70710677f) {
        boundRoof();
    } else {
        boundWall(rNormal, rPos);
    }
}

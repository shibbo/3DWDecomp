#include "MapObj/BallStateFunction.hpp"
#include "MapObj/BallStateFallParam.hpp"
#include "MapObj/BallStateThrowParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <cmath>
namespace BallStateFunction {
void setPositionOnRelease(al::LiveActor* actor, const al::HitSensor* sensor, const al::LiveActor* holder) {
    sead::Vector3f position(al::getTrans(actor));
    if (sensor) {
        const sead::Vector3f& base = al::getTrans(al::getSensorHost(sensor));
        al::resetPosition(actor, base + sead::Vector3f(0.0f, al::getColliderRadius(actor) + 1.0f, 0.0f), false);
    } else al::resetPosition(actor, al::getTrans(holder), false);
    al::setTrans(actor, position);
}
void sendMsgToCollision(al::LiveActor* actor, bool skipAttack) {
    al::HitSensor* sensor = nullptr;
    if (al::isOnGround(actor, 0, 0.0f)) sensor = al::tryGetCollidedGroundSensor(actor);
    else if (al::isCollidedWall(actor)) sensor = al::tryGetCollidedWallSensor(actor);
    else if (al::isCollidedCeiling(actor)) sensor = al::tryGetCollidedCeilingSensor(actor);
    if (!sensor) return;
    if (!skipAttack) al::sendMsgBallAttackCollide(sensor, al::getHitSensor(actor, "Attack"));
    if (al::isOnGround(actor, 0, 0.0f)) al::sendMsgBallTrampleCollide(sensor, al::getHitSensor(actor, "Body"));
}
NOINLINE void calcFrontVelocity(sead::Vector3f* out, al::LiveActor* actor) {
    sead::Vector3f normal(0.0f, 0.0f, 0.0f);
    if (al::isOnGround(actor, 0, 0.0f)) normal.set(al::getOnGroundNormal(actor, 0));
    bool isZero = al::isNearZero(normal, 0.001f);
    out->set(al::getVelocity(actor));
    if (isZero) al::verticalizeVec(out, sead::Vector3f::ey, *out);
    else *out -= normal * normal.dot(*out);
}
float calcSpeedFront(al::LiveActor* actor) {
    sead::Vector3f velocity(0.0f, 0.0f, 0.0f);
    calcFrontVelocity(&velocity, actor);
    return velocity.length();
}
float calcSpeedUp(al::LiveActor* actor) {
    sead::Vector3f normal;
    al::calcCollidedNormalSum(actor, &normal);
    if (al::isNearZero(normal, 0.001f)) return al::calcSpeedV(actor);
    al::normalize(&normal);
    sead::Vector3f velocity(al::getVelocity(actor));
    return -velocity.dot(normal);
}
void rotateOnAir(al::LiveActor* actor, float speed, bool reverse) {
    sead::Vector3f direction(0.0f, 0.0f, 0.0f);
    direction = al::getVelocity(actor);
    al::normalizeOrZero(&direction);
    if (al::isNearZero(direction, 0.001f)) return;
    sead::Vector3f axis = direction.cross(sead::Vector3f::ey);
    al::normalizeOrZero(&axis);
    if (al::isNearZero(axis, 0.001f)) return;
    if (reverse) speed = -speed;
    sead::Quatf quat(al::getQuat(actor));
    al::rotateQuatRadian(&quat, quat, axis, sead::Mathf::deg2rad(speed));
    al::setQuat(actor, quat);
}
float calcRotateSpeed(al::LiveActor* actor, const sead::Vector3f& previous) {
    sead::Vector3f position(al::getTrans(actor));
    float distance = (position - previous).length();
    return distance * -180.0f / al::getColliderRadius(actor) / sead::Mathf::pi();
}
void rotateOnGround(al::LiveActor* actor, const sead::Vector3f& previous) {
    sead::Vector3f delta(al::getTrans(actor));
    delta -= previous;
    sead::Vector3f direction(delta.x, 0.0f, delta.z);
    al::normalizeOrZero(&direction);
    if (al::isNearZero(direction, 0.001f)) return;
    float distance = delta.length();
    sead::Vector3f axis = direction.cross(sead::Vector3f::ey);
    al::normalizeOrZero(&axis);
    if (al::isNearZero(axis, 0.001f)) return;
    float speed = distance * -180.0f / al::getColliderRadius(actor) / sead::Mathf::pi();
    sead::Quatf quat(al::getQuat(actor));
    al::rotateQuatRadian(&quat, quat, axis, sead::Mathf::deg2rad(speed));
    al::setQuat(actor, quat);
}
void calcLaunchSpeed(al::LiveActor* actor, const sead::Vector3f& direction, const BallStateThrowParam* param) {
    sead::Vector3f velocity(direction);
    velocity.y = 0.0f;
    velocity = velocity * param->mLaunchSpeed + sead::Vector3f(0.0f, param->mLaunchSpeedUp, 0.0f);
    al::setVelocity(actor, velocity);
}
void calcReflectSpeed(al::LiveActor* actor, const al::LiveActor* other, const BallStateFallParam* param) {
    sead::Vector3f velocity(0.0f, 0.0f, 0.0f);
    if (other) {
        float angle = sead::Mathf::deg2rad(param->_04);
        float horizontal = param->_00 * sinf(angle);
        velocity = al::getTrans(actor);
        velocity -= al::getTrans(other);
        velocity.y = 0.0f;
        if (velocity.x == 0.0f && velocity.z == 0.0f) velocity.set(0.0f, 0.0f, horizontal);
        else {
            al::normalize(&velocity);
            velocity.x *= horizontal;
            velocity.z *= horizontal;
        }
        velocity.y = param->_00 * cosf(angle);
    } else {
        velocity = al::getVelocity(actor);
        velocity.y = sead::Mathf::abs(velocity.y) * param->_08;
    }
    al::setVelocity(actor, velocity);
}
bool reboundCollisionGround(al::LiveActor* actor, const BallStateFallParam* param, bool water) {
    if (al::isOnGround(actor, 0, 0.0f)) {
        al::stopSeByName(actor, "PgBound");
        al::startSeWithParam(actor, "PgBound", sead::Mathf::abs(al::calcSpeedV(actor)) + sead::Mathf::abs(al::calcSpeedH(actor) * 0.5f), nullptr);
        return al::reboundVelocityFromCollision(actor, water ? param->mWaterReboundRate : param->mReboundRate, param->mReboundMinSpeed, param->mReboundFriction);
    }
    return true;
}
bool reboundCollisionWallOrCeiling(al::LiveActor* actor) {
    if (al::isCollidedWall(actor) || al::isCollidedCeiling(actor)) {
        if (al::isCollidedWallVelocity(actor) || al::isCollidedCeilingVelocity(actor)) {
            al::stopSeByName(actor, "PgBound");
            if (al::isCollidedWall(actor))
                al::startSeWithParam(actor, "PgBound", sead::Mathf::abs(al::calcSpeedH(actor)) + sead::Mathf::abs(al::calcSpeedV(actor) * 0.5f), nullptr);
            else al::startSeWithParam(actor, "PgBound", sead::Mathf::abs(al::calcSpeedV(actor)) + sead::Mathf::abs(al::calcSpeedH(actor) * 0.5f), nullptr);
        }
        al::reboundVelocityFromCollision(actor, 1.0f, 0.0f, 1.0f);
        return true;
    }
    return false;
}
bool getCollidedNormalAndPos(al::LiveActor* actor, sead::Vector3f* normal, sead::Vector3f* position) {
    if (al::isOnGround(actor, 0, 0.0f)) {
        normal->set(al::getOnGroundNormal(actor, 0));
        position->set(al::getCollidedGroundPos(actor));
    } else if (al::isCollidedWall(actor)) {
        normal->set(al::getCollidedWallNormal(actor));
        position->set(al::getCollidedWallPos(actor));
    } else if (al::isCollidedCeiling(actor)) {
        normal->set(al::getCollidedCeilingNormal(actor));
        position->set(al::getCollidedCeilingPos(actor));
    } else return false;
    return true;
}
void setColliderReturnedSlowly(al::LiveActor* actor, s32 target, s32 increment) {
    float radius = al::getColliderRadius(actor);
    if (radius == target) return;
    radius += increment;
    if (radius >= target) radius = target;
    al::setColliderRadius(actor, radius);
}
al::HitSensor* tryGetRelativePlayerSensor(al::LiveActor* actor, al::HitSensor* sensor) {
    if (al::isSensorPlayer(sensor)) return sensor;
    int user = rc::tryFindRelativeControlUserId(sensor);
    if (user == -1) return nullptr;
    return al::getHitSensor(rc::findPlayerActorFirstByUserId(actor, user), "Body");
}
}

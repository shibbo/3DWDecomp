#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"

/**
 * @brief Applies gravity and friction, including downward pressure at ground edges.
 * @param pHost Walking actor.
 * @param pParam Motion parameters.
 */
void WalkerStateFunction::calcPassiveMovement(al::LiveActor* pHost, const WalkerStateParam* pParam) {
    if (al::isOnGround(pHost, 0, 0.0f)) {
        if (al::isCollidedGroundEdgeOrCorner(pHost)) {
            al::addVelocityToGravity(pHost, 1.0f);
        }
        al::scaleVelocity(pHost, pParam->mGroundFriction);
    } else {
        al::addVelocityToGravity(pHost, pParam->mGravity);
        al::scaleVelocity(pHost, pParam->mAirFriction);
    }
}

/**
 * @brief Applies gravity and friction using a supplied ground-contact result.
 * @param pHost Walking actor.
 * @param pParam Motion parameters.
 * @param isOnGround Whether the actor is grounded.
 */
void WalkerStateFunction::calcPassiveMovement(al::LiveActor* pHost, const WalkerStateParam* pParam,
                                              bool isOnGround) {
    if (isOnGround) {
        al::scaleVelocity(pHost, pParam->mGroundFriction);
    } else {
        al::addVelocityToGravity(pHost, pParam->mGravity);
        al::scaleVelocity(pHost, pParam->mAirFriction);
    }
}

/**
 * @brief Tests for a ledge ahead using horizontal and downward collision rays.
 * @param pHost Actor providing the collision context.
 * @param rPosition Starting position.
 * @param rVelocity Movement direction to project onto the ground plane.
 * @param rGravity Downward direction.
 * @param distance Horizontal look-ahead distance.
 * @param rise Height of the ray origin above the position.
 * @param drop Depth to test below the position.
 * @param checkWall Whether an intervening wall prevents a fall result.
 * @return Whether the projected destination lacks supporting ground.
 */
bool WalkerStateFunction::isFallNextMove(const al::LiveActor* pHost,
        const sead::Vector3f& rPosition, const sead::Vector3f& rVelocity,
        const sead::Vector3f& rGravity, float distance, float rise, float drop, bool checkWall) {
    if (al::isNearZero(rGravity)) {
        return false;
    }
    sead::Vector3f movement;
    al::verticalizeVec(&movement, rGravity, rVelocity);
    if (al::isNearZero(movement)) {
        return false;
    }
    al::normalize(&movement);
    movement *= distance;
    sead::Vector3f destination = movement + rPosition - rGravity * rise;
    sead::Vector3f hitPosition;
    al::Triangle triangle;
    if (checkWall && alCollisionUtil::getFirstPolyOnArrow(pHost, &hitPosition, &triangle,
            rPosition - rGravity * rise, movement, nullptr, nullptr)) {
        return false;
    }
    return !alCollisionUtil::getFirstPolyOnArrow(pHost, &hitPosition, &triangle,
            destination, (rise + drop) * rGravity, nullptr, nullptr);
}

/**
 * @brief Tests for a ledge using the actor's current position, velocity, and gravity.
 * @param pHost Actor to test.
 * @param distance Horizontal look-ahead distance.
 * @param rise Height above the actor to start the test.
 * @param drop Depth below the actor to search for ground.
 * @param checkWall Whether to reject destinations behind walls.
 * @return Whether the next move would cross a ledge.
 */
bool WalkerStateFunction::isFallNextMove(const al::LiveActor* pHost, float distance,
                                         float rise, float drop, bool checkWall) {
    return isFallNextMove(pHost, al::getTrans(pHost), al::getVelocity(pHost),
                          al::getGravity(pHost), distance, rise, drop, checkWall);
}

/**
 * @brief Tests for a ledge from a vertically offset actor position.
 * @param pHost Actor to test.
 * @param offsetY World-space vertical offset.
 * @param distance Horizontal look-ahead distance.
 * @param rise Height above the offset position to start the test.
 * @param drop Depth below the offset position to search for ground.
 * @param checkWall Whether to reject destinations behind walls.
 * @return Whether the next move would cross a ledge.
 */
bool WalkerStateFunction::isFallNextMoveY(const al::LiveActor* pHost, float offsetY,
        float distance, float rise, float drop, bool checkWall) {
    sead::Vector3f position(al::getTrans(pHost));
    position.y += offsetY;
    return isFallNextMove(pHost, position, al::getVelocity(pHost), al::getGravity(pHost),
                          distance, rise, drop, checkWall);
}



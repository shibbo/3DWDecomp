#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcStateParam.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/AreaObjUtil.hpp"

/**
 * @brief Checks a position against the NPC avoidance areas.
 * @param pActor Actor supplying access to the area director.
 * @param rPosition Position to test.
 * @return Whether the position is inside an NPC avoidance area.
 */
bool NpcStateFunction::isInNPCAvoidArea(const al::LiveActor* pActor,
                                      const sead::Vector3f& rPosition) {
    return rc::isInAreaObj(pActor, rc::AreaObjType::NPCAvoidArea, rPosition);
}

/**
 * @brief Checks the actor's current position against NPC avoidance areas.
 * @param pActor Actor to test.
 * @return Whether the actor is inside an NPC avoidance area.
 */
bool NpcStateFunction::isInNPCAvoidArea(const al::LiveActor* pActor) {
    return isInNPCAvoidArea(pActor, al::getTrans(pActor));
}

/**
 * @brief Applies gravity and velocity damping according to ground contact.
 * @param pActor Actor whose velocity is updated.
 * @param pParam Gravity, damping, and ground-check parameters.
 */
void NpcStateFunction::calcPassiveMovement(al::LiveActor* pActor, const NpcStateParam* pParam) {
    if (al::isOnGround(pActor, pParam->mGroundCheckFrames, pParam->mGroundCheckMargin)) {
        al::scaleVelocity(pActor, pParam->mGroundVelocityScale);
    } else {
        al::addVelocityToGravity(pActor, pParam->mGravity);
        al::scaleVelocity(pActor, pParam->mAirVelocityScale);
    }
}

/**
 * @brief Applies passive movement using a supplied grounded state for damping.
 * @param pActor Actor whose velocity is updated.
 * @param pParam Gravity, damping, and ground-check margin.
 * @param isGrounded Selects ground damping; actual contact still suppresses gravity.
 */
void NpcStateFunction::calcPassiveMovement(al::LiveActor* pActor, const NpcStateParam* pParam,
                                        bool isGrounded) {
    if (isGrounded) {
        if (!al::isOnGround(pActor, 0, pParam->mGroundCheckMargin)) {
            al::addVelocityToGravity(pActor, pParam->mGravity);
        }
        al::scaleVelocity(pActor, pParam->mGroundVelocityScale);
    } else {
        al::addVelocityToGravity(pActor, pParam->mGravity);
        al::scaleVelocity(pActor, pParam->mAirVelocityScale);
    }
}

/**
 * @brief Probes for a drop ahead along the velocity projected onto the ground plane.
 * @param pActor Actor supplying collision access.
 * @param rPosition Start position.
 * @param rVelocity Movement direction before projection.
 * @param rGravity Gravity direction used for projection and the downward probe.
 * @param distance Forward probe length.
 * @param height Probe start height opposite gravity.
 * @param depth Probe depth below the starting position.
 * @param checkWall Suppresses a fall result when the forward probe hits a wall.
 * @return Whether the downward probe finds no supporting collision.
 */
bool NpcStateFunction::isFallNextMove(const al::LiveActor* pActor,
    const sead::Vector3f& rPosition, const sead::Vector3f& rVelocity,
    const sead::Vector3f& rGravity, float distance, float height, float depth, bool checkWall) {
    if (al::isNearZero(rGravity)) {
        return false;
    }
    sead::Vector3f direction;
    al::verticalizeVec(&direction, rGravity, rVelocity);
    if (al::isNearZero(direction)) {
        return false;
    }
    al::normalize(&direction);
    direction *= distance;
    const sead::Vector3f nextPosition = rPosition + direction - rGravity * height;
    sead::Vector3f hitPosition;
    al::Triangle triangle;
    if (checkWall && alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPosition, &triangle,
            rPosition - rGravity * height, direction, nullptr, nullptr)) {
        return false;
    }
    return !alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPosition, &triangle,
        nextPosition, rGravity * (height + depth), nullptr, nullptr);
}

/**
 * @brief Checks for a drop using the actor's current pose and velocity.
 * @param pActor Actor to test.
 * @param distance Forward probe length.
 * @param height Probe start height opposite gravity.
 * @param depth Downward probe depth.
 * @param checkWall Whether to test for a blocking wall first.
 * @return Whether the next movement would lead over an unsupported drop.
 */
bool NpcStateFunction::isFallNextMove(const al::LiveActor* pActor, float distance, float height,
                                     float depth, bool checkWall) {
    return isFallNextMove(pActor, al::getTrans(pActor), al::getVelocity(pActor),
                          al::getGravity(pActor), distance, height, depth, checkWall);
}

/**
 * @brief Tests a forward collision probe projected onto the ground plane.
 * @param pActor Actor supplying collision access.
 * @param rPosition Start position.
 * @param rVelocity Movement direction before projection.
 * @param rGravity Gravity direction used to define the ground plane.
 * @param distance Forward probe length.
 * @param height Probe start height opposite gravity.
 * @return Whether the forward probe hits collision.
 */
bool NpcStateFunction::isWallNextMove(const al::LiveActor* pActor,
    const sead::Vector3f& rPosition, const sead::Vector3f& rVelocity,
    const sead::Vector3f& rGravity, float distance, float height) {
    if (al::isNearZero(rGravity)) {
        return false;
    }
    sead::Vector3f direction;
    al::verticalizeVec(&direction, rGravity, rVelocity);
    if (al::isNearZero(direction)) {
        return false;
    }
    al::normalize(&direction);
    direction *= distance;
    sead::Vector3f hitPosition;
    al::Triangle triangle;
    return alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPosition, &triangle,
        rPosition - rGravity * height, direction, nullptr, nullptr);
}

/**
 * @brief Tests for a wall using the actor's current pose and velocity.
 * @param pActor Actor to test.
 * @param distance Forward probe length.
 * @param height Probe start height opposite gravity.
 * @return Whether the next movement encounters a wall.
 */
bool NpcStateFunction::isWallNextMove(const al::LiveActor* pActor, float distance, float height) {
    return isWallNextMove(pActor, al::getTrans(pActor), al::getVelocity(pActor),
                          al::getGravity(pActor), distance, height);
}

/**
 * @brief Tests the next collision position for NPC avoidance areas, water, or puddles.
 * @param pActor Actor supplying collision and area access.
 * @param rPosition Start position.
 * @param rVelocity Movement direction before projection.
 * @param rGravity Gravity direction used by the collision probes.
 * @param distance Forward probe length.
 * @param height Probe start height opposite gravity.
 * @param depth Probe depth below the starting position.
 * @param checkWall Also checks a forward wall hit against NPC avoidance areas.
 * @return Whether a probed collision position is unsuitable for the NPC.
 */
bool NpcStateFunction::isNPCAvoidAreaNextMove(const al::LiveActor* pActor,
    const sead::Vector3f& rPosition, const sead::Vector3f& rVelocity,
    const sead::Vector3f& rGravity, float distance, float height, float depth, bool checkWall) {
    if (al::isNearZero(rGravity)) {
        return false;
    }
    sead::Vector3f direction;
    al::verticalizeVec(&direction, rGravity, rVelocity);
    if (al::isNearZero(direction)) {
        return false;
    }
    al::normalize(&direction);
    direction *= distance;
    const sead::Vector3f nextPosition = rPosition + direction - rGravity * height;
    sead::Vector3f hitPosition;
    al::Triangle triangle;
    if (checkWall && alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPosition, &triangle,
            rPosition - rGravity * height, direction, nullptr, nullptr)
        && isInNPCAvoidArea(pActor, hitPosition)) {
        return true;
    }
    if (!alCollisionUtil::getFirstPolyOnArrow(pActor, &hitPosition, &triangle,
            nextPosition, rGravity * (height + depth), nullptr, nullptr)) {
        return false;
    }
    return isInNPCAvoidArea(pActor, hitPosition) || rc::isInWaterAreaNoSink(pActor, hitPosition)
        || al::isMaterialCode("Puddle", triangle);
}

/**
 * @brief Tests avoidance conditions using the actor's current pose and velocity.
 * @param pActor Actor to test.
 * @param distance Forward probe length.
 * @param height Probe start height opposite gravity.
 * @param depth Downward probe depth.
 * @param checkWall Whether to include wall-hit positions.
 * @return Whether the next movement reaches an unsuitable surface or area.
 */
bool NpcStateFunction::isNPCAvoidAreaNextMove(const al::LiveActor* pActor, float distance,
                                            float height, float depth, bool checkWall) {
    return isNPCAvoidAreaNextMove(pActor, al::getTrans(pActor), al::getVelocity(pActor),
                                 al::getGravity(pActor), distance, height, depth, checkWall);
}

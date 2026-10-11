#include "Enemy/TerritoryMover.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"

/** @brief Creates a roaming territory centered at the origin.
 * @param radius Radius used to select new targets.
 */
TerritoryMover::TerritoryMover(float radius)
    : mRadius(radius), mCenter(sead::Vector3f::zero), mTargetPos(sead::Vector3f::zero) {}

/** @brief Chooses a random target in the plane perpendicular to the actor's gravity.
 * @param pActor Actor whose gravity defines the movement plane.
 */
void TerritoryMover::decideNextTargetPos(const al::LiveActor* pActor) {
    sead::Vector3f direction;
    al::getRandomVector(&direction, 1.0f);
    al::normalizeOrZero(&direction);
    al::verticalizeVec(&direction, al::getGravity(pActor), direction);
    mTargetPos.x = mRadius * direction.x + mCenter.x;
    mTargetPos.y = mRadius * direction.y + mCenter.y;
    mTargetPos.z = mRadius * direction.z + mCenter.z;
}

/** @brief Checks the actor's distance from its target in the movement plane.
 * @param pActor Actor whose position and gravity are used.
 * @param distance Exclusive distance threshold for reaching the target.
 * @return Whether the planar distance is less than the threshold.
 */
bool TerritoryMover::isReachedTarget(const al::LiveActor* pActor, float distance) const {
    sead::Vector3f offset = mTargetPos;
    offset -= al::getTrans(pActor);
    al::verticalizeVec(&offset, al::getGravity(pActor), offset);
    return offset.length() < distance;
}

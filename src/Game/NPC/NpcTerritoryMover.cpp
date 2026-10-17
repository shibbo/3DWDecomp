#include "NPC/NpcTerritoryMover.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include <cmath>

/**
 * @brief Allocates the candidate positions for an NPC's territory.
 * @param pActor Actor supplying collision and area queries.
 * @param radius Radius of the territory.
 * @param spacing Distance between candidate positions.
 */
NpcTerritoryMover::NpcTerritoryMover(const al::LiveActor* pActor, float radius, float spacing)
    : mRadius(radius), mCenter(sead::Vector3f::zero),
      mTargetPosition(sead::Vector3f::zero), mSpacing(spacing), mActor(pActor) {
    const int diameter = 2 * static_cast<int>(radius / spacing);
    mTargets.allocBuffer(diameter * diameter, nullptr);
}

/**
 * @brief Chooses a random candidate, discarding positions without suitable ground.
 */
void NpcTerritoryMover::decideNextTargetPos() {
    if (mTargets.isEmpty()) {
        return;
    }
    const int index = al::getRandom(0, mTargets.size() - 1);
    if (mTargets.unsafeAt(index)->mIsValidated) {
        mTargetPosition.set(mTargets[index]->mPosition);
        return;
    }
    const al::LiveActor* pActor = mActor;
    const sead::Vector3f position(mTargets.unsafeAt(index)->mPosition);
    sead::Vector3f hitPosition;
    if (!alCollisionUtil::getFirstPolyOnArrow(
            pActor, &hitPosition, nullptr,
            position + sead::Vector3f::ey * 300.0f,
            sead::Vector3f::ey * -1000.0f, nullptr, nullptr) ||
        NpcStateFunction::isInNPCAvoidArea(pActor, hitPosition) ||
        rc::isInWaterAreaNoSink(pActor, hitPosition)) {
        mTargets.erase(index);
        decideNextTargetPos();
        return;
    }
    mTargetPosition.set(mTargets[index]->mPosition);
    mTargets[index]->mIsValidated = true;
}

/**
 * @brief Tests the actor's distance from the target in the plane perpendicular to gravity.
 * @param pActor Actor whose position is tested.
 * @param distance Distance required to reach the target.
 * @return Whether the planar distance is below the supplied distance.
 */
bool NpcTerritoryMover::isReachedTarget(const al::LiveActor* pActor, float distance) const {
    sead::Vector3f difference(mTargetPosition);
    difference -= al::getTrans(pActor);
    al::verticalizeVec(&difference, al::getGravity(pActor), difference);
    return difference.length() < distance;
}

/**
 * @brief Rebuilds the circular grid of candidate positions around the territory center.
 */
void NpcTerritoryMover::calculateTargets() {
    mTargets.clear();
    const int radius = static_cast<int>(mRadius / mSpacing);
    for (int x = -radius; x <= radius; ++x) {
        const int extent = static_cast<int>(std::fabs(std::sqrt(static_cast<float>(radius * radius - x * x))));
        for (int z = -extent; z <= extent; ++z) {
            const sead::Vector3f position(mCenter.x + mSpacing * x, mCenter.y,
                                          mCenter.z + mSpacing * z);
            Target* pTarget = mTargets.emplaceBack();
            pTarget->mPosition = position;
        }
    }
}

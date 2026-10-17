#include "NPC/RabbitRoute.hpp"
#include "NPC/RabbitRoutePoint.hpp"
#include "Library/Math/MathUtil.hpp"
#include <cfloat>

/**
 * @brief Constructs a rabbit route with room for 128 connected points.
 * @param rInfo Placement of the first route point.
 */
RabbitRoute::RabbitRoute(const al::PlacementInfo& rInfo) : mStartPoint(nullptr) {
    mPoints.allocBuffer(128, nullptr);
    mStartPoint = new RabbitRoutePoint(rInfo, nullptr, &mPoints);
}

/**
 * @brief Finds the closest clamped projection onto an outgoing route segment.
 * @param pPosition Receives the selected projection, or rPosition if the route has no segments.
 * @param rPosition Position from which squared distances are measured.
 */
void RabbitRoute::calcNearestPosOnRoute(sead::Vector3f* pPosition,
                                       const sead::Vector3f& rPosition) const {
    pPosition->set(rPosition);
    float bestDistance = FLT_MAX;
    for (int i = 0; i < mPoints.size(); ++i) {
        auto* pPoint = mPoints.unsafeAt(i);
        for (int j = 0; j < pPoint->getNextUniquePointNum(); ++j) {
            const auto* pNext = pPoint->getNextUniquePoint(j);
            sead::Vector3f projected;
            al::calcPerpendicFootToLineInside(&projected, rPosition, pPoint->getPos(), pNext->getPos());
            const float distance = (rPosition - projected).squaredLength();
            if (distance < bestDistance) {
                bestDistance = distance;
                pPosition->set(projected);
            }
        }
    }
}

/**
 * @brief Selects the farthest of the clamped projections onto outgoing route segments.
 * @param pPosition Receives the selected projection, or rPosition if no farther projection exists.
 * @param rPosition Position from which squared distances are measured.
 */
void RabbitRoute::calcFarthestPosOnRoute(sead::Vector3f* pPosition,
                                        const sead::Vector3f& rPosition) const {
    pPosition->set(rPosition);
    float bestDistance = 0.0f;
    for (int i = 0; i < mPoints.size(); ++i) {
        auto* pPoint = mPoints.unsafeAt(i);
        for (int j = 0; j < pPoint->getNextUniquePointNum(); ++j) {
            const auto* pNext = pPoint->getNextUniquePoint(j);
            sead::Vector3f projected;
            al::calcPerpendicFootToLineInside(&projected, rPosition, pPoint->getPos(), pNext->getPos());
            const float distance = (rPosition - projected).squaredLength();
            if (bestDistance < distance) {
                bestDistance = distance;
                pPosition->set(projected);
            }
        }
    }
}

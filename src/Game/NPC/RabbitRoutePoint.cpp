#include "NPC/RabbitRoutePoint.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"

namespace {
/**
 * @brief Finds an existing route point at a linked placement's position.
 * @param pPoints Registry to search in construction order.
 * @param rInfo Linked placement whose position should be matched.
 * @return First point within 0.001 units, or nullptr if no point matches.
 */
RabbitRoutePoint* findPoint(const sead::PtrArray<RabbitRoutePoint>* pPoints,
                           const al::PlacementInfo& rInfo) {
    sead::Vector3f position;
    al::getTrans(&position, rInfo);
    for (int i = 0; i < pPoints->size(); ++i) {
        auto* pPoint = (*pPoints)[i];
        sead::Vector3f difference = pPoint->getPos() - position;
        if (al::isNearZero(difference, 0.001f)) {
            return pPoint;
        }
    }
    return nullptr;
}
}

/**
 * @brief Builds connected rabbit route points, reusing previously visited positions.
 * @param rInfo Placement containing Action, IsOnLand, and NextPoint links.
 * @param pPreviousPoint Parent in the traversal, or nullptr for the first point.
 * @param pPoints Registry of points already constructed during the traversal.
 */
RabbitRoutePoint::RabbitRoutePoint(const al::PlacementInfo& rInfo,
                                 RabbitRoutePoint* pPreviousPoint,
                                 sead::PtrArray<RabbitRoutePoint>* pPoints)
    : mPosition(sead::Vector3f::zero), mAction(0), mIsOnLand(false) {
    al::getTrans(&mPosition, rInfo);
    al::tryGetArg(&mAction, rInfo, "Action");
    al::tryGetArg(&mIsOnLand, rInfo, "IsOnLand");
    pPoints->pushBack(this);
    for (int i = 0; i < al::calcLinkChildNum(rInfo, "NextPoint"); ++i) {
        al::PlacementInfo nextInfo;
        al::getLinksInfoByIndex(&nextInfo, rInfo, "NextPoint", i);
        RabbitRoutePoint* pNextPoint = findPoint(pPoints, nextInfo);
        if (pNextPoint) {
            pNextPoint->addNextPoint(this);
        } else {
            pNextPoint = new RabbitRoutePoint(nextInfo, this, pPoints);
        }
        addNextPoint(pNextPoint);
        mNextUniquePoints.pushBack(pNextPoint);
    }
    if (pPreviousPoint) {
        addNextPoint(pPreviousPoint);
    }
}

/**
 * @brief Adds an adjacent point when the fixed array has room.
 * @param pPoint Point to append; duplicates are retained.
 */
void RabbitRoutePoint::addNextPoint(RabbitRoutePoint* pPoint) { mNextPoints.pushBack(pPoint); }

/** @brief Checks the configured action. @return Whether Action is 1 (jump). */
bool RabbitRoutePoint::isActionJump() const { return mAction == 1; }

/** @brief Gets this point's position. @return Reference to the stored position. */
const sead::Vector3f& RabbitRoutePoint::getPos() const { return mPosition; }

/** @brief Counts all adjacent points. @return Number of stored adjacent points. */
int RabbitRoutePoint::getNextPointNum() const { return mNextPoints.size(); }

/**
 * @brief Gets an adjacent point.
 * @param index Zero-based index into the adjacent points.
 * @return The indexed point, or nullptr if the index is outside the array.
 */
const IUseRabbitRoutePoint* RabbitRoutePoint::getNextPoint(int index) const {
    return mNextPoints[index];
}

/** @brief Counts outgoing placement links. @return Number of stored outgoing points. */
int RabbitRoutePoint::getNextUniquePointNum() const { return mNextUniquePoints.size(); }

/**
 * @brief Gets a point reached by an outgoing placement link.
 * @param index Zero-based index into the outgoing points.
 * @return The indexed point, or nullptr if the index is outside the array.
 */
const IUseRabbitRoutePoint* RabbitRoutePoint::getNextUniquePoint(int index) const {
    return mNextUniquePoints[index];
}

/** @brief Checks the placement's terrain flag. @return Whether IsOnLand is enabled. */
bool RabbitRoutePoint::isPointOnLand() const { return mIsOnLand; }

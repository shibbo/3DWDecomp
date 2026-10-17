#include "NPC/RabbitInitPlacePoint.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Creates the rabbit's starting point from its placement.
 * @param rInfo Placement containing the position and optional Action argument.
 * @param pNextPoint Point reached after leaving the initial placement.
 */
RabbitInitPlacePoint::RabbitInitPlacePoint(const al::PlacementInfo& rInfo,
                                         const IUseRabbitRoutePoint* pNextPoint)
    : mPosition(sead::Vector3f::zero), mNextPoint(pNextPoint), mAction(1) {
    al::getTrans(&mPosition, rInfo);
    al::tryGetArg(&mAction, rInfo, "Action");
}

/** @brief Identifies this point as an initial placement. @return False. */
bool RabbitInitPlacePoint::isRoutePoint() const { return false; }

/** @brief Checks the configured action. @return Whether Action is 1 (jump). */
bool RabbitInitPlacePoint::isActionJump() const { return mAction == 1; }

/** @brief Gets the initial position. @return Reference to the stored position. */
const sead::Vector3f& RabbitInitPlacePoint::getPos() const { return mPosition; }

/** @brief Gets the outgoing point count. @return One. */
int RabbitInitPlacePoint::getNextPointNum() const { return 1; }

/**
 * @brief Gets the sole outgoing point.
 * @param index Point index; unused because there is only one outgoing point.
 * @return The next point supplied at construction.
 */
const IUseRabbitRoutePoint* RabbitInitPlacePoint::getNextPoint(int index) const { return mNextPoint; }

/** @brief Gets the unique outgoing point count. @return One. */
int RabbitInitPlacePoint::getNextUniquePointNum() const { return 1; }

/**
 * @brief Gets the sole unique outgoing point.
 * @param index Point index; unused because there is only one outgoing point.
 * @return The next point supplied at construction.
 */
const IUseRabbitRoutePoint* RabbitInitPlacePoint::getNextUniquePoint(int index) const {
    return mNextPoint;
}

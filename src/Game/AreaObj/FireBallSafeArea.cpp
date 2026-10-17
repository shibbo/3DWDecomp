#include "AreaObj/FireBallSafeArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Project/Collision/CollisionParts.hpp"

/**
 * @brief Constructs a fireball safe area with default collision settings.
 * @param pName Area name.
 */
FireBallSafeArea::FireBallSafeArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Loads fireball filtering and collision connection settings.
 * @param rInfo Area placement and switch information.
 */
void FireBallSafeArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    al::tryGetArg(&mIgnoreGiantFireballs, *mPlacementInfo, "IgnoreGiantFireballs");
    al::tryGetArg(&mIsConnectToCollision, *mPlacementInfo, "IsConnectToCollision");
    al::tryGetArg(&mCollisionCheckDist, *mPlacementInfo, "CollisionCheckDist");
}

/** @brief Performs the empty post-placement initialization. */
void FireBallSafeArea::initAfterPlacement() {}

/** @brief Updates the area's position from connected collision geometry. */
void FireBallSafeArea::tryFollowCollisionConnection() {
    if (mCollisionParts) {
        const sead::Matrix34f& rMtx = mCollisionParts->getBaseMtx();
        _28.setTranslation(rMtx.getTranslation() + mCollisionOffset);
    }
}

/**
 * @brief Queries whether this area ignores giant fireballs.
 * @return The configured giant-fireball filtering flag.
 */
bool FireBallSafeArea::isIgnoreGiantFireballs() {
    return mIgnoreGiantFireballs;
}

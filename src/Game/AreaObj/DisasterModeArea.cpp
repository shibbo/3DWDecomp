#include "AreaObj/DisasterModeArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Constructs a disaster mode area without a spawn location.
 * @param pName Area name.
 */
DisasterModeArea::DisasterModeArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Initializes the area and resolves its spawn location identifier.
 * @param rInfo Area placement and switch information.
 * @param pHolder Unused scene object holder.
 */
void DisasterModeArea::init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) {
    al::AreaObj::init(rInfo);
    mSpawnLocationId = rInfo.mPlacementInfo._28;
    if (mSpawnLocationId < 0) {
        al::tryGetArg(&mSpawnLocationId, rInfo.mPlacementInfo, "SpawnLocationId");
    }
}

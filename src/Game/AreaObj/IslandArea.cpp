#include "AreaObj/IslandArea.hpp"
#include "MapObj/IslandAreaWatcher.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Scene/SceneObjHolder.hpp"

/**
 * @brief Constructs an island area with disaster events enabled.
 * @param pName Area name.
 */
IslandArea::IslandArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Loads island settings and registers with the scene's island watcher when present.
 * @param rInfo Area placement and switch information.
 * @param pHolder Scene object holder queried for the island watcher.
 */
void IslandArea::init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) {
    al::AreaObj::init(rInfo);
    if (mZoneID < 0) {
        al::tryGetArg(&mZoneID, rInfo.mPlacementInfo, "IslandID");
    }
    al::tryGetArg(&mNoDisaster, rInfo.mPlacementInfo, "NoDisaster");
    if (pHolder->isExist(39)) {
        static_cast<IslandAreaWatcher*>(pHolder->getObj(39))->registerIslandArea(this);
    }
}

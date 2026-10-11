#include "AreaObj/IslandArea.hpp"
#include "MapObj/IslandAreaWatcher.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Scene/SceneObjHolder.hpp"

IslandArea::IslandArea(const char* pName) : al::AreaObj(pName) {}

void IslandArea::init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) {
    al::AreaObj::init(rInfo);
    if (mZoneID < 0)
        al::tryGetArg(&mZoneID, rInfo.mPlacementInfo, "IslandID");
    al::tryGetArg(&mNoDisaster, rInfo.mPlacementInfo, "NoDisaster");
    if (pHolder->isExist(39))
        static_cast<IslandAreaWatcher*>(pHolder->getObj(39))->registerIslandArea(this);
}

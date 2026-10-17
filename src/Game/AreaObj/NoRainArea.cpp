#include "AreaObj/NoRainArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Constructs an area that suppresses rain.
 * @param pName Area name.
 */
NoRainArea::NoRainArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Initializes the area and its camera exclusion setting.
 * @param rInfo Area placement and switch information.
 */
void NoRainArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    al::tryGetArg(&mIsIgnoreCamera, rInfo.mPlacementInfo, "IsIgnoreCamera");
}

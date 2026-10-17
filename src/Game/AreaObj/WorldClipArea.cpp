#include "AreaObj/WorldClipArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Constructs an area with no active world bits.
 * @param pName Area name.
 */
WorldClipArea::WorldClipArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Accumulates the four configured active worlds into a bit mask.
 * @param rInfo Area placement and switch information.
 */
void WorldClipArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    s32 world = 0;
    al::tryGetArg(&world, rInfo.mPlacementInfo, "ActiveWorld0");
    mActiveWorldMask |= 1u << world;
    al::tryGetArg(&world, rInfo.mPlacementInfo, "ActiveWorld1");
    mActiveWorldMask |= 1u << world;
    al::tryGetArg(&world, rInfo.mPlacementInfo, "ActiveWorld2");
    mActiveWorldMask |= 1u << world;
    al::tryGetArg(&world, rInfo.mPlacementInfo, "ActiveWorld3");
    mActiveWorldMask |= 1u << world;
}

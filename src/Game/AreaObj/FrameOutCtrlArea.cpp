#include "AreaObj/FrameOutCtrlArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Constructs an area controlling frame-out checks.
 * @param pName Area name.
 */
FrameOutCtrlArea::FrameOutCtrlArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Loads the frame-out checks and disabled-state icon setting.
 * @param rInfo Area placement and switch information.
 */
void FrameOutCtrlArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    al::PlacementInfo placementInfo(rInfo.mPlacementInfo);
    al::tryGetArg(&mEnableHorizontalCheck, placementInfo, "EnableHorizontalCheck");
    al::tryGetArg(&mEnableVerticalCheck, placementInfo, "EnableVerticalCheck");
    al::tryGetArg(&mShowIconAtDisable, placementInfo, "ShowIconAtDisable");
}

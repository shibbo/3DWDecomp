#include "AreaObj/InkArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Constructs an ink area with no texture overrides.
 * @param pName Area name.
 */
InkArea::InkArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Loads ink texture overrides and shader settings.
 * @param rInfo Area placement and switch information.
 */
void InkArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    al::tryGetStringArg(&mTextureName, rInfo, "TextureName");
    al::tryGetStringArg(&mForceInkName, rInfo, "ForceInkName");
    if (mForceInkName && mForceInkName[0] == '\0') {
        mForceInkName = nullptr;
    }
    al::tryGetArg(&mIsDisableShaderInk, rInfo.mPlacementInfo, "IsDisableShaderInk");
}

/**
 * @brief Dispatches to the area's placement initialization.
 * @param rInfo Area placement and switch information.
 * @param pHolder Unused scene object holder.
 */
void InkArea::init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) {
    init(rInfo);
}

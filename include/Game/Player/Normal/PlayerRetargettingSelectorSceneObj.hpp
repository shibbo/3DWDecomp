#pragma once

#include "Library/Scene/ISceneObj.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"

/**
 * @brief PlayerRetargettingSelector shared through the scene object holder.
 * @note Only the members used by already-decompiled callers are declared.
 */
class PlayerRetargettingSelectorSceneObj : public PlayerRetargettingSelector,
                                           public al::ISceneObj {
public:
    PlayerRetargettingSelectorSceneObj();
};

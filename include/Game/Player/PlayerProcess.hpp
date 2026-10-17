#pragma once

#include <basis/seadTypes.h>
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class PlayerHolder;
}  // namespace al

/**
 * @brief Scene object running the per-frame processing shared by all players.
 * @note Only the members used by already-decompiled callers are declared.
 */
class PlayerProcess : public al::ISceneObj {
public:
    explicit PlayerProcess(al::PlayerHolder* pPlayerHolder);

private:
    u8 _8[0x18 - 0x8];
};

static_assert(sizeof(PlayerProcess) == 0x18);

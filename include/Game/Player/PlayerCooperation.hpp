#pragma once

#include <basis/seadTypes.h>
#include "Library/Scene/ISceneObj.hpp"

/**
 * @brief Scene object coordinating the players of a multiplayer session.
 * @note Only the members used by already-decompiled callers are declared.
 */
class PlayerCooperation : public al::ISceneObj {
public:
    explicit PlayerCooperation(u32 num);

    void update();

private:
    u8 _8[0x48 - 0x8];
};

static_assert(sizeof(PlayerCooperation) == 0x48);

#pragma once

#include <basis/seadTypes.h>

namespace al {
class ExecuteDirector;
class IUseSceneObjHolder;
class PlayerHolder;
struct SceneInitInfo;
}  // namespace al

class GameDataHolder;

/**
 * @brief Director of the shadow Mario ghosts of Bowser's Fury.
 * @note Only what reconstructed code needs is declared so far.
 */
class ShadowMarioDirector {
public:
    ShadowMarioDirector(s32 maxGhosts, const al::IUseSceneObjHolder* pHolder,
                        const al::SceneInitInfo& rInfo, al::ExecuteDirector* pExecuteDirector,
                        al::PlayerHolder* pPlayerHolder, const GameDataHolder* pGameDataHolder);

private:
    u8 _0[0x28];
};
static_assert(sizeof(ShadowMarioDirector) == 0x28);

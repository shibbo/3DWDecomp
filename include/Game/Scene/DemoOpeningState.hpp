#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class GraphicsSystemInfo;
class Scene;
}  // namespace al

/**
 * @brief Scene state playing the opening demo of a new game.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class DemoOpeningState : public al::NerveStateBase {
public:
    DemoOpeningState(al::Scene* pScene, const al::ActorInitInfo& rInfo,
                     al::GraphicsSystemInfo* pGraphicsSystemInfo, sead::Matrix34f* pMtx);

private:
    u8 _pad[0x68 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(DemoOpeningState) == 0x68);

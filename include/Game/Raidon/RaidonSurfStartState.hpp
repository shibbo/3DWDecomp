#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class RaidonBase;

/// Surfing Plessie's start sequence after all players got on.
/// @note Only what reconstructed code needs is declared so far.
class RaidonSurfStartState : public al::NerveStateBase {
public:
    RaidonSurfStartState(const char* pName, RaidonBase* pHost, const al::ActorInitInfo& rInfo);

    void start();

private:
    u8 _11[0x28 - 0x11];
};

static_assert(sizeof(RaidonSurfStartState) == 0x28);

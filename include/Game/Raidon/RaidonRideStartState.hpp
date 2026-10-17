#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class RaidonBase;

/// Plessie starting to move once every player got on.
class RaidonRideStartState : public al::NerveStateBase {
public:
    RaidonRideStartState(const char* pName, RaidonBase* pHost, const al::ActorInitInfo& rInfo);

private:
    u8 _11[0x28 - 0x11];
};

static_assert(sizeof(RaidonRideStartState) == 0x28);
